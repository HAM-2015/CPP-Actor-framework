#ifndef __WRAPPED_NEXT_TICK_HANDLER_H
#define __WRAPPED_NEXT_TICK_HANDLER_H

#include "try_move.h"

template <typename Poster, typename Handler>
class wrapped_next_tick_handler
{
public:
	template <typename H>
	wrapped_next_tick_handler(Poster* poster, H&& handler)
		: _poster(poster),
		_handler(TRY_MOVE(handler))
	{
	}

	wrapped_next_tick_handler(wrapped_next_tick_handler&& sp)
		:_poster(sp._poster),
		_handler(std::move(sp._handler))
	{
	}

	wrapped_next_tick_handler& operator=(wrapped_next_tick_handler&& sp)
	{
		_poster = sp._poster;
		_handler = std::move(sp._handler);
	}

	void operator()()
	{
		_poster->next_tick(_handler);
	}

	void operator()() const
	{
		_poster->next_tick(_handler);
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1)
	{
		Handler& h_ = _handler;
		_poster->next_tick([=]{h_((Arg1&)arg1); });
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1) const
	{
		Handler& h_ = _handler;
		_poster->next_tick([=]{h_((Arg1&)arg1); });
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2)
	{
		Handler& h_ = _handler;
		_poster->next_tick([=]{h_((Arg1&)arg1, (Arg2&)arg2); });
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2) const
	{
		Handler& h_ = _handler;
		_poster->next_tick([=]{h_((Arg1&)arg1, (Arg2&)arg2); });
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
	{
		Handler& h_ = _handler;
		_poster->next_tick([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3); });
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
	{
		Handler& h_ = _handler;
		_poster->next_tick([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4)
	{
		Handler& h_ = _handler;
		_poster->next_tick([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4) const
	{
		Handler& h_ = _handler;
		_poster->next_tick([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5)
	{
			Handler& h_ = _handler;
			_poster->next_tick([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4, (Arg5&)arg5); });
		}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5) const
	{
			Handler& h_ = _handler;
			_poster->next_tick([=]{h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4, (Arg5&)arg5); });
		}

	Poster* _poster;
	Handler _handler;
};

#endif