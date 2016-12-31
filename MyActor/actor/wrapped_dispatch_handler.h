#ifndef __WRAPPED_DISPATCH_HANDLER_H
#define __WRAPPED_DISPATCH_HANDLER_H

#include "wrapped_capture.h"

template <typename Dispatcher, typename Handler, bool = false>
class wrapped_dispatch_handler
{
public:
	template <typename H>
	wrapped_dispatch_handler(Dispatcher* dispatcher, H&& handler)
		: _dispatcher(dispatcher),
		_handler(std::forward<H>(handler))
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

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_dispatcher->dispatch(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	Dispatcher* _dispatcher;
	Handler _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Dispatcher, typename Handler>
class wrapped_dispatch_handler<Dispatcher, Handler, true>
{
public:
	template <typename H>
	wrapped_dispatch_handler(Dispatcher* dispatcher, H&& handler)
		: _dispatcher(dispatcher),
		_handler(std::forward<H>(handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(new std::atomic<bool>(false))
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
		_dispatcher->dispatch(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Dispatcher* _dispatcher;
	Handler _handler;
#if (_DEBUG || DEBUG)
	std::shared_ptr<std::atomic<bool> > _checkOnce;
#endif
};
#endif