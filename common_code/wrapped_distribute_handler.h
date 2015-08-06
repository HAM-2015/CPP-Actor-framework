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

	wrapped_distribute_handler(wrapped_distribute_handler&& sd)
		:_distributier(sd._distributier),
		_handler(std::move(sd._handler))
	{
	}

	wrapped_distribute_handler& operator=(wrapped_distribute_handler&& sd)
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
#ifdef _DEBUG
		, _checkOnce(new boost::atomic<bool>(false))
#endif
	{
	}

	wrapped_distribute_handler(wrapped_distribute_handler&& sd)
		:_distributier(sd._distributier),
		_handler(std::move(sd._handler))
#ifdef _DEBUG
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	wrapped_distribute_handler& operator=(wrapped_distribute_handler&& sd)
	{
		_distributier = sd._distributier;
		_handler = std::move(sd._handler);
#ifdef _DEBUG
		_checkOnce = sd._checkOnce;
#endif
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_distributier->distribute(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Distributier* _distributier;
	Handler _handler;
#ifdef _DEBUG
	std::shared_ptr<boost::atomic<bool> > _checkOnce;
#endif
};

#endif