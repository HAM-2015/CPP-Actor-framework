#ifndef __WRAPPED_POST_HANDLER_H
#define __WRAPPED_POST_HANDLER_H

#include <boost/bind.hpp>

template <typename Poster, typename Handler>
class wrapped_post_handler
{
public:
	wrapped_post_handler(Poster* poster, const Handler& handler)
		: poster_(poster),
		handler_(handler)
	{
	}

	void operator()()
	{
		poster_->post(handler_);
	}

	void operator()() const
	{
		poster_->post(handler_);
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1)
	{
		poster_->post(boost::bind(handler_, arg1));
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1) const
	{
		poster_->post(boost::bind(handler_, arg1));
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2)
	{
		poster_->post(boost::bind(handler_, arg1, arg2));
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2) const
	{
		poster_->post(boost::bind(handler_, arg1, arg2));
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
	{
		poster_->post(boost::bind(handler_, arg1, arg2, arg3));
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
	{
		poster_->post(boost::bind(handler_, arg1, arg2, arg3));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4)
	{
		poster_->post(
			boost::bind(handler_, arg1, arg2, arg3, arg4));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4) const
	{
		poster_->post(
			boost::bind(handler_, arg1, arg2, arg3, arg4));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5)
	{
		poster_->post(
			boost::bind(handler_, arg1, arg2, arg3, arg4, arg5));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5) const
	{
		poster_->post(
			boost::bind(handler_, arg1, arg2, arg3, arg4, arg5));
	}

	//private:
	Poster* poster_;
	Handler handler_;
};

#endif