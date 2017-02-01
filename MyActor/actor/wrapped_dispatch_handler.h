#ifndef __WRAPPED_DISPATCH_HANDLER_H
#define __WRAPPED_DISPATCH_HANDLER_H

#include "wrapped_capture.h"

template <typename Dispatcher, typename Handler, bool = false>
class wrapped_dispatch_handler
{
	typedef RM_CREF(Handler) handler_type;
public:
	wrapped_dispatch_handler(Dispatcher* dispatcher, Handler& handler)
		: _dispatcher(dispatcher),
		_handler(std::forward<Handler>(handler))
	{
	}

	template <typename D, typename H>
	wrapped_dispatch_handler(wrapped_dispatch_handler<D, H>&& sd)
		:_dispatcher(sd._dispatcher),
		_handler(std::move(sd._handler))
	{
	}

	template <typename D, typename H>
	void operator=(wrapped_dispatch_handler<D, H>&& sd)
	{
		_dispatcher = sd._dispatcher;
		_handler = std::move(sd._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_dispatcher->dispatch(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	void operator()()
	{
		_dispatcher->dispatch(_handler);
	}

	Dispatcher* _dispatcher;
	handler_type _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Dispatcher, typename Handler>
class wrapped_dispatch_handler<Dispatcher, Handler, true>
{
	typedef RM_CREF(Handler) handler_type;
public:
	wrapped_dispatch_handler(Dispatcher* dispatcher, Handler& handler)
		: _dispatcher(dispatcher),
		_handler(std::forward<Handler>(handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(std::make_shared<std::atomic<bool>>(false))
#endif
	{
	}

	template <typename D, typename H>
	wrapped_dispatch_handler(wrapped_dispatch_handler<D, H, true>&& sd)
		:_dispatcher(sd._dispatcher),
		_handler(std::move(sd._handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	template <typename D, typename H>
	void operator=(wrapped_dispatch_handler<D, H, true>&& sd)
	{
		_dispatcher = sd._dispatcher;
		_handler = std::move(sd._handler);
#if (_DEBUG || DEBUG)
		_checkOnce = sd._checkOnce;
#endif
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), std::forward<Args>(args)...));
	}

	void operator()()
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(std::move(_handler));
	}

	Dispatcher* _dispatcher;
	handler_type _handler;
#if (_DEBUG || DEBUG)
	std::shared_ptr<std::atomic<bool> > _checkOnce;
#endif
};
#endif