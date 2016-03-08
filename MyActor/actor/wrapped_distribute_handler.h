#ifndef __WRAPPED_DISTRIBUTE_HANDLER_H
#define __WRAPPED_DISTRIBUTE_HANDLER_H

#include "wrapped_capture.h"

template <typename Distributier, typename Handler, bool = false>
class wrapped_distribute_handler
{
public:
	template <typename H>
	wrapped_distribute_handler(Distributier* distributier, H&& handler)
		: _distributier(distributier),
		_handler(TRY_MOVE(handler))
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
		_distributier->distribute(wrap_capture(_handler, TRY_MOVE(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_distributier->distribute(wrap_capture(_handler, TRY_MOVE(args)...));
	}

	Distributier* _distributier;
	Handler _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Distributier, typename Handler>
class wrapped_distribute_handler<Distributier, Handler, true>
{
public:
	template <typename H>
	wrapped_distribute_handler(Distributier* distributier, H&& handler)
		: _distributier(distributier),
		_handler(TRY_MOVE(handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(new std::atomic<bool>(false))
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
		_distributier->distribute(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Distributier* _distributier;
	Handler _handler;
#if (_DEBUG || DEBUG)
	std::shared_ptr<std::atomic<bool> > _checkOnce;
#endif
};
//////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_POST_FRONT
template <typename Distributier, typename Handler, bool = false>
class wrapped_distribute_front_handler
{
public:
	template <typename H>
	wrapped_distribute_front_handler(Distributier* distributier, H&& handler)
		: _distributier(distributier),
		_handler(TRY_MOVE(handler))
	{
	}

	template <typename D, typename H>
	wrapped_distribute_front_handler(wrapped_distribute_front_handler<D, H>&& sd)
		: _distributier(sd._distributier),
		_handler(std::move(sd._handler))
	{
	}

	template <typename D, typename H>
	void operator=(wrapped_distribute_front_handler<D, H>&& sd)
	{
		_distributier = sd._distributier;
		_handler = std::move(sd._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_distributier->distribute_front(wrap_capture(_handler, TRY_MOVE(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_distributier->distribute_front(wrap_capture(_handler, TRY_MOVE(args)...));
	}

	Distributier* _distributier;
	Handler _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Distributier, typename Handler>
class wrapped_distribute_front_handler<Distributier, Handler, true>
{
public:
	template <typename H>
	wrapped_distribute_front_handler(Distributier* distributier, H&& handler)
		: _distributier(distributier),
		_handler(TRY_MOVE(handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(new std::atomic<bool>(false))
#endif
	{
	}

	template <typename D, typename H>
	wrapped_distribute_front_handler(wrapped_distribute_front_handler<D, H, true>&& sd)
		:_distributier(sd._distributier),
		_handler(std::move(sd._handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	template <typename D, typename H>
	void operator=(wrapped_distribute_front_handler<D, H, true>&& sd)
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
		_distributier->distribute_front(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Distributier* _distributier;
	Handler _handler;
#if (_DEBUG || DEBUG)
	std::shared_ptr<std::atomic<bool> > _checkOnce;
#endif
};
#endif
#endif