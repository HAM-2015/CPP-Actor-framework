#ifndef __CHECK_MOVE_H
#define __CHECK_MOVE_H

#include <type_traits>
#include <assert.h>

template <typename T>
struct check_move
{
	static const bool is_rvalue = false;

	template <typename PT>
	static PT& move(PT& p0)
	{
		return p0;
	}
};

template <typename T>
struct check_move<T&&>
{
	static const bool is_rvalue = true;

	template <typename PT>
	static auto move(PT&& p0)->decltype(std::move(p0))
	{
		return std::move(p0);
	}
};

//检测当前参数是否是右值，是就执行std::move
#define CHECK_MOVE(__P__) check_move<decltype(__P__)>::move(__P__)
#define IS_RVALUE(__P__) check_move<decltype(__P__)>::is_rvalue;

//移除const和&
#define RM_CREF(__P__) std::remove_reference<std::remove_const<decltype(__P__)>::type>::type

/*!
@brief 对象拷贝链测试
*/
struct copy_chain_test
{
#ifdef _DEBUG
	copy_chain_test()
	:_copied(false) {}

	copy_chain_test(const copy_chain_test& s)
		: _copied(false)
	{
		assert(!s._copied);
		s._copied = true;
	}

	void operator =(const copy_chain_test& s)
	{
		assert(!_copied);
		assert(!s._copied);
		s._copied = true;
	}

	mutable bool _copied;
#endif
};

template <typename T0, typename T1 = void, typename T2 = void, typename T3 = void>
struct post_lambda
{
	template <typename TP0, typename TP1, typename TP2, typename TP3>
	post_lambda(TP0&& p0, TP1&& p1, TP2&& p2, TP3&& p3)
		:_p0(CHECK_MOVE(p0)), _p1(CHECK_MOVE(p1)), _p2(CHECK_MOVE(p2)), _p3(CHECK_MOVE(p3)) {}

	post_lambda(const post_lambda& s)
		:_p0(std::move((T0&)s._p0)), _p1(std::move((T1&)s._p1)), _p2(std::move((T2&)s._p2)), _p3(std::move((T3&)s._p3)), _ct(s._ct) {}

	T0 _p0;
	T1 _p1;
	T2 _p2;
	T3 _p3;
	copy_chain_test _ct;
};

template <typename T0, typename T1, typename T2>
struct post_lambda<T0, T1, T2, void>
{
	template <typename TP0, typename TP1, typename TP2>
	post_lambda(TP0&& p0, TP1&& p1, TP2&& p2)
		:_p0(CHECK_MOVE(p0)), _p1(CHECK_MOVE(p1)), _p2(CHECK_MOVE(p2)) {}

	post_lambda(const post_lambda& s)
		:_p0(std::move((T0&)s._p0)), _p1(std::move((T1&)s._p1)), _p2(std::move((T2&)s._p2)), _ct(s._ct) {}

	T0 _p0;
	T1 _p1;
	T2 _p2;
	copy_chain_test _ct;
};

template <typename T0, typename T1>
struct post_lambda<T0, T1, void, void>
{
	template <typename TP0, typename TP1>
	post_lambda(TP0&& p0, TP1&& p1)
		:_p0(CHECK_MOVE(p0)), _p1(CHECK_MOVE(p1)) {}

	post_lambda(const post_lambda& s)
		:_p0(std::move((T0&)s._p0)), _p1(std::move((T1&)s._p1)), _ct(s._ct) {}

	T0 _p0;
	T1 _p1;
	copy_chain_test _ct;
};

template <typename T0>
struct post_lambda<T0, void, void, void>
{
	post_lambda(T0&& p0)
		:_p0(std::move(p0)) {}

	post_lambda(const T0& p0)
		:_p0(p0) {}

	post_lambda(const post_lambda& s)
		:_p0(std::move((T0&)s._p0)), _ct(s._ct) {}

	T0 _p0;
	copy_chain_test _ct;
};

#endif