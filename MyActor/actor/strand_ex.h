#ifndef __STRAND_EX_H
#define __STRAND_EX_H

#include <algorithm>
#include <boost/asio/detail/strand_service.hpp>
#include "try_move.h"

class io_engine;
class boost_strand;

/*!
@brief 修改标准boost strand，impl_改为独占
*/
class StrandEx_
{
	friend boost_strand;
private:
	StrandEx_(io_engine& ios);
	~StrandEx_();

	bool running_in_this_thread() const;
	bool empty() const;
	bool ready_empty() const;
	bool waiting_empty() const;
	bool running() const;
	bool safe_running() const;
	bool only_self() const;

	template <typename Handler>
	void post(Handler& handler)
	{
		boost::asio::detail::async_result_init<Handler&, void()> init(handler);
		_service.post(_impl, init.handler);
	}

	template <typename Handler>
	void dispatch(Handler& handler)
	{
		boost::asio::detail::async_result_init<Handler&, void()> init(handler);
		_service.dispatch(_impl, init.handler);
	}

	template <typename Handler>
	void post(Handler&& handler)
	{
		_service.post(_impl, handler);
	}

	template <typename Handler>
	void dispatch(Handler&& handler)
	{
		_service.dispatch(_impl, handler);
	}
private:
	boost::asio::detail::strand_service& _service;
	boost::asio::detail::strand_service::implementation_type _impl;
};

#endif