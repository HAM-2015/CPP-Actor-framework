#ifndef __STREAM_IO_BASE_H
#define __STREAM_IO_BASE_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/io_service.hpp>
#include <functional>
#include <memory>

/*!
@brief 流数据接口类
*/
class stream_io_base
{
protected:
	stream_io_base(boost::asio::io_service& ios)
		:_ios(ios) {}

	virtual ~stream_io_base() 
	{}
public:
	virtual void close() = 0;
	virtual void async_write(const unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h) = 0;
	virtual void async_read_some(unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h) = 0;
	virtual void async_read(unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h) = 0;
private:
	boost::asio::io_service& _ios;
};

#endif