#ifndef __WRAPPED_DISPATCH_HANDLER_H
#define __WRAPPED_DISPATCH_HANDLER_H

#include "check_move.h"

template <typename Dispatcher, typename Handler>
class wrapped_dispatch_handler
{
public:
	template <typename H>
	wrapped_dispatch_handler(Dispatcher* dispatcher, H&& handler)
		: _dispatcher(dispatcher),
		_handler(CHECK_MOVE(handler))
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
	void operator()(const Arg1& arg1)
	{
		Handler& h_ = _handler;
		_dispatcher->dispatch([=]{h_((Arg1&)arg1); });
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1) const
	{
		Handler& h_ = _handler;
		_dispatcher->dispatch([=]{h_((Arg1&)arg1); });
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2)
	{
		Handler& h_ = _handler;
		_dispatcher->dispatch([=]{h_((Arg1&)arg1, (Arg2&)arg2); });
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2) const
	{
		Handler& h_ = _handler;
		_dispatcher->dispatch([=]{h_((Arg1&)arg1, (Arg2&)arg2); });
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
	{
		Handler& h_ = _handler;
		_dispatcher->dispatch([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3); });
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
	{
		Handler& h_ = _handler;
		_dispatcher->dispatch([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4)
	{
		Handler& h_ = _handler;
		_dispatcher->dispatch([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4) const
	{
		Handler& h_ = _handler;
		_dispatcher->dispatch([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5)
	{
			Handler& h_ = _handler;
			_dispatcher->dispatch([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4, (Arg5&)arg5); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5) const
	{
			Handler& h_ = _handler;
			_dispatcher->dispatch([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4, (Arg5&)arg5); });
	}

	//private:
	Dispatcher* _dispatcher;
	Handler _handler;
};

#endif