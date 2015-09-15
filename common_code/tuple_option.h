#ifndef __TUPLE_OPTION_H
#define __TUPLE_OPTION_H

#include <tuple>
#include "try_move.h"

template <typename... ARGS>
struct types_pck;

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

/*!
@brief 比较数，取最大的值或最小值
*/
template <long long A, long long B = 0, long long... NUMS>
struct static_cmp
{
	template <long long A, long long B>
	struct cmp
	{
		template <bool = true>
		struct result
		{
			enum { max = A, min = B };
		};

		template <>
		struct result<false>
		{
			enum { max = B, min = A };
		};

		enum { max = result<(A > B)>::max, min = result<(A > B) >::min };
	};

	enum
	{
		max = cmp<A, static_cmp<B, NUMS...>::max>::max,
		min = cmp<A, static_cmp<B, NUMS...>::min>::min
	};
};

template <long long N>
struct static_cmp<N>
{
	enum { min = N, max = N };
};

template <typename... TYPES>
struct static_cmp_type_size: public static_cmp<sizeof(TYPES)...>
{
};

template <size_t N, typename FIRST, typename... ARGS>
struct types_element
{
	typedef typename types_element<N - 1, ARGS...>::type type;
};

template <typename FIRST, typename... ARGS>
struct types_element<0, FIRST, ARGS...>
{
	typedef FIRST type;
};

template <size_t N>
struct static_args_get 
{
	template <typename First, typename... Args>
	static inline typename types_element<N - 1, Args...>::type& get(First& fst, Args&... args)
	{
		static_assert(N < 1 + sizeof...(Args), "");
		return static_args_get<N - 1>::get(args...);
	}
};

template <>
struct static_args_get<0>
{
	template <typename First, typename... Args>
	static inline First& get(First& fst, Args&...)
	{
		return fst;
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

template <typename First>
inline typename First& args_get(const size_t n, First& fst)
{
	assert(0 == n);
	return fst;
}

template <typename First, typename... Args>
inline typename First& args_get(const size_t n, First& fst, Args&... args)
{
	if (n)
	{
		return args_get(n - 1, args...);
	} 
	else
	{
		return fst;
	}
}

#endif