#ifndef __TUPLE_OPTION_H
#define __TUPLE_OPTION_H

#include <tuple>
#include "try_move.h"

template <typename... TYPES>
struct types_pck
{
	enum { number = sizeof...(TYPES) };
};

template <typename... TYPES>
struct pck_to_tuple {};

template <typename... TYPES>
struct pck_to_tuple<types_pck<TYPES...>>
{
	typedef std::tuple<TYPES...> tuple;
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 取第I个类型
*/
template <size_t I, typename FIRST, typename... TYPES>
struct single_element : public single_element<I - 1, TYPES...> {};

template <typename FIRST, typename... TYPES>
struct single_element<0, FIRST, TYPES...>
{
	typedef FIRST type;
};

template <size_t I, typename... TYPES>
struct single_element<I, types_pck<TYPES...>>: public single_element<I, TYPES...>{};

template <size_t I, typename... TYPES>
struct single_element<I, std::tuple<TYPES...>>: public single_element<I, TYPES...>{};

template <size_t I, typename... TYPES>
struct single_element<I, const std::tuple<TYPES...>>: public single_element<I, TYPES...>{};
//////////////////////////////////////////////////////////////////////////

enum cmp_result
{
	cmp_LT = 1,
	cmp_EQ = 2,
	cmp_GT = 4,
};

/*!
@brief 比较大小
*/
template <size_t A, size_t B>
struct cmp_size
{
	template <bool = true>
	struct LE
	{
		template <bool = true>
		struct eq
		{
			enum { res = cmp_EQ };
		};

		template <>
		struct eq<false>
		{
			enum { res = cmp_LT };
		};

		enum { res = eq<(A == B)>::res };
	};

	template <>
	struct LE<false>
	{
		enum { res = cmp_GT };
	};

	enum { result = LE<(A <= B)>::res };
};

template <size_t N, typename... TYPES>
struct TypesPckElement_
{
	template <size_t N, typename... TYPES>
	struct types {};

	template <size_t N, typename... Lefts, typename T, typename... Rights>
	struct types<N, types_pck<Lefts...>, T, Rights...> : public types<N - 1, types_pck<Lefts..., T>, Rights...>{};

	template <typename... Lefts, typename... Rights>
	struct types<0, types_pck<Lefts...>, Rights...>
	{
		typedef types_pck<Lefts...> left;
		typedef types_pck<Rights...> right;
	};

	typedef typename types<N, types_pck<>, TYPES...>::left left_pck;
	typedef typename types<sizeof...(TYPES)-N, types_pck<>, TYPES...>::right right_pck;
};

template <size_t N, typename... TYPES>
struct TypesPckElement_<N, types_pck<TYPES...>>: public TypesPckElement_<N, TYPES...>{};

/*!
@brief 取左边N个类型打包
*/
template <size_t N, typename... TYPES>
struct left_type
{
	typedef typename TypesPckElement_<N, TYPES...>::left_pck types;
};

template <size_t N, typename... TYPES>
struct left_type<N, types_pck<TYPES...>>: public left_type<N, TYPES...>{};

/*!
@brief 取右边N个类型打包
*/
template <size_t N, typename... TYPES>
struct right_type
{
	typedef typename TypesPckElement_<N, TYPES...>::right_pck types;
};

template <size_t N, typename... TYPES>
struct right_type<N, types_pck<TYPES...>>: public right_type<N, TYPES...>{};

/*!
@brief 取中间[S, S+L)个类型打包
*/
template <size_t S, size_t L, typename... TYPES>
struct middle_type
{
	typedef typename left_type<L, typename right_type<sizeof...(TYPES)-S, TYPES...>::types>::types types;
};

template <size_t S, size_t L, typename... TYPES>
struct middle_type<S, L, types_pck<TYPES...>>: public middle_type<S, L, TYPES...>{};

template <typename... PCKS>
struct merge_type 
{
	template <typename... TYPES>
	struct merge {};

	template <typename... TYPEL, typename... TYPER>
	struct merge<types_pck<TYPEL...>, types_pck<TYPER...>>
	{
		typedef types_pck<TYPEL..., TYPER...> types;
	};

	template <>
	struct merge<>
	{
		typedef types_pck<> types;
	};

	template <typename... PCKS>
	struct depth;

	template <typename Fst, typename...PCKS>
	struct depth<Fst, PCKS...> 
	{
		typedef typename merge<Fst, typename depth<PCKS...>::types>::types types;
	};

	template <>
	struct depth<>
	{
		typedef types_pck<> types;
	};

