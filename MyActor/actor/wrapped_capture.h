#ifndef __WRAPPED_CAPTURE_H
#define __WRAPPED_CAPTURE_H

#include "tuple_option.h"
#include <atomic>
#include <memory>

template <typename Handler, typename... Args>
struct wrapped_capture 
{
	typedef RM_CREF(Handler) handler_type;

	wrapped_capture(bool pl, Handler& handler, Args&... args)
		:_handler(std::forward<Handler>(handler)), _args(std::forward<Args>(args)...) {}

	wrapped_capture(const wrapped_capture<Handler, Args...>& s)
		:_handler(s._handler), _args(s._args) {}

	wrapped_capture(wrapped_capture<Handler, Args...>&& s)
		:_handler(std::move(s._handler)), _args(std::move(s._args)) {}

	void operator =(const wrapped_capture<Handler, Args...>& s)
	{
		_handler = s._handler;
		_args = s._args;
	}

	void operator =(wrapped_capture<Handler, Args...>&& s)
	{
		_handler = std::move(s._handler);
		_args = std::move(s._args);
	}

	void operator ()()
	{
		tuple_invoke(_handler, _args);
	}

	handler_type _handler;
	std::tuple<RM_CREF(Args)...> _args;
};

template <typename Handler, typename... Args>
wrapped_capture<Handler, Args...> wrap_capture(Handler&& handler, Args&&... args)
{
	return wrapped_capture<Handler, Args...>(bool(), handler, args...);
}

#endif