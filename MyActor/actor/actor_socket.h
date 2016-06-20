#ifndef __ACTOR_SOCKET_H
#define __ACTOR_SOCKET_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include "my_actor.h"

class tcp_acceptor;

class tcp_socket
{
	friend tcp_acceptor;
public:
	tcp_socket(boost::asio::io_service& ios);
	~tcp_socket();
public:
	bool no_delay();
	std::string local_endpoint(unsigned short& port);
	std::string remote_endpoint(unsigned short& port);
	bool connect(my_actor* host, const char* ip, unsigned short port);
	bool read(my_actor* host, void* buff, size_t length);
	size_t read_some(my_actor* host, void* buff, size_t length);
	bool write(my_actor* host, const void* buff, size_t length);
	size_t write_some(my_actor* host, const void* buff, size_t length);
	bool timed_connect(my_actor* host, int tm, bool& timed, const char* ip, unsigned short port);
	bool timed_read(my_actor* host, int tm, bool& timed, void* buff, size_t length);
	size_t timed_read_some(my_actor* host, int tm, bool& timed, void* buff, size_t length);
	bool timed_write(my_actor* host, int tm, bool& timed, const void* buff, size_t length);
	size_t timed_write_some(my_actor* host, int tm, bool& timed, const void* buff, size_t length);
	bool close();

	template <typename Handler>
	void async_connect(const char* ip, unsigned short port, Handler&& handler)
	{
		_socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port), std::bind([](Handler& handler, const boost::system::error_code& ec)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1));
	}

	template <typename Handler>
	void async_read(void* buff, size_t length, Handler&& handler)
	{
		boost::asio::async_read(_socket, boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1, __2));
	}

	template <typename Handler>
	void async_read_some(void* buff, size_t length, Handler&& handler)
	{
		_socket.async_read_some(boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			handler(ec ? 0: s);
		}, TRY_MOVE(handler), __1, __2));
	}

	template <typename Handler>
	void async_write(const void* buff, size_t length, Handler&& handler)
	{
		boost::asio::async_write(_socket, boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1, __2));
	}

	template <typename Handler>
	void async_write_some(const void* buff, size_t length, Handler&& handler)
	{
		_socket.async_write_some(boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			handler(ec ? 0 : s);
		}, TRY_MOVE(handler), __1, __2));
	}
private:
	boost::asio::ip::tcp::socket _socket;
	NONE_COPY(tcp_socket);
};

class tcp_acceptor
{
public:
	tcp_acceptor(boost::asio::io_service& ios);
	~tcp_acceptor();
public:
	bool open(const char* ip, unsigned short port);
	bool open_v4(unsigned short port);
	bool open_v6(unsigned short port);
	bool close();
	bool accept(my_actor* host, tcp_socket& socket);
	bool timed_accept(my_actor* host, int tm, bool& timed, tcp_socket& socket);

	template <typename Handler>
	void async_accept(tcp_socket& socket, Handler&& handler)
	{
		_acceptor->async_accept(socket._socket, std::bind([](Handler& handler, const boost::system::error_code& ec)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1));
	}
private:
	boost::asio::io_service& _ios;
	stack_obj<boost::asio::ip::tcp::acceptor, false> _acceptor;
	bool _opend;
	NONE_COPY(tcp_acceptor);
};

class udp_socket
{
public:
	typedef boost::asio::ip::udp::endpoint remote_sender_endpoint;
public:
	udp_socket(boost::asio::io_service& ios);
	~udp_socket();
public:
	bool close();
	bool open_v4();
	bool open_v6();
	bool bind(const char* ip, unsigned short port);
	bool bind_v4(unsigned short port);
	bool bind_v6(unsigned short port);
	bool connect(my_actor* host, const char* remoteIp, unsigned short remotePort);
	bool send_to(my_actor* host, const remote_sender_endpoint& remoteEndpoint, const void* buff, size_t length);
	bool send(my_actor* host, const void* buff, size_t length);
	bool receive_from(my_actor* host, void* buff, size_t length);
	bool receive(my_actor* host, void* buff, size_t length);
	bool timed_connect(my_actor* host, int tm, bool& timed, const char* remoteIp, unsigned short remotePort);
	bool timed_send_to(my_actor* host, int tm, bool& timed, const remote_sender_endpoint& remoteEndpoint, const void* buff, size_t length);
	bool timed_send(my_actor* host, int tm, bool& timed, const void* buff, size_t length);
	bool timed_receive_from(my_actor* host, int tm, bool& timed, void* buff, size_t length);
	bool timed_receive(my_actor* host, int tm, bool& timed, void* buff, size_t length);
	const remote_sender_endpoint& last_remote_sender_endpoint();

	template <typename Handler>
	void async_connect(const char* remoteIp, unsigned short remotePort, Handler&& handler)
	{
		_socket.async_connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(remoteIp), remotePort), std::bind([](Handler& handler, const boost::system::error_code& ec)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1));
	}

	template <typename Handler>
	void async_send_to(const remote_sender_endpoint& remoteEndpoint, const void* buff, size_t length, Handler&& handler)
	{
		_socket.async_send_to(boost::asio::buffer(buff, length), remoteEndpoint, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1, __2));
 	}

	template <typename Handler>
	void async_send(const void* buff, size_t length, Handler&& handler)
	{
		_socket.async_send(boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1, __2));
	}

	template <typename Handler>
	void async_receive_from(void* buff, size_t length, Handler&& handler)
	{
		_socket.async_receive_from(boost::asio::buffer(buff, length), _remoteSenderEndpoint, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1, __2));
	}

	template <typename Handler>
	void async_receive(void* buff, size_t length, Handler&& handler)
	{
		_socket.async_receive(boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			handler(!ec);
		}, TRY_MOVE(handler), __1, __2));
	}
private:
	boost::asio::ip::udp::socket _socket;
	boost::asio::ip::udp::endpoint _remoteSenderEndpoint;
	NONE_COPY(udp_socket);
};

#endif