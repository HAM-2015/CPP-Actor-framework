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

	void operator()()
	{
		_dispatcher->dispatch(_handler);
	}

	void operator()() const
	{
		_dispatcher->dispatch(_handler);
	}

	template <typename Arg1>
	void operator()(Arg1&& arg1)
	{
		_dispatcher->dispatch(wrap_capture(_handler, arg1));
	}

	template <typename Arg1>
	void operator()(Arg1&& arg1) const
	{
		_dispatcher->dispatch(wrap_capture(_handler, arg1));
	}

	template <typename Arg1, typename Arg2>
	void operator()(Arg1&& arg1, Arg2&& arg2)
	{
		_dispatcher->dispatch(wrap_capture(_handler, arg1, arg2));
	}

	template <typename Arg1, typename Arg2>
	void operator()(Arg1&& arg1, Arg2&& arg2) const
	{
		_dispatcher->dispatch(wrap_capture(_handler, arg1, arg2));
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
	{
		_dispatcher->dispatch(wrap_capture(_handler, arg1, arg2, arg3));
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3) const
	{
		_dispatcher->dispatch(wrap_capture(_handler, arg1, arg2, arg3));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
	{
		_dispatcher->dispatch(wrap_capture(_handler, arg1, arg2, arg3, arg4));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4) const
	{
		_dispatcher->dispatch(wrap_capture(_handler, arg1, arg2, arg3, arg4));
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

	void operator()()
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(_handler);
	}

	void operator()() const
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(_handler);
	}

	template <typename Arg1>
	void operator()(Arg1&& arg1)
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), FORCE_MOVE(arg1)));
	}

	template <typename Arg1>
	void operator()(Arg1&& arg1) const
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), FORCE_MOVE(arg1)));
	}

	template <typename Arg1, typename Arg2>
	void operator()(Arg1&& arg1, Arg2&& arg2)
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), FORCE_MOVE(arg1), FORCE_MOVE(arg2)));
	}

	template <typename Arg1, typename Arg2>
	void operator()(Arg1&& arg1, Arg2&& arg2) const
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), FORCE_MOVE(arg1), FORCE_MOVE(arg2)));
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), FORCE_MOVE(arg1), FORCE_MOVE(arg2), FORCE_MOVE(arg3)));
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3) const
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), FORCE_MOVE(arg1), FORCE_MOVE(arg2), FORCE_MOVE(arg3)));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), FORCE_MOVE(arg1), FORCE_MOVE(arg2), FORCE_MOVE(arg3), FORCE_MOVE(arg4)));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4) const
	{
		assert(!_checkOnce->exchange(true));
		_dispatcher->dispatch(wrap_capture(std::move(_handler), FORCE_MOVE(arg1), FORCE_MOVE(arg2), FORCE_MOVE(arg3), FORCE_MOVE(arg4)));
	}

	Dispatcher* _dispatcher;
	Handler _handler;
#ifdef _DEBUG
	std::shared_ptr<boost::atomic<bool> > _checkOnce;
#endif
};

#endif