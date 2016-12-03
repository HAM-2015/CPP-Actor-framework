#ifndef __ACTOR_SOCKET_H
#define __ACTOR_SOCKET_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include "my_actor.h"

/*!
@brief tcp通信
*/
class tcp_socket
{
public:
	struct result
	{
		size_t s;///<字节数
		bool ok;///<是否成功
	};
public:
	tcp_socket(boost::asio::io_service& ios);
	~tcp_socket();
public:
	/*!
	@brief 设置no_delay属性
	*/
	bool no_delay();

	/*!
	@brief 本地端口IP
	*/
	std::string local_endpoint(unsigned short& port);

	/*!
	@brief 远端端口IP
	*/
	std::string remote_endpoint(unsigned short& port);

	/*!
	@brief 获取boost socket对象
	*/
	boost::asio::ip::tcp::socket& boost_socket();

	/*!
	@brief 客户端模式下连接远端服务器
	*/
	bool connect(my_actor* host, const char* remoteIp, unsigned short remotePort);

	/*!
	@brief 往buff缓冲区内读取数据，直到读满
	*/
	result read(my_actor* host, void* buff, size_t length);

	/*!
	@brief 往buff缓冲区内读取数据，有多少读多少
	*/
	result read_some(my_actor* host, void* buff, size_t length);

	/*!
	@brief 将buff缓冲区内的数据全部发送出去
	*/
	result write(my_actor* host, const void* buff, size_t length);

	/*!
	@brief 将buff缓冲区内的数据发送出去，能发多少是多少
	*/
	result write_some(my_actor* host, const void* buff, size_t length);

	/*!
	@brief 在ms时间范围内，客户端模式下连接远端服务器
	*/
	bool timed_connect(my_actor* host, int ms, bool& overtime, const char* remoteIp, unsigned short remotePort);

	/*!
	@brief 在ms时间范围内，往buff缓冲区内读取数据，直到读满
	*/
	result timed_read(my_actor* host, int ms, bool& overtime, void* buff, size_t length);

	/*!
	@brief 在ms时间范围内，往buff缓冲区内读取数据，有多少读多少
	*/
	result timed_read_some(my_actor* host, int ms, bool& overtime, void* buff, size_t length);

	/*!
	@brief 在ms时间范围内，将buff缓冲区内的数据全部发送出去
	*/
	result timed_write(my_actor* host, int ms, bool& overtime, const void* buff, size_t length);

	/*!
	@brief 在ms时间范围内，将buff缓冲区内的数据发送出去，能发多少是多少
	*/
	result timed_write_some(my_actor* host, int ms, bool& overtime, const void* buff, size_t length);

	/*!
	@brief 关闭socket
	*/
	bool close();

