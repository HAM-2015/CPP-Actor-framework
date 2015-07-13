#ifndef __STRAND_EX_H
#define __STRAND_EX_H

#ifdef ENABLE_STRAND_IMPL_POOL
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
	strand_ex(ios_proxy& ios);
	~strand_ex();

	boost::asio::io_service& get_io_service();
	bool running_in_this_thread() const;
	bool empty();

	template <typename Handler>
	void post(Handler&& handler)
	{
		_service.post(_impl, TRY_MOVE(handler));
	}
private:
	ios_proxy& _ios;
	boost::asio::detail::strand_service& _service;
	boost::asio::detail::strand_service::implementation_type _impl;
};
#endif

#endif