	typedef typename depth<PCKS...>::types types;
};

/*!
@brief 颠倒类型顺序后打包
*/
template <typename... TYPES>
struct reverse_type {};

template <typename HEAD, typename... TYPES>
struct reverse_type<HEAD, TYPES...>
{
	typedef typename merge_type<typename reverse_type<TYPES...>::types, types_pck<HEAD>>::types types;
};

template <>
struct reverse_type<>
{
	typedef types_pck<> types;
};

template <typename... TYPES>
struct reverse_type<types_pck<TYPES...>>:public reverse_type<TYPES...> {};

/*!
@brief 交换A,B类型顺序后打包
*/
template <size_t A, size_t B, typename... TYPES>
struct swap_type
{
	template <size_t A, size_t B, size_t = cmp_LT>
	struct lt
	{
		typedef typename merge_type<typename left_type<A, TYPES...>::types, types_pck<typename single_element<B, TYPES...>::type>>::types left;
		typedef typename merge_type<typename middle_type<A + 1, B - A - 1, TYPES...>::types, types_pck<typename single_element<A, TYPES...>::type>>::types middle;
		typedef typename right_type<sizeof...(TYPES)-B - 1, TYPES...>::types right;
		typedef typename merge_type<typename merge_type<left, middle>::types, right>::types types;
	};

	template <size_t A, size_t B>
	struct lt<A, B, cmp_GT> : public lt<B, A, cmp_LT>{};

	template <size_t A, size_t B>
	struct lt<A, B, cmp_EQ>
	{
		typedef types_pck<TYPES...> types;
	};

	typedef typename lt<A, B, cmp_size<A, B>::result>::types types;
};

template <size_t A, size_t B, typename... TYPES>
struct swap_type<A, B, types_pck<TYPES...>>:public swap_type<A, B, TYPES...>{};

/*!
@brief 在包中I位置插入新类型
*/
template <size_t I, typename NEW, typename... TYPES>
struct insert_type 
{
	typedef typename merge_type<typename left_type<I, TYPES...>::types, NEW, typename right_type<sizeof...(TYPES)-I, TYPES...>::types>::types types;
};

template <size_t I, typename NEW, typename... TYPES>
struct insert_type<I, NEW, types_pck<TYPES...>> : public insert_type<I, NEW, TYPES...>{};

/*!
@brief 删除包中[S, S+L)位置的类型
*/
template <size_t S, size_t L, typename... TYPES>
struct erase_type
{
	typedef typename merge_type<typename left_type<S, TYPES...>::types, typename right_type<sizeof...(TYPES)-S - L, TYPES...>::types>::types types;
};

template <size_t S, size_t L, typename... TYPES>
struct erase_type<S, L, types_pck<TYPES...>> : public erase_type<S, L, TYPES...>{};

/*!
@brief 展开所有类型包
*/
template <typename... PCKS>
struct solve_types
{
	template <typename... PCKS>
	struct solve {};

	template <typename Fst, typename... PCKS>
	struct solve<Fst, PCKS...>
	{
		typedef typename merge_type<typename solve<Fst>::types, typename solve<PCKS...>::types>::types types;
	};

	template <typename TYPE>
	struct solve<TYPE>
	{
		typedef types_pck<TYPE> types;
	};

	template <typename... TYPES>
	struct solve<types_pck<TYPES...>> : public solve<TYPES...>{};

	template <>
	struct solve<>
	{
		typedef types_pck<> types;
	};

	typedef typename solve<PCKS...>::types types;
};

//////////////////////////////////////////////////////////////////////////

/*!
@brief 常数包
*/
template <size_t... NUMS>
struct numbers_pck
{
	template <size_t... NUMS>
	struct calc_sum;

	template <size_t Fst, size_t... NUMS>
	struct calc_sum<Fst, NUMS...>
	{
		enum { sum = Fst + calc_sum<NUMS...>::sum };
	};

	template <>
	struct calc_sum<>
	{
		enum { sum = 0 };
	};

	enum { number = sizeof...(NUMS), sum = calc_sum<NUMS...>::sum };
};

/*!
@brief 使用常数包调用某个函数
*/
template <typename PCK>
struct number_invoke{};

template <size_t... NUMS>
struct number_invoke<numbers_pck<NUMS...>>
{
	template <typename R = void, typename Handler>
	static R invoke(Handler&& h)
	{
		return h(NUMS...);
	}
};

/*!
@brief 提取包中的某个数
*/
template <size_t I, typename PCK>
struct number_element
{
	template <typename PCK>
	struct solve;

	template <size_t... NUMS>
	struct solve<numbers_pck<NUMS...>>
	{
		template <size_t I, size_t... NUMS>
		struct depth;

		template <size_t I, size_t FST, size_t... NUMS>
		struct depth<I, FST, NUMS...> : public depth<I - 1, NUMS...>{};

		template <size_t FST, size_t... NUMS>
		struct depth<0, FST, NUMS...>
		{
			enum { num = FST };
		};

		enum { num = depth<I, NUMS...>::num };
	};

	enum { num = solve<PCK>::num };
};

/*!
@brief 合并多个包
*/
template <typename... PCKS>
struct merge_number
{
	template <typename PCKL, typename PCKR>
	struct merge {};

