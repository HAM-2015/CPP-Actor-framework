#ifndef __STRAND_EX_H
#define __STRAND_EX_H

#include <boost/asio/detail/config.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/handler_type_requirements.hpp>
#include <boost/asio/detail/strand_service.hpp>
#include <boost/asio/detail/wrapped_handler.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/detail/push_options.hpp>
#include "ios_proxy.h"
#include "try_move.h"

/*!
@brief 修改标准boost strand，impl_改为独占
*/
class strand_ex
{
	friend ios_proxy;
public:
	strand_ex(ios_proxy& ios)
		: _ios(ios), _service(boost::asio::use_service<boost::asio::detail::strand_service>(ios))
	{
		_impl = (boost::asio::detail::strand_service::implementation_type)_ios.getImpl();
	}

	~strand_ex()
	{
		_ios.freeImpl(_impl);
	}

	boost::asio::io_service& get_io_service()
	{
		return _service.get_io_service();
	}

	template <typename Handler>
	void post(BOOST_ASIO_MOVE_ARG(Handler) handler)
	{
		_service.post(_impl, TRY_MOVE(handler));
	}

	bool running_in_this_thread() const
	{
		return _service.running_in_this_thread(_impl);
	}

private:
	ios_proxy& _ios;
	boost::asio::detail::strand_service& _service;
	boost::asio::detail::strand_service::implementation_type _impl;
};

#endif