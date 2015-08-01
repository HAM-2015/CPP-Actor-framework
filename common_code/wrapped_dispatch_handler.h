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
		_handler(TRY_MOVE(handler))
	{
	}

	wrapped_dispatch_handler(wrapped_dispatch_handler&& sd)
		:_dispatcher(sd._dispatcher),
		_handler(std::move(sd._handler))
	{
	}

	wrapped_dispatch_handler& operator=(wrapped_dispatch_handler&& sd)
	{
		_dispatcher = sd._dispatcher;
		_handler = std::move(sd._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_dispatcher->dispatch(wrap_capture(_handler, TRY_MOVE(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_dispatcher->dispatch(wrap_capture(_handler, TRY_MOVE(args)...));
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
		_handler(TRY_MOVE(handler))
#ifdef _DEBUG
		, _checkOnce(new boost::atomic<bool>(false))
#endif
	{
	}

	wrapped_dispatch_handler(wrapped_dispatch_handler&& sd)
		:_dispatcher(sd._dispatcher),
		_handler(std::move(sd._handler))
#ifdef _DEBUG
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	wrapped_dispatch_handler& operator=(wrapped_dispatch_handler&& sd)
	{
		_dispatcher = sd._dispatcher;
		_handler = std::move(sd._handler);
#ifdef _DEBUG
		_checkOnce = sd._checkOnce;
#endif
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_dispatcher->dispatch(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Dispatcher* _dispatcher;
	Handler _handler;
#ifdef _DEBUG
	std::shared_ptr<boost::atomic<bool> > _checkOnce;
#endif
};

#endif