	template <size_t... NUMSL, size_t... NUMSR>
	struct merge<numbers_pck<NUMSL...>, numbers_pck<NUMSR...>>
	{
		typedef numbers_pck<NUMSL..., NUMSR...> result;
	};

	template <typename... PCKS>
	struct depth;

	template <typename Fst, typename... PCKS>
	struct depth<Fst, PCKS...>
	{
		typedef typename merge<Fst, typename depth<PCKS...>::result>::result result;
	};

	template <>
	struct depth<>
	{
		typedef numbers_pck<> result;
	};

	typedef typename depth<PCKS...>::result numbers;
};

template <size_t N, typename PCK>
struct NumberPckElement_
{
	template <typename PCK>
	struct solve;

	template <size_t... NUMS>
	struct solve<numbers_pck<NUMS...>>
	{
		template <size_t N, size_t... NUMS>
		struct left_depth;

		template <size_t N, size_t Fst, size_t... NUMS>
		struct left_depth<N, Fst, NUMS...>
		{
			typedef typename merge_number<numbers_pck<Fst>, typename left_depth<N - 1, NUMS...>::lefts>::numbers lefts;
			typedef typename left_depth<N - 1, NUMS...>::rights rights;
		};

		template <size_t... NUMS>
		struct left_depth<0, NUMS...>
		{
			typedef numbers_pck<> lefts;
			typedef numbers_pck<NUMS...> rights;
		};

		typedef typename left_depth<N, NUMS...>::lefts lefts;
		typedef typename left_depth<N, NUMS...>::rights rights;
	};

	typedef typename solve<PCK>::lefts lefts;
	typedef typename solve<PCK>::rights rights;
};

/*!
@brief 取包中右边N个数重新打包
*/
template <size_t N, typename PCK>
struct right_number
{
	typedef typename NumberPckElement_<PCK::number - N, PCK>::rights numbers;
};

/*!
@brief 取包中左边N个数重新打包
*/
template <size_t N, typename PCK>
struct left_number
{
	typedef typename NumberPckElement_<N, PCK>::lefts numbers;
};

/*!
@brief 取包中[S, S+L)数重新打包
*/
template <size_t S, size_t L, typename PCK>
struct middle_number
{
	typedef typename left_number<L, typename right_number<PCK::number - S, PCK>::numbers>::numbers numbers;
};

/*!
@brief 交互包中A,B数的位置
*/
template <size_t A, size_t B, typename PCK>
struct swap_number
{
	template <size_t A, size_t B, size_t = cmp_LT>
	struct lt
	{
		typedef typename left_number<A, PCK>::numbers lefts;
		typedef typename middle_number<A + 1, B - A - 1, PCK>::numbers middles;
		typedef typename right_number<PCK::number - B - 1, PCK>::numbers rights;

		typedef typename merge_number<lefts, numbers_pck<number_element<B, PCK>::num>>::numbers n1;
		typedef typename merge_number<n1, middles>::numbers n2;
		typedef typename merge_number<n2, numbers_pck<number_element<A, PCK>::num>>::numbers n3;
		typedef typename merge_number<n3, rights>::numbers numbers;
	};

	template <size_t A, size_t B>
	struct lt<A, B, cmp_GT> : public lt<B, A, cmp_LT>{};

	template <size_t A, size_t B>
	struct lt<A, B, cmp_EQ>
	{
		typedef PCK numbers;
	};

	typedef typename lt<A, B, cmp_size<A, B>::result>::numbers numbers;
};

/*!
@brief 颠倒顺序
*/
template <typename PCK>
struct reverse_number
{
	template <typename PCK>
	struct reverse {};

	template <size_t... NUMS>
	struct reverse<numbers_pck<NUMS...>>
	{
		template <size_t... NUMS>
		struct depth;

		template <size_t Fst, size_t... NUMS>
		struct depth<Fst, NUMS...>
		{
			typedef typename merge_number<typename depth<NUMS...>::result, numbers_pck<Fst>>::numbers result;
		};

		template <>
		struct depth<>
		{
			typedef numbers_pck<> result;
		};

		typedef typename depth<NUMS...>::result result;
	};

	typedef typename reverse<PCK>::result numbers;
};

/*!
@brief 在包中I位置插入一个新包
*/
template <size_t I, typename NEW, typename OLD>
struct insert_number
{
	typedef typename merge_number<typename left_number<I, OLD>::numbers, NEW, typename right_number<OLD::number - I, OLD>::numbers>::numbers numbers;
};

/*!
@brief 删除包中[S, S+L)数后重新打包
*/
template <size_t S, size_t L, typename PCK>
struct erase_number
{
	typedef typename merge_number<typename left_number<S, PCK>::numbers, typename right_number<PCK::number - S - L, PCK>::numbers>::numbers numbers;
};

/*!
@brief 把类型大小转为常数包
*/
template <typename... TYPES>
struct sizeof_types
{
	template <typename... TYPES>
	struct to_number;

	template <typename... TYPES>
	struct to_number<types_pck<TYPES...>>
	{
		typedef numbers_pck<sizeof(TYPES)...> numbers;
	};

