#ifndef __TUPLE_OPTION_H
#define __TUPLE_OPTION_H

#include <tuple>
#include "try_move.h"

template <typename... TYPES>
struct types_pck
{
	enum { number = sizeof...(TYPES) };
};
//////////////////////////////////////////////////////////////////////////

template <size_t I, typename FIRST, typename... TYPES>
struct SingleElement_ : public SingleElement_<I - 1, TYPES...> {};

template <typename FIRST, typename... TYPES>
struct SingleElement_<0, FIRST, TYPES...>
{
	typedef FIRST type;
};

/*!
@brief 取第I个类型
*/
template <size_t I, typename FIRST, typename... TYPES>
struct single_element : public SingleElement_<I, FIRST, TYPES...> {};

template <size_t I, typename... TYPES>
struct single_element<I, types_pck<TYPES...>>: public single_element<I, TYPES...>{};

 template <size_t I, typename... TYPES>
 struct single_element<I, std::tuple<TYPES...>>: public single_element<I, TYPES...>{};
 
 template <size_t I, typename... TYPES>
 struct single_element<I, const std::tuple<TYPES...>>: public single_element<I, TYPES...>{};
//////////////////////////////////////////////////////////////////////////

template <typename T>
struct CheckRef0_
{
	typedef T& type;
};

template <typename T>
struct CheckRef0_<T&&>
{
	typedef T&& type;
};

/*!
@brief 尝试移动tuple中的第I个参数
*/
template <size_t I, typename TUPLE>
struct tuple_move
{
	typedef typename single_element<I, RM_REF(TUPLE)>::type type;

	template <typename Tuple>
	static auto get(Tuple&& tup)->typename CheckRef0_<type>::type
	{
		return (typename CheckRef0_<type>::type)std::get<I>(tup);
	}
};

template <typename T>
struct CheckRef1_
{
	typedef const T& type;
};

template <typename T>
struct CheckRef1_<T&&>
{
	typedef const T& type;
};

template <typename T>
struct CheckRef1_<T&>
{
	typedef T& type;
};

template <size_t I, typename TUPLE>
struct tuple_move<I, const TUPLE&>
{
	typedef typename single_element<I, RM_REF(TUPLE)>::type type;

	template <typename Tuple>
	static auto get(Tuple&& tup)->typename CheckRef1_<type>::type
	{
		return (typename CheckRef1_<type>::type)std::get<I>(tup);
	}
};

template <typename T>
struct CheckRef2_
{
	typedef T&& type;
};

template <typename T>
struct CheckRef2_<T&>
{
	typedef T& type;
};

template <size_t I, typename TUPLE>
struct tuple_move<I, TUPLE&&>
{
	typedef typename single_element<I, RM_REF(TUPLE)>::type type;

	template <typename Tuple>
	static auto get(Tuple&& tup)->typename CheckRef2_<type>::type
	{
		return (typename CheckRef2_<type>::type)std::get<I>(tup);
	}
};
 
 template <size_t I, typename TUPLE>
 struct tuple_move<I, const TUPLE&&>: public tuple_move<I, const TUPLE&>{};

template <typename R, size_t N>
struct ApplyArg_
{
	template <typename Handler, typename Tuple, typename... Args>
	static inline R append(Handler&& h, Tuple&& tup, Args&&... args)
	{
		return ApplyArg_<R, N - 1>::append(TRY_MOVE(h), TRY_MOVE(tup), tuple_move<N - 1, Tuple&&>::get(TRY_MOVE(tup)), TRY_MOVE(args)...);
	}

	template <typename Tuple, typename First, typename... Args>
	static inline void to_args(Tuple&& tup, First& fst, Args&... dsts)
	{
		fst = tuple_move<std::tuple_size<RM_CREF(Tuple)>::value - N, Tuple&&>::get(TRY_MOVE(tup));
		ApplyArg_<R, N - 1>::to_args(TRY_MOVE(tup), dsts...);
	}

	template <typename Tuple, typename First, typename... Args>
	static inline void to_tuple(Tuple&& dst, First&& fst, Args&&... args)
	{
		std::get<std::tuple_size<RM_CREF(Tuple)>::value - N>(dst) = TRY_MOVE(fst);
		ApplyArg_<R, N - 1>::to_tuple(TRY_MOVE(dst), TRY_MOVE(args)...);
	}
};

template <typename R>
struct ApplyArg_<R, 0>
{
	template <typename Handler, typename Tuple, typename... Args>
	static inline R append(Handler&& h, Tuple&&, Args&&... args)
	{
		return h(TRY_MOVE(args)...);
	}

	template <typename Tuple>
	static inline void to_args(Tuple&&)
	{

	}

	template <typename Tuple>
	static inline void to_tuple(Tuple&&)
	{

	}
};

template <typename R, typename Handler, typename... TYPES>
struct TupleInvokeWrap_;

template <typename R, typename Handler, typename... TYPES>
struct TupleInvokeWrap_<R, Handler, std::tuple<TYPES...>&&>
{
	template <typename... Args>
	inline R operator ()(Args&&... args)
	{
		return ApplyArg_<R, sizeof...(TYPES)>::append(_h, std::move(_tuple), TRY_MOVE(args)...);
	}

	Handler& _h;
	std::tuple<TYPES...>& _tuple;
};

template <typename R, typename Handler, typename... TYPES>
struct TupleInvokeWrap_<R, Handler, std::tuple<TYPES...>&>
{
	template <typename... Args>
	inline R operator ()(Args&&... args)
	{
		return ApplyArg_<R, sizeof...(TYPES)>::append(_h, _tuple, TRY_MOVE(args)...);
	}

