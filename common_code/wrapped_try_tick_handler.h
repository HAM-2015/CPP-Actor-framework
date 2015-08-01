#ifndef __WRAPPED_TRY_TICK_HANDLER_H
#define __WRAPPED_TRY_TICK_HANDLER_H

#include "wrapped_capture.h"

template <typename Poster, typename Handler, bool = false>
class wrapped_try_tick_handler
{
public:
	template <typename H>
	wrapped_try_tick_handler(Poster* poster, H&& handler)
		: _poster(poster),
		_handler(TRY_MOVE(handler))
	{
	}

	wrapped_try_tick_handler(wrapped_try_tick_handler&& sp)
		:_poster(sp._poster),
		_handler(std::move(sp._handler))
	{
	}

	wrapped_try_tick_handler& operator=(wrapped_try_tick_handler&& sp)
	{
		_poster = sp._poster;
		_handler = std::move(sp._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_poster->try_tick(wrap_capture(_handler, TRY_MOVE(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_poster->try_tick(wrap_capture(_handler, TRY_MOVE(args)...));
	}

	Poster* _poster;
	Handler _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Poster, typename Handler>
class wrapped_try_tick_handler<Poster, Handler, true>
{
public:
	template <typename H>
	wrapped_try_tick_handler(Poster* poster, H&& handler)
		: _poster(poster),
		_handler(TRY_MOVE(handler))
#ifdef _DEBUG
		, _checkOnce(new boost::atomic<bool>(false))
#endif
	{
	}

	wrapped_try_tick_handler(wrapped_try_tick_handler&& sd)
		:_poster(sd._poster),
		_handler(std::move(sd._handler))
#ifdef _DEBUG
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	wrapped_try_tick_handler& operator=(wrapped_try_tick_handler&& sd)
	{
		_poster = sd._poster;
		_handler = std::move(sd._handler);
#ifdef _DEBUG
		_checkOnce = sd._checkOnce;
#endif
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_poster->try_tick(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Poster* _poster;
	Handler _handler;
#ifdef _DEBUG
	std::shared_ptr<boost::atomic<bool> > _checkOnce;
#endif
};

#endif