	typedef typename to_number<typename solve_types<TYPES...>::types>::numbers numbers;
};

/*!
@brief 对包中的数进行排序后重新打包
*/
template <typename PCK>
struct sort_number
{
	template <size_t N, typename PCK>
	struct search_index
	{
		template <typename PCK>
		struct search {};

		template <size_t... NUMS>
		struct search<numbers_pck<NUMS...>>
		{
			template <size_t... NUMS>
			struct depth;

			template <size_t Fst, size_t... NUMS>
			struct depth<Fst, NUMS...>
			{
				template <bool = true>
				struct le
				{
					enum { n = 1 + sizeof...(NUMS) };
				};

				template <>
				struct le<false>
				{
					enum { n = depth<NUMS...>::n };
				};

				enum { n = le<(N <= Fst)>::n };
			};

			template <>
			struct depth<>
			{
				enum { n = 0 };
			};

			enum { i = sizeof...(NUMS)-depth<NUMS...>::n };
		};

		enum { i = search<PCK>::i };
	};

	template <typename PCK>
	struct sort{};

	template <size_t... NUMS>
	struct sort<numbers_pck<NUMS...>>
	{
		template <size_t... NUMS>
		struct depth;

		template <size_t Fst, size_t... NUMS>
		struct depth<Fst, NUMS...>
		{
			enum { i = search_index<Fst, typename depth<NUMS...>::numbers>::i };
			typedef typename insert_number<i, numbers_pck<Fst>, typename depth<NUMS...>::numbers>::numbers numbers;
			typedef typename insert_number<i, numbers_pck<PCK::number - sizeof...(NUMS)-1>, typename depth<NUMS...>::indexes>::numbers indexes;
		};

		template <>
		struct depth<>
		{
			typedef numbers_pck<> numbers;
			typedef numbers_pck<> indexes;
		};

		typedef typename depth<NUMS...>::numbers numbers;
		typedef typename depth<NUMS...>::indexes indexes;
	};

	typedef typename sort<PCK>::numbers positive_numbers;//正序
	typedef typename sort<PCK>::indexes positive_indexes;//正序索引
	typedef typename reverse_number<positive_numbers>::numbers reverse_numbers;//反序
	typedef typename reverse_number<positive_indexes>::numbers reverse_indexes;//反序索引
};
//////////////////////////////////////////////////////////////////////////

template <typename... TYPES>
struct SplitFunc_;

template <typename R, typename C, typename... Element>
struct SplitFunc_<R(C::*)(Element...)>
{
	typedef R result;
	typedef types_pck<Element...> params;
};

template <typename R, typename C, typename... Element>
struct SplitFunc_<R(C::*)(Element...) const>
{
	typedef R result;
	typedef types_pck<Element...> params;
};

template <typename R, typename... Element>
struct SplitFunc_<R(*)(Element...)>
{
	typedef R result;
	typedef types_pck<Element...> params;
};

template <typename R, typename... Element>
struct SplitFunc_<R(Element...)>
{
	typedef R result;
	typedef types_pck<Element...> params;
};

template <typename Func>
struct SplitFunc_<Func> : public SplitFunc_<decltype(&Func::operator())>{};

/*!
@brief 获取函数(仿函数)返回类型
*/
template <typename FUNC>
struct get_result_type
{
	typedef typename SplitFunc_<FUNC>::result type;
};

/*!
@brief 获取函数(仿函数)某个参数类型
*/
template <size_t I, typename FUNC>
struct get_param_type
{
	typedef typename SplitFunc_<FUNC>::params param_pck;
	typedef typename single_element<I, param_pck>::type type;
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 尝试移动tuple中的第I个参数
*/
template <size_t I, typename TUPLE>
struct tuple_move
{
	typedef typename single_element<I, RM_REF(TUPLE)>::type type;

	template <typename T>
	struct check_ref
	{
		typedef T& type;
	};

	template <typename T>
	struct check_ref<T&&>
	{
		typedef T&& type;
	};

	template <typename Tuple>
	static auto get(Tuple&& tup)->typename check_ref<type>::type
	{
		return (typename check_ref<type>::type)std::get<I>(tup);
	}
};

template <size_t I, typename TUPLE>
struct tuple_move<I, const TUPLE&>
{
	typedef typename single_element<I, RM_REF(TUPLE)>::type type;

	template <typename T>
	struct check_ref
	{
		typedef const T& type;
	};

	template <typename T>
	struct check_ref<T&&>
	{
		typedef const T& type;
	};

	template <typename T>
	struct check_ref<T&>
	{
		typedef T& type;
	};

	template <typename Tuple>
	static auto get(Tuple&& tup)->typename check_ref<type>::type
	{
		return (typename check_ref<type>::type)std::get<I>(tup);
	}
};

template <size_t I, typename TUPLE>
struct tuple_move<I, TUPLE&&>
{
	typedef typename single_element<I, RM_REF(TUPLE)>::type type;

