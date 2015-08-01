#ifndef __FUNCTION_TYPE_H
#define __FUNCTION_TYPE_H

#include <functional>

template <typename... ARGS>
struct func_type
{
	enum { number = sizeof...(ARGS) };
	typedef std::function<void(ARGS...)> result;
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