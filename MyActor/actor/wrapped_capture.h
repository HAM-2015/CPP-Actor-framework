#ifndef __WRAPPED_CAPTURE_H
#define __WRAPPED_CAPTURE_H

#include "tuple_option.h"
#include <atomic>

template <typename H, typename... ARGS>
struct wrapped_capture 
{
	template <typename Handler, typename... Args>
	wrapped_capture(bool pl, Handler&& h, Args&&... args)
		:_h(TRY_MOVE(h)), _args(TRY_MOVE(args)...) {}

	wrapped_capture(const wrapped_capture<H, ARGS...>& s)
		:_h(s._h), _args(s._args) {}

	wrapped_capture(wrapped_capture<H, ARGS...>&& s)
		:_h(std::move(s._h)), _args(std::move(s._args)) {}

	void operator =(const wrapped_capture<H, ARGS...>& s)
	{
		_h = s._h;
		_args = s._args;
	}

	void operator =(wrapped_capture<H, ARGS...>&& s)
	{
		_h = std::move(s._h);
		_args = std::move(s._args);
	}

	void operator ()()
	{
		tuple_invoke(_h, _args);
	}

	H _h;
	std::tuple<ARGS...> _args;
};

template <typename Handler, typename... Args>
wrapped_capture<RM_REF(Handler), RM_CREF(Args)...> wrap_capture(Handler&& h, Args&&... args)
{
	return wrapped_capture<RM_REF(Handler), RM_CREF(Args)...>(true, TRY_MOVE(h), TRY_MOVE(args)...);
}

#endif