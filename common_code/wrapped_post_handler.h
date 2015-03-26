#ifndef __WRAPPED_POST_HANDLER_H
#define __WRAPPED_POST_HANDLER_H

template <typename Poster, typename Handler>
class wrapped_post_handler
{
public:
	wrapped_post_handler(Poster* poster, const Handler& handler)
		: _poster(poster),
		_handler(handler)
	{
	}

	void operator()()
	{
		_poster->post(_handler);
	}

	void operator()() const
	{
		_poster->post(_handler);
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1)
	{
		Handler& h_ = _handler;
		_poster->post([=](){h_((Arg1&)arg1); });
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1) const
	{
		Handler& h_ = _handler;
		_poster->post([=](){h_((Arg1&)arg1); });
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2)
	{
		Handler& h_ = _handler;
		_poster->post([=](){h_((Arg1&)arg1, (Arg2&)arg2); });
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2) const
	{
		Handler& h_ = _handler;
		_poster->post([=](){h_((Arg1&)arg1, (Arg2&)arg2); });
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
	{
		Handler& h_ = _handler;
		_poster->post([=](){h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3); });
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
	{
		Handler& h_ = _handler;
		_poster->post([=](){h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4)
	{
		Handler& h_ = _handler;
		_poster->post([=](){h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4) const
	{
		Handler& h_ = _handler;
		_poster->post([=](){h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5)
	{
			Handler& h_ = _handler;
			_poster->post([=](){h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4, (Arg5&)arg5); });
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5) const
	{
			Handler& h_ = _handler;
			_poster->post([=](){h_((Arg1&)arg1, (Arg2&)arg2, (Arg3&)arg3, (Arg4&)arg4, (Arg5&)arg5); });
	}

	Poster* _poster;
	Handler _handler;
};

#endif