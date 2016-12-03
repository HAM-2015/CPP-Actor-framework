#ifndef __WRAPPED_POST_HANDLER_H
#define __WRAPPED_POST_HANDLER_H

#include <boost/asio/io_service.hpp>
#include "wrapped_capture.h"

template <typename Poster, typename Handler, bool = false>
class wrapped_post_handler
{
public:
	template <typename H>
	wrapped_post_handler(Poster* poster, H&& handler)
		: _poster(poster),
		_handler(std::forward<H>(handler))
	{
	}

	template <typename P, typename H>
	wrapped_post_handler(wrapped_post_handler<P, H>&& sp)
		:_poster(sp._poster),
		_handler(std::move(sp._handler))
	{
	}

	template <typename P, typename H>
	void operator=(wrapped_post_handler<P, H>&& sp)
	{
		_poster = sp._poster;
		_handler = std::move(sp._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_poster->post(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_poster->post(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	Poster* _poster;
	Handler _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Poster, typename Handler>
class wrapped_post_handler<Poster, Handler, true>
{
public:
	template <typename H>
	wrapped_post_handler(Poster* poster, H&& handler)
		: _poster(poster),
		_handler(std::forward<H>(handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(new std::atomic<bool>(false))
#endif
	{
	}

	template <typename P, typename H>
	wrapped_post_handler(wrapped_post_handler<P, H, true>&& sd)
		:_poster(sd._poster),
		_handler(std::move(sd._handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	template <typename P, typename H>
	void operator=(wrapped_post_handler<P, H, true>&& sd)
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
		_poster->post(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Poster* _poster;
	Handler _handler;
#if (_DEBUG || DEBUG)
	std::shared_ptr<std::atomic<bool> > _checkOnce;
#endif
};
//////////////////////////////////////////////////////////////////////////

template <typename Poster, typename Handler>
class wrapped_hold_work_post_handler
{
public:
	template <typename H>
	wrapped_hold_work_post_handler(boost::asio::io_service& ios, Poster* poster, H&& handler)
		: _holdWork(ios), _poster(poster),
		_handler(std::forward<H>(handler))
	{
	}

	template <typename P, typename H>
	wrapped_hold_work_post_handler(wrapped_hold_work_post_handler<P, H>&& sp)
		: _holdWork(sp._holdWork), _poster(sp._poster),
		_handler(std::move(sp._handler))
	{
	}

	template <typename P, typename H>
	void operator=(wrapped_hold_work_post_handler<P, H>&& sp)
	{
		_holdWork = sp._holdWork;
		_poster = sp._poster;
		_handler = std::move(sp._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_poster->post(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_poster->post(wrap_capture(_handler, std::forward<Args>(args)...));
	}

private:
	boost::asio::io_service::work _holdWork;
	Poster* _poster;
	Handler _handler;
};
//////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_POST_FRONT
template <typename Poster, typename Handler, bool = false>
class wrapped_post_front_handler
{
public:
	template <typename H>
	wrapped_post_front_handler(Poster* poster, H&& handler)
		: _poster(poster),
		_handler(std::forward<H>(handler))
	{
	}

	template <typename P, typename H>
	wrapped_post_front_handler(wrapped_post_front_handler<P, H>&& sp)
		: _poster(sp._poster),
		_handler(std::move(sp._handler))
	{
	}

	template <typename P, typename H>
	void operator=(wrapped_post_front_handler<P, H>&& sp)
	{
		_poster = sp._poster;
		_handler = std::move(sp._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_poster->post_front(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_poster->post_front(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	Poster* _poster;
	Handler _handler;
};
//////////////////////////////////////////////////////////////////////////

template <typename Poster, typename Handler>
class wrapped_post_front_handler<Poster, Handler, true>
{
public:
	template <typename H>
	wrapped_post_front_handler(Poster* poster, H&& handler)
		: _poster(poster),
		_handler(std::forward<H>(handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(new std::atomic<bool>(false))
#endif
	{
	}

	template <typename P, typename H>
	wrapped_post_front_handler(wrapped_post_front_handler<P, H, true>&& sd)
		:_poster(sd._poster),
		_handler(std::move(sd._handler))
#if (_DEBUG || DEBUG)
		, _checkOnce(sd._checkOnce)
#endif
	{
	}

	template <typename P, typename H>
	void operator=(wrapped_post_front_handler<P, H, true>&& sd)
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
		_poster->post_front(wrap_capture(FORCE_MOVE(_handler), FORCE_MOVE(args)...));
	}

	Poster* _poster;
	Handler _handler;
#if (_DEBUG || DEBUG)
	std::shared_ptr<std::atomic<bool> > _checkOnce;
#endif
};
//////////////////////////////////////////////////////////////////////////

template <typename Poster, typename Handler>
class wrapped_hold_work_post_front_handler
{
public:
	template <typename H>
	wrapped_hold_work_post_front_handler(boost::asio::io_service& ios, Poster* poster, H&& handler)
		: _holdWork(ios), _poster(poster),
		_handler(std::forward<H>(handler))
	{
	}

	template <typename P, typename H>
	wrapped_hold_work_post_front_handler(wrapped_hold_work_post_front_handler<P, H>&& sp)
		: _holdWork(sp._holdWork), _poster(sp._poster),
		_handler(std::move(sp._handler))
	{
	}

	template <typename P, typename H>
	void operator=(wrapped_hold_work_post_front_handler<P, H>&& sp)
	{
		_holdWork = sp._holdWork;
		_poster = sp._poster;
		_handler = std::move(sp._handler);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_poster->post_front(wrap_capture(_handler, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		_poster->post_front(wrap_capture(_handler, std::forward<Args>(args)...));
	}

private:
	boost::asio::io_service::work _holdWork;
	Poster* _poster;
	Handler _handler;
};
#endif
#endif