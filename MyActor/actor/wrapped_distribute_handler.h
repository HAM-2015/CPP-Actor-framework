#ifndef __WRAPPED_DISTRIBUTE_HANDLER_H
#define __WRAPPED_DISTRIBUTE_HANDLER_H

#include "wrapped_capture.h"

template <typename Distributier, typename Handler, bool = false>
class wrapped_distribute_handler
{
	typedef RM_CREF(Handler) handler_type;
public:
	wrapped_distribute_handler(Distributier* distributier, Handler& handler)
		: _distributier(distributier),
		_handler(std::forward<Handler>(handler))
	{
	}

	template <typename D, typename H>
	wrapped_distribute_handler(wrapped_distribute_handler<D, H>&& sd)
		:_distributier(sd._distributier),
		_handler(std::move(sd._handler))
	{
	}

	template <typename D, typename H>
	void operator=(wrapped_distribute_handler<D, H>&& sd)
	{
		_distributier = sd._distributier;
		_handler = std::move(sd._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_distributier->distribute(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	void operator()()
	{
		_distributier->distribute(_handler);
	}

	Distributier* _distributier;
	handler_type _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Distributier, typename Handler>
class wrapped_distribute_handler<Distributier, Handler, true>
{
	typedef RM_CREF(Handler) handler_type;
public:
	wrapped_distribute_handler(Distributier* distributier, Handler& handler)
		: _distributier(distributier),
		_handler(std::forward<Handler>(handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(std::make_shared<std::atomic<bool>>(false))
#endif
	{
	}

	template <typename D, typename H>
	wrapped_distribute_handler(wrapped_distribute_handler<D, H, true>&& sd)
		:_distributier(sd._distributier),
		_handler(std::move(sd._handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	template <typename D, typename H>
	void operator=(wrapped_distribute_handler<D, H, true>&& sd)
	{
		_distributier = sd._distributier;
		_handler = std::move(sd._handler);
#if (_DEBUG || DEBUG)
		_checkOnce = sd._checkOnce;
#endif
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(!_checkOnce->exchange(true));
		_distributier->distribute(wrap_capture(std::move(_handler), std::forward<Args>(args)...));
	}

	void operator()()
	{
		assert(!_checkOnce->exchange(true));
		_distributier->distribute(std::move(_handler));
	}

	Distributier* _distributier;
	handler_type _handler;
#if (_DEBUG || DEBUG)
	std::shared_ptr<std::atomic<bool> > _checkOnce;
#endif
};
#endif