	template <typename T>
	struct check_ref
	{
		typedef T&& type;
	};

	template <typename T>
	struct check_ref<T&>
	{
		typedef T& type;
	};

	template <typename Tuple>
	static auto get(Tuple&& tup)->typename check_ref<type>::type
	{
		return (typename check_ref<type>::type)std::get<I>(tup);
	}
};

template <size_t I, typename TUPLE>
struct tuple_move<I, const TUPLE&&>: public tuple_move<I, const TUPLE&>{};

template <typename R, size_t N>
struct ApplyArg_
{
	template <typename Handler, typename Tuple, typename... Args>
	static inline R append(Handler& h, Tuple&& tup, Args&&... args)
	{
		return ApplyArg_<R, N - 1>::append(h, TRY_MOVE(tup), tuple_move<N - 1, Tuple&&>::get(TRY_MOVE(tup)), TRY_MOVE(args)...);
	}

	template <typename Tuple, typename First, typename... Args>
	static inline void to_args(Tuple&& tup, First& fst, Args&... dsts)
	{
		fst = tuple_move<std::tuple_size<RM_REF(Tuple)>::value - N, Tuple&&>::get(TRY_MOVE(tup));
		ApplyArg_<R, N - 1>::to_args(TRY_MOVE(tup), dsts...);
	}

	template <typename Tuple, typename First, typename... Args>
	static inline void to_tuple(Tuple& dst, First&& fst, Args&&... args)
	{
		std::get<std::tuple_size<Tuple>::value - N>(dst) = TRY_MOVE(fst);
		ApplyArg_<R, N - 1>::to_tuple(dst, TRY_MOVE(args)...);
	}
};

template <typename R>
struct ApplyArg_<R, 0>
{
	template <typename Handler, typename Tuple, typename... Args>
	static inline R append(Handler& h, Tuple&&, Args&&... args)
	{
		return h(TRY_MOVE(args)...);
	}

	template <typename Tuple>
	static inline void to_args(Tuple&&)
	{

	}

	template <typename Tuple>
	static inline void to_tuple(Tuple&)
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

/*!
@brief 从tuple中取参数调用某个函数
*/
template <typename R = void, typename Handler>
inline R tuple_invoke(Handler&& h)
{
	return h();
}

template <typename R = void, typename Handler, typename First, typename... Tuple>
inline R tuple_invoke(Handler&& h, First&& fst, Tuple&&... tups)
{
	TupleInvokeWrap_<R, Handler, First&&> wrap = { h, fst };
	return tuple_invoke<R>(wrap, TRY_MOVE(tups)...);
}

template <typename R = void, typename F, typename C, typename... Tuple>
inline R tuple_invoke(F pf, C* obj, Tuple&&... tups)
{
	InvokeWrapObj_<R, F, C> wrap = { pf, obj };
	return tuple_invoke<R>(wrap, TRY_MOVE(tups)...);
}
//////////////////////////////////////////////////////////////////////////

/*!
@brief 比较数，取最大的值或最小值
*/
template <size_t A, size_t B = 0, size_t... NUMS>
struct static_cmp
{
	template <size_t A, size_t B>
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

template <size_t N>
struct static_cmp<N>
{
	enum { min = N, max = N };
};

/*!
@brief 比较类型大小，取最大的值或最小值
*/
template <typename... TYPES>
struct static_cmp_type_size: public static_cmp<sizeof(TYPES)...>
{
};

/*!
@brief 往tuple尾部添加新类型
*/
template <typename... TYPES>
struct tuple_append {};

/*!
@brief 往tuple头部添加新类型
*/
template <typename... TYPES>
struct tuple_append_front {};

template <typename... ARGS, typename... TYPES>
struct tuple_append<std::tuple<ARGS...>, TYPES...>
{
	typedef std::tuple<ARGS..., TYPES...> tuple;
};

template <typename... ARGS, typename... TYPES>
struct tuple_append<std::tuple<ARGS...>, types_pck<TYPES...>>: public tuple_append<std::tuple<ARGS...>, TYPES...>{};

template <typename... ARGS, typename... TYPES>
struct tuple_append_front<std::tuple<ARGS...>, TYPES...>
{
	typedef std::tuple<TYPES..., ARGS...> tuple;
};

template <typename... ARGS, typename... TYPES>
struct tuple_append_front<std::tuple<ARGS...>, types_pck<TYPES...>>: public tuple_append_front<std::tuple<ARGS...>, TYPES...>{};

/*!
@brief 取第I个参数
*/
template <size_t I>
struct args_element
{
	template <typename Head, typename... Args>
	static inline auto get(Head&&, Args&&... args)->typename single_element<I - 1, Args&&...>::type
	{
		return args_element<I - 1>::get(TRY_MOVE(args)...);
	}
};

template <>
struct args_element<0>
{
	template <typename Head, typename... Args>
	static inline auto get(Head&& head, Args&&...)->Head&&
	{
		return (Head&&)head;
	}
};

template <size_t N>
struct ReverseArgs_
{
	template <typename R, typename Handler, typename Tail>
	struct reverse_depth
	{
		template <typename... Args>
		inline R operator ()(Args&&... args)
		{
			return handler(TRY_MOVE(args)..., (Tail)tail);
		}

