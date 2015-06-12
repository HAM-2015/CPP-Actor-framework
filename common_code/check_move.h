#ifndef __CHECK_MOVE_H
#define __CHECK_MOVE_H

#include <type_traits>

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

#endif