	Handler& _h;
	std::tuple<TYPES...>& _tuple;
};

template <typename R, typename Handler, typename... TYPES>
struct TupleInvokeWrap_<R, Handler, const std::tuple<TYPES...>&>
{
	template <typename... Args>
	inline R operator ()(Args&&... args)
	{
		return ApplyArg_<R, sizeof...(TYPES)>::append(_h, _tuple, TRY_MOVE(args)...);
	}

	Handler& _h;
	const std::tuple<TYPES...>& _tuple;
};

template <typename R, typename Handler, typename... TYPES>
struct TupleInvokeWrap_<R, Handler, const std::tuple<TYPES...>&&>
{
	template <typename... Args>
	inline R operator ()(Args&&... args)
	{
		return ApplyArg_<R, sizeof...(TYPES)>::append(_h, _tuple, TRY_MOVE(args)...);
	}

	Handler& _h;
	const std::tuple<TYPES...>& _tuple;
};

template <typename R, typename Handler, typename TYPE>
struct TupleInvokeWrap_<R, Handler, TYPE>
{
	template <typename... Args>
	inline R operator ()(Args&&... args)
	{
		return _h((TYPE)_type, TRY_MOVE(args)...);
	}

	Handler& _h;
	TYPE& _type;
};

template <typename R, typename F, typename C>
struct InvokeWrapObj_
{
	template <typename... Args>
	inline R operator ()(Args&&... args)
	{
		return (obj->*pf)(TRY_MOVE(args)...);
	}

	F pf;
	C* obj;
};

template <typename R, bool>
struct TupleInvoke_ 
{
	template <typename Handler>
	static inline R tuple_invoke(Handler&& h)
	{
		return h();
	}

	template <typename Handler, typename First, typename... Tuple>
	static inline R tuple_invoke(Handler&& h, First&& fst, Tuple&&... tups)
	{
		TupleInvokeWrap_<R, Handler, First&&> wrap = { h, fst };
		return tuple_invoke(wrap, TRY_MOVE(tups)...);
	}
};

template <typename R>
struct TupleInvoke_<R, true>
{
	template <typename F, typename C, typename... Tuple>
	static inline R tuple_invoke(F pf, C* obj, Tuple&&... tups)
	{
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return TupleInvoke_<R, false>::tuple_invoke(wrap, TRY_MOVE(tups)...);
	}
};

template <typename T>
struct CheckClassFunc_
{
	enum { value = false };
};

template <typename R, typename C>
struct CheckClassFunc_<R(C::*)>
{
	enum { value = true };
};

template <typename R, typename C, typename... Types>
struct CheckClassFunc_<R(C::*)(Types...) const>
{
	enum { value = true };
};

/*!
@brief 从tuple中取参数调用某个函数
*/
template <typename R = void, typename Handler, typename Unknown, typename... Tuple>
inline R tuple_invoke(Handler&& h, Unknown&& unkown, Tuple&&... tups)
{
	return TupleInvoke_<R, CheckClassFunc_<Handler>::value>::tuple_invoke(TRY_MOVE(h), TRY_MOVE(unkown), TRY_MOVE(tups)...);
}

template <typename R = void, typename Handler>
inline R tuple_invoke(Handler&& h)
{
	static_assert(!CheckClassFunc_<Handler>::value, "");
	return h();
}

template <typename R, size_t N>
struct ApplyRval_ 
{
	template <typename Handler, typename Arg>
	struct wrap_handler
	{
		template <typename... Args>
		R operator ()(Args&&... args)
		{
			bool rval = 0 != try_move<Arg>::can_move;
			return _h(_arg, rval, TRY_MOVE(args)...);
		}

		Handler& _h;
		Arg& _arg;
	};

	template <typename Handler, typename First, typename... Args>
	static inline R append(Handler&& h, First&& fst, Args&&... args)
	{
		wrap_handler<Handler, First&&> wrap = { h, fst };
		return ApplyRval_<R, N - 1>::append(wrap, TRY_MOVE(args)...);
	}
};

template <typename R>
struct ApplyRval_<R, 0>
{
	template <typename Handler>
	static inline R append(Handler&& h)
	{
		return h();
	}
};

template <typename R, bool>
struct RvalInvoke_
{
	template <typename Handler, typename... Args>
	static inline R invoke(Handler&& h, Args&&... args)
	{
		return ApplyRval_<R, sizeof...(Args)>::append(TRY_MOVE(h), TRY_MOVE(args)...);
	}
};

template <typename R>
struct RvalInvoke_<R, true>
{
	template <typename F, typename C, typename... Args>
	static inline R invoke(F pf, C* obj, Args&&... args)
	{
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return RvalInvoke_<R, false>::invoke(wrap, TRY_MOVE(args)...);
	}
};

/*!
@brief 动态传入当前调用是否是右值
*/
template <typename R = void, typename Handler, typename Unknown, typename... Args>
inline R try_rval_invoke(Handler&& h, Unknown&& unkown, Args&&... args)
{
	return RvalInvoke_<R, CheckClassFunc_<Handler>::value>::invoke(TRY_MOVE(h), TRY_MOVE(unkown), TRY_MOVE(args)...);
}

template <typename R = void, typename Handler>
inline R try_rval_invoke(Handler&& h)
{
	static_assert(!CheckClassFunc_<Handler>::value, "");
	return h();
}

#endif