		Handler& handler;
		Tail& tail;
	};

	template <typename R, typename Handler, typename Head, typename... Args>
	static inline R reverse(Handler& h, Head&& head, Args&&... args)
	{
		reverse_depth<R, Handler, Head&&> rd = { h, head };
		return ReverseArgs_<N - 1>::reverse<R>(rd, TRY_MOVE(args)...);
	}
};

template <>
struct ReverseArgs_<0>
{
	template <typename R, typename Handler>
	static inline R reverse(Handler& h)
	{
		return h();
	}
};

/*!
@brief 颠倒参数前后顺序调用某个函数
*/
template <typename R = void, typename Handler, typename... Args>
inline R reverse_invoke(Handler&& h, Args&&... args)
{
	return ReverseArgs_<sizeof...(Args)>::reverse<R>(h, TRY_MOVE(args)...);
}

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

template <typename R = void, typename F, typename C, typename... Args>
inline R reverse_invoke(F pf, C* obj, Args&&... args)
{
	InvokeWrapObj_<R, F, C> wrap = { pf, obj };
	return ReverseArgs_<sizeof...(Args)>::reverse<R>(wrap, TRY_MOVE(args)...);
}

/*!
@brief 取右边N个参数调用某个函数
*/
template <size_t N>
struct right_invoke
{
	template <size_t I>
	struct right
	{
		template <typename R, typename Handler, typename Head, typename... Args>
		static inline R invoke(Handler& h, Head&& head, Args&&... args)
		{
			return right<I - 1>::invoke<R>(h, TRY_MOVE(args)...);
		}

	};

	template <>
	struct right<0>
	{
		template <typename R, typename Handler, typename... Args>
		static inline R invoke(Handler& h, Args&&... args)
		{
			return h(TRY_MOVE(args)...);
		}
	};

	template <size_t N, typename R, typename Handler>
	struct tuple_open
	{
		template <typename... Args>
		inline R operator ()(Args&&... args)
		{
			return right_invoke<N>::invoke<R>(h, TRY_MOVE(args)...);
		}

		Handler& h;
	};

	template <typename R = void, typename Handler, typename... Args>
	static inline R invoke(Handler&& h, Args&&... args)
	{
		static_assert(N <= sizeof...(Args), "");
		return right<sizeof...(Args)-N>::invoke<R>(h, TRY_MOVE(args)...);
	}

	template <typename R = void, typename F, typename C, typename... Args>
	static inline R invoke(F pf, C* obj, Args&&... args)
	{
		static_assert(N <= sizeof...(Args), "");
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return invoke<R>(wrap, TRY_MOVE(args)...);
	}

	template <typename R = void, typename Handler, typename Tuple>
	static inline R tupleInvoke(Handler&& h, Tuple&& tup)
	{
		tuple_open<N, R, Handler> wrap = { h };
		return tuple_invoke<R>(wrap, TRY_MOVE(tup));
	}

	template <typename R = void, typename F, typename C, typename Tuple>
	static inline R tupleInvoke(F pf, C* obj, Tuple&& tup)
	{
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return tupleInvoke<R>(wrap, TRY_MOVE(tup));
	}
};

/*!
@brief 取左边N个参数调用某个函数
*/
template <size_t N>
struct left_invoke
{
	template <typename R, typename Handler>
	struct reverse;

	template <typename R, typename Handler>
	struct left
	{
		template <typename... Args>
		inline R operator ()(Args&&... args)
		{
			return reverse_invoke<R>(h, TRY_MOVE(args)...);
		}

		Handler& h;
	};

	template <typename R, typename Handler>
	struct reverse
	{
		template <typename... Args>
		inline R operator ()(Args&&... args)
		{
			left<R, Handler> lh = { h };
			return right_invoke<N>::invoke<R>(lh, TRY_MOVE(args)...);
		}

		Handler& h;
	};

	template <size_t N, typename R, typename Handler>
	struct tuple_open
	{
		template <typename... Args>
		inline R operator ()(Args&&... args)
		{
			return left_invoke<N>::invoke<R>(h, TRY_MOVE(args)...);
		}

		Handler& h;
	};

	template <typename R = void, typename Handler, typename... Args>
	static inline R invoke(Handler&& h, Args&&... args)
	{
		static_assert(N <= sizeof...(Args), "");
		reverse<R, Handler> rh = { h };
		return reverse_invoke<R>(rh, TRY_MOVE(args)...);
	}

	template <typename R = void, typename F, typename C, typename... Args>
	static inline R invoke(F pf, C* obj, Args&&... args)
	{
		static_assert(N <= sizeof...(Args), "");
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return invoke<R>(wrap, TRY_MOVE(args)...);
	}

