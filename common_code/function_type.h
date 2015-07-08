#ifndef __FUNCTION_TYPE_H
#define __FUNCTION_TYPE_H

#include <functional>

template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
struct func_type
{
	enum { number = 4 };
	typedef std::function<void(T0, T1, T2, T3)> result;
};

template <typename T0, typename T1, typename T2>
struct func_type<T0, T1, T2, void>
{
	enum { number = 3 };
	typedef std::function<void(T0, T1, T2)> result;
};

template <typename T0, typename T1>
struct func_type<T0, T1, void, void>
{
	enum { number = 2 };
	typedef std::function<void(T0, T1)> result;
};

template <typename T0>
struct func_type<T0, void, void, void>
{
	enum { number = 1 };
	typedef std::function<void(T0)> result;
};

template <>
struct func_type<void, void, void, void>
{
	enum { number = 0 };
	typedef std::function<void()> result;
};

namespace ph
{
	static decltype(std::placeholders::_1) _1;
	static decltype(std::placeholders::_2) _2;
	static decltype(std::placeholders::_3) _3;
	static decltype(std::placeholders::_4) _4;
	static decltype(std::placeholders::_5) _5;
	static decltype(std::placeholders::_6) _6;
	static decltype(std::placeholders::_7) _7;
	static decltype(std::placeholders::_8) _8;
	static decltype(std::placeholders::_9) _9;
}

#endif