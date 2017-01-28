#ifndef __WRAPPED_NEXT_TICK_HANDLER_H
#define __WRAPPED_NEXT_TICK_HANDLER_H

#include "wrapped_capture.h"

template <typename Poster, typename Handler, bool = false>
class wrapped_next_tick_handler
{
	typedef RM_CREF(Handler) handler_type;
public:
	wrapped_next_tick_handler(Poster* poster, Handler& handler)
		: _poster(poster),
		_handler(std::forward<Handler>(handler))
	{
	}

	template <typename P, typename H>
	wrapped_next_tick_handler(wrapped_next_tick_handler<P, H>&& sp)
		:_poster(sp._poster),
		_handler(std::move(sp._handler))
	{
	}

	template <typename P, typename H>
	void operator=(wrapped_next_tick_handler<P, H>&& sp)
	{
		_poster = sp._poster;
		_handler = std::move(sp._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_poster->next_tick(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_poster->next_tick(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	Poster* _poster;
	handler_type _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Poster, typename Handler>
class wrapped_next_tick_handler<Poster, Handler, true>
{
	typedef RM_CREF(Handler) handler_type;
public:
	wrapped_next_tick_handler(Poster* poster, Handler& handler)
		: _poster(poster),
		_handler(std::forward<Handler>(handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(new std::atomic<bool>(false))
#endif
	{
	}

	template <typename P, typename H>
	wrapped_next_tick_handler(wrapped_next_tick_handler<P, H, true>&& sd)
		:_poster(sd._poster),
		_handler(std::move(sd._handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	template <typename P, typename H>
	void operator=(wrapped_next_tick_handler<P, H, true>&& sd)
	{
		_poster = sd._poster;
		_handler = std::move(sd._handler);
#if (_DEBUG || DEBUG)
		_checkOnce = sd._checkOnce;
#endif
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(!_checkOnce->exchange(true));
		_poster->next_tick(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Poster* _poster;
	handler_type _handler;
#if (_DEBUG || DEBUG)
	std::shared_ptr<std::atomic<bool> > _checkOnce;
#endif
};

#endif