	/*!
	@brief 异步模式下，客户端模式下连接远端服务器
	*/
	template <typename Handler>
	void async_connect(const char* remoteIp, unsigned short remotePort, Handler&& handler)
	{
		_socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(remoteIp), remotePort), std::bind([](Handler& handler, const boost::system::error_code& ec)
		{
			handler(!ec);
		}, std::forward<Handler>(handler), __1));
	}

	/*!
	@brief 异步模式下，往buff缓冲区内读取数据，直到读满
	*/
	template <typename Handler>
	void async_read(void* buff, size_t length, Handler&& handler)
	{
		boost::asio::async_read(_socket, boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
	}

	/*!
	@brief 异步模式下，往buff缓冲区内读取数据，有多少读多少
	*/
	template <typename Handler>
	void async_read_some(void* buff, size_t length, Handler&& handler)
	{
		_socket.async_read_some(boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
	}

	/*!
	@brief 异步模式下，将buff缓冲区内的数据全部发送出去
	*/
	template <typename Handler>
	void async_write(const void* buff, size_t length, Handler&& handler)
	{
		boost::asio::async_write(_socket, boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
	}

	/*!
	@brief 异步模式下，将buff缓冲区内的数据发送出去，能发多少是多少
	*/
	template <typename Handler>
	void async_write_some(const void* buff, size_t length, Handler&& handler)
	{
		_socket.async_write_some(boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
	}
private:
	boost::asio::ip::tcp::socket _socket;
	NONE_COPY(tcp_socket);
};

/*!
@brief 服务器侦听器
*/
class tcp_acceptor
{
public:
	tcp_acceptor(boost::asio::io_service& ios);
	~tcp_acceptor();
public:
	/*!
	@brief 只允许特定ip连接该服务器
	*/
	bool open(const char* ip, unsigned short port);

	/*!
	@brief ip v4下打开服务器
	*/
	bool open_v4(unsigned short port);

	/*!
	@brief ip v6下打开服务器
	*/
	bool open_v6(unsigned short port);

	/*!
	@brief 关闭侦听器
	*/
	bool close();

	/*!
	@brief 获取boost acceptor对象
	*/
	boost::asio::ip::tcp::acceptor& boost_acceptor();

	/*!
	@brief 用socket侦听客户端连接
	*/
	bool accept(my_actor* host, tcp_socket& socket);

	/*!
	@brief 在ms时间范围内，用socket侦听客户端连接
	*/
	bool timed_accept(my_actor* host, int ms, bool& overtime, tcp_socket& socket);

	/*!
	@brief 异步模式下，用socket侦听客户端连接
	*/
	template <typename Handler>
	void async_accept(tcp_socket& socket, Handler&& handler)
	{
		_acceptor->async_accept(socket.boost_socket(), std::bind([](Handler& handler, const boost::system::error_code& ec)
		{
			handler(!ec);
		}, std::forward<Handler>(handler), __1));
	}
private:
	boost::asio::io_service& _ios;
	stack_obj<boost::asio::ip::tcp::acceptor, false> _acceptor;
	bool _opend;
	NONE_COPY(tcp_acceptor);
};

/*!
@brief udp通信
*/
class udp_socket
{
public:
	typedef boost::asio::ip::udp::endpoint remote_sender_endpoint;

	struct result
	{
		size_t s;///<字节数
		bool ok;///<是否成功
	};
public:
	udp_socket(boost::asio::io_service& ios);
	~udp_socket();
public:
	/*!
	@brief 关闭socket
	*/
	bool close();

	/*!
	@brief ip v4模式打开socket
	*/
	bool open_v4();

	/*!
	@brief ip v6模式打开socket
	*/
	bool open_v6();

	/*!
	@brief 绑定本地一个ip下某个端口接收发送数据
	*/
	bool bind(const char* ip, unsigned short port);

	/*!
	@brief 绑定ip v4下某个端口接收发送数据
	*/
	bool bind_v4(unsigned short port);

	/*!
	@brief 绑定ip v6下某个端口接收发送数据
	*/
	bool bind_v6(unsigned short port);

	/*!
	@brief 打开并绑定ip v4下某个端口接收发送数据
	*/
	bool open_bind_v4(unsigned short port);

	/*!
	@brief 打开并绑定ip v6下某个端口接收发送数据
	*/
	bool open_bind_v6(unsigned short port);

	/*!
	@brief 获取boost socket对象
	*/
	boost::asio::ip::udp::socket& boost_socket();

	/*!
	@brief 设定一个远程端口作为默认发送接收目标
	*/
	bool connect(my_actor* host, const char* remoteIp, unsigned short remotePort);
	bool sync_connect(const char* remoteIp, unsigned short remotePort);

	/*!
	@brief 发送buff缓冲区数据到指定目标
	*/
	result send_to(my_actor* host, const remote_sender_endpoint& remoteEndpoint, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 发送buff缓冲区数据到指定目标
	*/
	result send_to(my_actor* host, const char* remoteIp, unsigned short remotePort, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 发送buff缓冲区数据到默认目标(connect成功后)
	*/
	result send(my_actor* host, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 接收远端发送的数据到buff缓冲区，并记录下远端地址
	*/
	result receive_from(my_actor* host, void* buff, size_t length, int flags = 0);

	/*!
	@brief 接收远端发送的数据到buff缓冲区
	*/
	result receive(my_actor* host, void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，发送端设定一个远程端口作为默认发送接收目标
	*/
	bool timed_connect(my_actor* host, int ms, bool& overtime, const char* remoteIp, unsigned short remotePort);

	/*!
	@brief 在ms时间范围内，发送buff缓冲区数据到指定目标
	*/
	result timed_send_to(my_actor* host, int ms, bool& overtime, const remote_sender_endpoint& remoteEndpoint, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，发送buff缓冲区数据到指定目标
	*/
	result timed_send_to(my_actor* host, int ms, bool& overtime, const char* remoteIp, unsigned short remotePort, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，发送buff缓冲区数据到默认目标(connect成功后)
	*/
	result timed_send(my_actor* host, int ms, bool& overtime, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，接收远端发送的数据到buff缓冲区，并记录下远端地址
	*/
	result timed_receive_from(my_actor* host, int ms, bool& overtime, void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，接收远端发送的数据到buff缓冲区
	*/
	result timed_receive(my_actor* host, int ms, bool& overtime, void* buff, size_t length, int flags = 0);

	/*!
	@brief 创建一个远端目标
	*/
	static remote_sender_endpoint make_endpoint(const char* remoteIp, unsigned short remotePort);

	/*!
	@brief receive_from完成后获取远端地址
	*/
	const remote_sender_endpoint& last_remote_sender_endpoint();

	/*!
	@brief 重置远端地址
	*/
	void reset_remote_sender_endpoint();

	/*!
	@brief 异步模式下，发送端设定一个远程端口作为默认发送接收目标
	*/
	template <typename Handler>
	void async_connect(const char* remoteIp, unsigned short remotePort, Handler&& handler)
	{
		_socket.async_connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(remoteIp), remotePort), std::bind([](Handler& handler, const boost::system::error_code& ec)
		{
			handler(!ec);
		}, std::forward<Handler>(handler), __1));
	}

	/*!
	@brief 异步模式下，发送buff缓冲区数据到指定目标
	*/
	template <typename Handler>
	void async_send_to(const remote_sender_endpoint& remoteEndpoint, const void* buff, size_t length, Handler&& handler, int flags = 0)
	{
		_socket.async_send_to(boost::asio::buffer(buff, length), remoteEndpoint, flags, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
	}

	/*!
	@brief 异步模式下，发送buff缓冲区数据到指定目标
	*/
	template <typename Handler>
	void async_send_to(const char* remoteIp, unsigned short remotePort, const void* buff, size_t length, Handler&& handler, int flags = 0)
	{
		async_send_to(remote_sender_endpoint(boost::asio::ip::address::from_string(remoteIp), remotePort), buff, length, std::forward<Handler>(handler), flags);
	}

	/*!
	@brief 异步模式下，发送buff缓冲区数据到默认目标(connect成功后)
	*/
	template <typename Handler>
	void async_send(const void* buff, size_t length, Handler&& handler, int flags = 0)
	{
		_socket.async_send(boost::asio::buffer(buff, length), flags, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
	}

	/*!
	@brief 异步模式下，接收远端发送的数据到buff缓冲区，并记录下远端地址
	*/
	template <typename Handler>
	void async_receive_from(void* buff, size_t length, Handler&& handler, int flags = 0)
	{
		_socket.async_receive_from(boost::asio::buffer(buff, length), _remoteSenderEndpoint, flags, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
	}

	/*!
	@brief 异步模式下，接收远端发送的数据到buff缓冲区
	*/
	template <typename Handler>
	void async_receive(void* buff, size_t length, Handler&& handler, int flags = 0)
	{
		_socket.async_receive(boost::asio::buffer(buff, length), flags, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
	}
private:
	boost::asio::ip::udp::socket _socket;
	boost::asio::ip::udp::endpoint _remoteSenderEndpoint;
	NONE_COPY(udp_socket);
};

#endif