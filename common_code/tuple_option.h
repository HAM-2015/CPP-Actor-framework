#ifndef __TUPLE_OPTION_H
#define __TUPLE_OPTION_H

#include <tuple>
#include "try_move.h"

template <typename TUPLE, size_t N>
struct tuple_move
{
	typedef typename std::tuple_element<N, RM_REF(TUPLE)>::type type;

	static RM_REF(type)& get(TUPLE& tup)
	{
		return std::get<N>(tup);
	}
};

template <typename TUPLE, size_t N>
struct tuple_move<TUPLE&&, N>
{
	typedef typename std::tuple_element<N, RM_REF(TUPLE)>::type type;

	static RM_REF(type) && get(TUPLE&& tup)
	{
		return std::move(std::get<N>(tup));
	}
};

template <typename R, size_t N>
struct apply_arg
{
	template <typename H, typename TUPLE, typename... Args>
	static inline R append(H&& h, TUPLE&& tp, Args&&... args)
	{
		return apply_arg<R, N - 1>::append(TRY_MOVE(h), TRY_MOVE(tp), tuple_move<TUPLE&&, N - 1>::get(TRY_MOVE(tp)), TRY_MOVE(args)...);
	}

	template <typename F, typename TUPLE, typename... Args>
	static inline R append(RM_REF(F) pf, TUPLE&& tp, Args&&... args)
	{
		return apply_arg<R, N - 1>::append(pf, TRY_MOVE(tp), tuple_move<TUPLE&&, N - 1>::get(TRY_MOVE(tp)), TRY_MOVE(args)...);
	}

	template <typename F, typename C, typename TUPLE, typename... Args>
	static inline R append(F pf, C* obj, TUPLE&& tp, Args&&... args)
	{
		return apply_arg<R, N - 1>::append(pf, obj, TRY_MOVE(tp), tuple_move<TUPLE&&, N - 1>::get(TRY_MOVE(tp)), TRY_MOVE(args)...);
	}

	template <typename TUPLE, typename FIRST, typename... Args>
	static inline void to_args(TUPLE&& tp, FIRST& fst, Args&... dsts)
	{
		fst = tuple_move<TUPLE&&, std::tuple_size<RM_REF(TUPLE)>::value - N>::get(TRY_MOVE(tp));
		apply_arg<R, N - 1>::to_args(TRY_MOVE(tp), dsts...);
	}

	template <typename TUPLE, typename FIRST, typename... Args>
	static inline void to_tuple(TUPLE& dst, FIRST&& fst, Args&&... args)
	{
		std::get<std::tuple_size<TUPLE>::value - N>(dst) = TRY_MOVE(fst);
		apply_arg<R, N - 1>::to_tuple(dst, TRY_MOVE(args)...);
	}
};

template <typename R>
struct apply_arg<R, 0>
{
	template <typename H, typename TUPLE, typename... Args>
	static inline R append(H&& h, TUPLE&&, Args&&... args)
	{
		return h(TRY_MOVE(args)...);
	}

	template <typename F, typename TUPLE, typename... Args>
	static inline R append(RM_REF(F) pf, TUPLE&&, Args&&... args)
	{
		return pf(TRY_MOVE(args)...);
	}

	template <typename F, typename C, typename TUPLE, typename... Args>
	static inline R append(F pf, C* obj, TUPLE&&, Args&&... args)
	{
		return (obj->*pf)(TRY_MOVE(args)...);
	}

	template <typename TUPLE>
	static inline void to_args(TUPLE&&)
	{

	}

	template <typename TUPLE>
	static inline void to_tuple(TUPLE&)
	{

	}
};

template <typename R = void, typename H, typename TUPLE>
inline R tuple_invoke(H&& h, TUPLE&& t)
{
	return apply_arg<R, std::tuple_size<RM_REF(TUPLE)>::value>::append(TRY_MOVE(h), TRY_MOVE(t));
}

template <typename R = void, typename F, typename TUPLE>
inline R tuple_invoke(RM_REF(F) pf, TUPLE&& t)
{
	return apply_arg<R, std::tuple_size<RM_REF(TUPLE)>::value>::append(pf, TRY_MOVE(t));
}

template <typename R = void, typename F, typename C, typename TUPLE>
inline R tuple_invoke(F pf, C* obj, TUPLE&& t)
{
	return apply_arg<R, std::tuple_size<RM_REF(TUPLE)>::value>::append(pf, obj, TRY_MOVE(t));
}

template <typename TUPLE, typename... Args>
inline void tuple_to_args(TUPLE&& t, Args&... dsts)
{
	static_assert(std::tuple_size<RM_REF(TUPLE)>::value == sizeof...(Args), "src size != dst size");
	apply_arg<void, std::tuple_size<RM_REF(TUPLE)>::value>::to_args(TRY_MOVE(t), dsts...);
}

template <typename TUPLE, typename... Args>
inline void args_to_tuple(TUPLE& dst, Args&&... args)
{
	static_assert(std::tuple_size<RM_REF(TUPLE)>::value == sizeof...(Args), "src size != dst size");
	apply_arg<void, std::tuple_size<RM_REF(TUPLE)>::value>::to_tuple(dst, TRY_MOVE(args)...);
}

#endif