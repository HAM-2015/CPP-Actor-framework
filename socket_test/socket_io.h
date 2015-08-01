#ifndef __SOCKET_IO_H
#define __SOCKET_IO_H

#include "stream_io_base.h"
#include <boost/asio/ip/tcp.hpp>
#include <string>

class socket_io;
typedef std::shared_ptr<socket_io> socket_handle;

/*!
@brief tcp socket¶ÁÐ´
*/
class socket_io: public stream_io_base
{
private:
	socket_io(boost::asio::io_service& ios);
	~socket_io();
public:
	static socket_handle create(boost::asio::io_service& ios);
public:
	void close();
	const std::string ip();
	bool no_delay();
	void async_connect(const char* ip, size_t port, const std::function<void (const boost::system::error_code&)>& h);
	void async_write(const unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h);
	void async_read_some(unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h);
	void async_read(unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h);
	operator boost::asio::ip::tcp::socket& ();
private:
	std::string _ip;
	boost::asio::ip::tcp::socket _socket;
};

#endif