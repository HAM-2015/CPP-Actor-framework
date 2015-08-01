#include "socket_io.h"
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>

socket_io::socket_io( boost::asio::io_service& ios )
	: stream_io_base(ios), _socket(ios)
{

}

socket_io::~socket_io()
{
	close();
}

socket_handle socket_io::create(boost::asio::io_service& ios)
{
	return socket_handle(new socket_io(ios), [](socket_io* p){delete p; });
}

void socket_io::close()
{
	boost::system::error_code ec;
	_socket.close(ec);
}

bool socket_io::no_delay()
{
	boost::system::error_code ec;
	_socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);
	return !ec;
}

const std::string socket_io::ip()
{
	if (_ip.empty())
	{
		try
		{
			_ip = _socket.remote_endpoint().address().to_string();
		}
		catch (...)
		{

		}
	} 
	return _ip;
}

void socket_io::async_connect( const char* ip, size_t port, const std::function<void (const boost::system::error_code&)>& h )
{
	auto endPoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(ip), (unsigned short)port);
	_socket.async_connect(endPoint, h);
}

void socket_io::async_write( const unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h )
{
	boost::asio::async_write(_socket, boost::asio::buffer(buff, length), h);
}

void socket_io::async_read_some( unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h )
{
	_socket.async_read_some(boost::asio::buffer(buff, length), h);
}

void socket_io::async_read( unsigned char* buff, size_t length, const std::function<void (const boost::system::error_code&, size_t)>& h )
{
	boost::asio::async_read(_socket, boost::asio::buffer(buff, length), h);
}

socket_io::operator boost::asio::ip::tcp::socket&()
{
	return _socket;
}