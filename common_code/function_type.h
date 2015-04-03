#ifndef __FUNCTION_TYPE_H
#define __FUNCTION_TYPE_H

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

#endif