#ifndef __WRAPPED_DISPATCH_HANDLER_H
#define __WRAPPED_DISPATCH_HANDLER_H

#include <boost/bind.hpp>

template <typename Dispatcher, typename Handler>
class wrapped_dispatch_handler
{
public:
	wrapped_dispatch_handler(Dispatcher* dispatcher, const Handler& handler)
		: dispatcher_(dispatcher),
		handler_(handler)
	{
	}

	void operator()()
	{
		dispatcher_->dispatch(handler_);
	}

	void operator()() const
	{
		dispatcher_->dispatch(handler_);
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1)
	{
		dispatcher_->dispatch(boost::BOOST_BIND(handler_, arg1));
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1) const
	{
		dispatcher_->dispatch(boost::BOOST_BIND(handler_, arg1));
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2)
	{
		dispatcher_->dispatch(boost::BOOST_BIND(handler_, arg1, arg2));
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2) const
	{
		dispatcher_->dispatch(boost::BOOST_BIND(handler_, arg1, arg2));
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
	{
		dispatcher_->dispatch(boost::BOOST_BIND(handler_, arg1, arg2, arg3));
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
	{
		dispatcher_->dispatch(boost::BOOST_BIND(handler_, arg1, arg2, arg3));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4)
	{
		dispatcher_->dispatch(
			boost::BOOST_BIND(handler_, arg1, arg2, arg3, arg4));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4) const
	{
		dispatcher_->dispatch(
			boost::BOOST_BIND(handler_, arg1, arg2, arg3, arg4));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5)
	{
		dispatcher_->dispatch(
			boost::BOOST_BIND(handler_, arg1, arg2, arg3, arg4, arg5));
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5) const
	{
		dispatcher_->dispatch(
			boost::BOOST_BIND(handler_, arg1, arg2, arg3, arg4, arg5));
	}

	//private:
	Dispatcher* dispatcher_;
	Handler handler_;
};

#endif