	template <typename R = void, typename Handler, typename Tuple>
	static inline R tupleInvoke(Handler&& h, Tuple&& tup)
	{
		tuple_open<N, R, Handler> wrap = { h };
		return tuple_invoke<R>(wrap, TRY_MOVE(tup));
	}

	template <typename R = void, typename F, typename C, typename Tuple>
	static inline R tupleInvoke(F pf, C* obj, Tuple&& tup)
	{
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return tupleInvoke<R>(wrap, TRY_MOVE(tup));
	}
};

/*!
@brief 取中间[S, S+L)参数调用某个函数
*/
template <size_t S, size_t L>
struct middle_invoke
{
	template <typename R, typename Handler>
	struct remove_left
	{
		template <typename... Args>
		inline R operator ()(Args&&... args)
		{
			return left_invoke<L>::invoke<R>(h, TRY_MOVE(args)...);
		}

		Handler& h;
	};

	template <size_t S, size_t L, typename R, typename Handler>
	struct tuple_open
	{
		template <typename... Args>
		inline R operator ()(Args&&... args)
		{
			return middle_invoke<S, L>::invoke<R>(h, TRY_MOVE(args)...);
		}

		Handler& h;
	};

	template <typename R = void, typename Handler, typename... Args>
	static inline R invoke(Handler&& h, Args&&... args)
	{
		static_assert(S + L <= sizeof...(Args), "");
		remove_left<R, Handler> wrap = { h };
		return right_invoke<sizeof...(Args) - S>::invoke<R>(wrap, TRY_MOVE(args)...);
	}

	template <typename R = void, typename F, typename C, typename... Args>
	static inline R invoke(F pf, C* obj, Args&&... args)
	{
		static_assert(S + L <= sizeof...(Args), "");
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return invoke<R>(wrap, TRY_MOVE(args)...);
	}

	template <typename R = void, typename Handler, typename Tuple>
	static inline R tupleInvoke(Handler&& h, Tuple&& tup)
	{
		tuple_open<S, L, R, Handler> wrap = { h };
		return tuple_invoke<R>(wrap, TRY_MOVE(tup));
	}

	template <typename R = void, typename F, typename C, typename Tuple>
	static inline R tupleInvoke(F pf, C* obj, Tuple&& tup)
	{
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return tupleInvoke<R>(wrap, TRY_MOVE(tup));
	}
};

template <size_t A, size_t B, bool = false>
struct SwapInvoke_
{
	template <typename... TYPES>
	struct type_tuple;

	template <typename... TYPES>
	struct type_tuple<types_pck<TYPES...>>
	{
		template <typename... Args>
		inline std::tuple<TYPES...> operator ()(Args&... args)
		{
			return std::tuple<TYPES...>(args...);
		}
	};

	template <typename R, typename Handler, typename... TYPES>
	struct type_tuple<R, Handler, types_pck<TYPES...>>
	{
		template <typename... Args>
		inline R operator ()(Args&... args)
		{
			return h((TYPES)args...);
		}

		Handler& h;
	};

	template <size_t A, size_t B, typename R, typename Handler>
	struct tuple_open
	{
		template <typename... Args>
		inline R operator ()(Args&&... args)
		{
			return swap_invoke<A, B>::invoke<R>(h, TRY_MOVE(args)...);
		}

		Handler& h;
	};

	template <typename R = void, typename Handler, typename... Args>
	static inline R invoke(Handler&& h, Args&&... args)
	{
		static_assert(A < sizeof...(Args) && B < sizeof...(Args), "");

		const size_t leftN = static_cmp<A, B>::min;
		const size_t rightN = static_cmp<A, B>::max;

		typedef typename left_type<leftN, Args&...>::types type_left;
		typedef typename middle_type<leftN + 1, rightN - leftN - 1, Args&...>::types type_middle;
		typedef typename right_type<sizeof...(Args)-rightN - 1, Args&...>::types type_right;

		auto& tuple_a = args_element<leftN>::get(args...);
		auto& tuple_b = args_element<rightN>::get(args...);
		auto tuple_left = left_invoke<leftN>::invoke<typename pck_to_tuple<type_left>::tuple>(type_tuple<type_left>(), args...);
 		auto tuple_middle = middle_invoke<leftN + 1, rightN - leftN - 1>::invoke<typename pck_to_tuple<type_middle>::tuple>(type_tuple<type_middle>(), args...);
		auto tuple_right = right_invoke<sizeof...(Args)-rightN - 1>::invoke<typename pck_to_tuple<type_right>::tuple>(type_tuple<type_right>(), args...);
		type_tuple<R, Handler, typename swap_type<leftN, rightN, Args&&...>::types> wrap = { h };
		return tuple_invoke<R>(wrap, tuple_left, tuple_b, tuple_middle, tuple_a, tuple_right);
	}

	template <typename R = void, typename F, typename C, typename... Args>
	static inline R invoke(F pf, C* obj, Args&&... args)
	{
		static_assert(A < sizeof...(Args) && B < sizeof...(Args), "");
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return invoke<R>(wrap, TRY_MOVE(args)...);
	}

	template <typename R = void, typename Handler, typename Tuple>
	static inline R tupleInvoke(Handler&& h, Tuple&& tup)
	{
		tuple_open<A, B, R, Handler> wrap = { h };
		return tuple_invoke<R>(wrap, TRY_MOVE(tup));
	}

	template <typename R = void, typename F, typename C, typename Tuple>
	static inline R tupleInvoke(F pf, C* obj, Tuple&& tup)
	{
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return tupleInvoke<R>(wrap, TRY_MOVE(tup));
	}
};

template <size_t A, size_t B>
struct SwapInvoke_<A, B, true>
{
	template <typename R = void, typename Handler, typename... Args>
	static inline R invoke(Handler&& h, Args&&... args)
	{
		return h(TRY_MOVE(args)...);
	}

	template <typename R = void, typename F, typename C, typename... Args>
	static inline R invoke(F pf, C* obj, Args&&... args)
	{
		return (obj->*pf)(TRY_MOVE(args)...);
	}

	template <typename R = void, typename Handler, typename Tuple>
	static inline R tupleInvoke(Handler&& h, Tuple&& tup)
	{
		return tuple_invoke<R>(h, TRY_MOVE(tup));
	}

	template <typename R = void, typename F, typename C, typename Tuple>
	static inline R tupleInvoke(F pf, C* obj, Tuple&& tup)
	{
		return tuple_invoke<R>(pf, obj, TRY_MOVE(tup));
	}
};

/*!
@brief 交换A,B参数调用某个函数
*/
template <size_t A, size_t B>
struct swap_invoke : public SwapInvoke_<A, B, A == B> {};

/*!
@brief 使用第I个参数调用
*/
template <size_t I>
struct index_invoke
{
	template <size_t N, typename R, typename Handler>
	struct depth
	{
		template <typename Head, typename... Args>
		inline R operator ()(Head&& head, Args&&... args)
		{
			depth<N - 1, R, Handler> dp = { h };
			return dp(TRY_MOVE(args)...);
		}

		Handler& h;
	};

	template <typename R, typename Handler>
	struct depth<0, R, Handler>
	{
		template <typename Head, typename... Args>
		inline R operator ()(Head&& head, Args&&... args)
		{
			return h(TRY_MOVE(head));
		}

		Handler& h;
	};

	template <typename R = void, typename Handler, typename... Args>
	static inline R invoke(Handler&& h, Args&&... args)
	{
		static_assert(I < sizeof...(Args), "");
		depth<I, R, Handler> dp = { h };
		return dp(TRY_MOVE(args)...);
	}

	template <typename R = void, typename F, typename C, typename... Args>
	static inline R invoke(F pf, C* obj, Args&&... args)
	{
		static_assert(I < sizeof...(Args), "");
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return invoke<R>(wrap, TRY_MOVE(args)...);
	}

	template <typename R = void, typename Handler, typename Tuple>
	static inline R tupleInvoke(Handler&& h, Tuple&& tup)
	{
		depth<I, R, Handler> dp = { h };
		return tuple_invoke<R>(dp, TRY_MOVE(tup));
	}

	template <typename R = void, typename F, typename C, typename Tuple>
	static inline R tupleInvoke(F pf, C* obj, Tuple&& tup)
	{
		InvokeWrapObj_<R, F, C> wrap = { pf, obj };
		return tupleInvoke<R>(wrap, TRY_MOVE(tup));
	}
};

/*!
@brief 把tuple移到零散参数中
*/
template <typename Tuple, typename... Args>
inline void tuple_to_args(Tuple&& tup, Args&... dsts)
{
	static_assert(std::tuple_size<RM_REF(Tuple)>::value == sizeof...(Args), "src size != dst size");
	ApplyArg_<void, std::tuple_size<RM_REF(Tuple)>::value>::to_args(TRY_MOVE(tup), dsts...);
}

/*!
@brief 把零散参数移到tuple中
*/
template <typename Tuple, typename... Args>
inline void args_to_tuple(Tuple& dst, Args&&... args)
{
	static_assert(std::tuple_size<RM_REF(Tuple)>::value == sizeof...(Args), "src size != dst size");
	ApplyArg_<void, std::tuple_size<RM_REF(Tuple)>::value>::to_tuple(dst, TRY_MOVE(args)...);
}

/*!
@brief 取第i个参数（所有参数类型一样）
*/
template <typename First>
inline typename First& args_get(const size_t i, First& fst)
{
	assert(0 == i);
	return fst;
}

template <typename First, typename... Args>
inline typename First& args_get(const size_t i, First& fst, Args&... args)
{
	if (i)
	{
		return args_get(i - 1, args...);
	} 
	else
	{
		return fst;
	}
}

#endif