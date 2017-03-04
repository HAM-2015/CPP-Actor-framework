#ifndef __ACTOR_SOCKET_H
#define __ACTOR_SOCKET_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include "my_actor.h"

class tcp_acceptor;
/*!
@brief tcp通信
*/
class tcp_socket
{
	friend tcp_acceptor;
public:
	struct result
	{
		size_t s;///<字节数/缓存数
		int code;///<错误码
		bool ok;///<是否成功
	};
private:
#ifdef ENABLE_ASIO_PRE_OP
#ifdef HAS_ASIO_HANDLER_IS_TRIED
	template <typename Handler>
	struct async_read_op
	{
		typedef RM_CREF(Handler) handler_type;

		async_read_op(Handler& handler, boost::asio::ip::tcp::socket& sck, void* buff, size_t currBytes, size_t totalBytes)
			:_handler(std::forward<Handler>(handler)), _sck(sck), _buffer(buff), _currBytes(currBytes), _totalBytes(totalBytes) {}

		void operator()(const boost::system::error_code& ec, size_t s)
		{
			_currBytes += s;
			if (ec || _totalBytes == _currBytes)
			{
				tcp_socket::result res = { _currBytes, ec.value(), !ec };
				_handler(res);
			}
			else
			{
				_sck.async_read_some(boost::asio::buffer((char*)_buffer + _currBytes, _totalBytes - _currBytes), std::move(*this));
			}
		}

		friend bool asio_handler_is_tried(async_read_op*)
		{
			return true;
		}

		handler_type _handler;
		boost::asio::ip::tcp::socket& _sck;
		void* const _buffer;
		size_t _currBytes;
		const size_t _totalBytes;
		COPY_CONSTRUCT5(async_read_op, _handler, _sck, _buffer, _currBytes, _totalBytes);
	};

	template <typename Handler>
	struct async_write_op
	{
		typedef RM_CREF(Handler) handler_type;

		async_write_op(Handler& handler, boost::asio::ip::tcp::socket& sck, const void* buff, size_t currBytes, size_t totalBytes)
			:_handler(std::forward<Handler>(handler)), _sck(sck), _buffer(buff), _currBytes(currBytes), _totalBytes(totalBytes) {}

		void operator()(const boost::system::error_code& ec, size_t s)
		{
			_currBytes += s;
			if (ec || _totalBytes == _currBytes)
			{
				tcp_socket::result res = { _currBytes, ec.value(), !ec };
				_handler(res);
			}
			else
			{
				_sck.async_write_some(boost::asio::buffer((const char*)_buffer + _currBytes, _totalBytes - _currBytes), std::move(*this));
			}
		}

		friend bool asio_handler_is_tried(async_write_op*)
		{
			return true;
		}

		handler_type _handler;
		boost::asio::ip::tcp::socket& _sck;
		const void* const _buffer;
		size_t _currBytes;
		const size_t _totalBytes;
		COPY_CONSTRUCT5(async_write_op, _handler, _sck, _buffer, _currBytes, _totalBytes);
	};
#endif
#endif

#ifdef HAS_ASIO_SEND_FILE
#ifdef __linux__
	template <typename Handler>
	struct async_send_file_op
	{
		typedef RM_CREF(Handler) handler_type;

		async_send_file_op(Handler& handler, tcp_socket& sck, size_t currBytes)
			:_handler(std::forward<Handler>(handler)), _sck(sck), _currBytes(currBytes) {}

		void operator()(const boost::system::error_code& ec, size_t s)
		{
			_currBytes += s;
			_sck._sendFileState.count -= s;
			if (ec || !_sck._sendFileState.count || !s)
			{
				_sck._sendFileState.offset = NULL;
				_sck._sendFileState.count = 0;
				_sck._sendFileState.fd = 0;
				tcp_socket::result res = { _currBytes, ec.value(), !ec };
				_handler(res);
			}
			else
			{
				_sck._socket.async_write_some(boost::asio::buffer((const char*)&_sck._sendFileState, -1), std::move(*this));
			}
		}

#ifdef HAS_ASIO_HANDLER_IS_TRIED
		friend bool asio_handler_is_tried(async_send_file_op*)
		{
			return true;
		}
#endif

		handler_type _handler;
		tcp_socket& _sck;
		size_t _currBytes;
		COPY_CONSTRUCT3(async_send_file_op, _handler, _sck, _currBytes);
	};
#endif
#endif

public:
	tcp_socket(io_engine& ios);
	~tcp_socket();
public:
	/*!
	@brief 设置no_delay属性
	*/
	result no_delay();

	/*!
	@brief linux下优化异步返回（如果有数据，在async_xxx操作中直接回调）
	*/
	void pre_option();

	/*!
	@brief
	*/
	bool is_pre_option();

	/*!
	@brief 本地端口IP
	*/
	std::string local_endpoint(unsigned short& port);

	/*!
	@brief 远端端口IP
	*/
	std::string remote_endpoint(unsigned short& port);

	/*!
	@brief 创建一个远端目标
	*/
	static boost::asio::ip::tcp::endpoint make_endpoint(const char* remoteIp, unsigned short remotePort);

	/*!
	@brief 客户端模式下连接远端服务器
	*/
	result connect(my_actor* host, const boost::asio::ip::tcp::endpoint& remoteEndpoint);
	result connect(my_actor* host, const char* remoteIp, unsigned short remotePort);

	/*!
	@brief 往缓冲区内读取数据，直到读满
	*/
	result read(my_actor* host, void* buff, size_t length);

	/*!
	@brief 往缓冲区内读取数据，有多少读多少
	*/
	result read_some(my_actor* host, void* buff, size_t length);

	/*!
	@brief 将数据全部发送出去
	*/
	result write(my_actor* host, const void* buff, size_t length);

	/*!
	@brief 将数据发送出去，能发多少是多少
	*/
	result write_some(my_actor* host, const void* buff, size_t length);

	/*!
	@brief 在ms时间范围内，客户端模式下连接远端服务器
	*/
	result timed_connect(my_actor* host, int ms, const boost::asio::ip::tcp::endpoint& remoteEndpoint);
	result timed_connect(my_actor* host, int ms, const char* remoteIp, unsigned short remotePort);

	/*!
	@brief 在ms时间范围内，往缓冲区内读取数据，直到读满
	*/
	result timed_read(my_actor* host, int ms, void* buff, size_t length);

	/*!
	@brief 在ms时间范围内，往缓冲区内读取数据，有多少读多少
	*/
	result timed_read_some(my_actor* host, int ms, void* buff, size_t length);

	/*!
	@brief 在ms时间范围内，将数据全部发送出去
	*/
	result timed_write(my_actor* host, int ms, const void* buff, size_t length);

	/*!
	@brief 在ms时间范围内，将数据发送出去，能发多少是多少
	*/
	result timed_write_some(my_actor* host, int ms, const void* buff, size_t length);

	/*!
	@brief 关闭socket
	*/
	result close();

	/*!
	@brief 用原始句柄构造
	*/
	result assign(boost::asio::detail::socket_type sckFd);
	result assign_v6(boost::asio::detail::socket_type sckFd);

	/*!
	@brief 异步模式下，客户端模式下连接远端服务器
	*/
	template <typename Handler>
	bool async_connect(const boost::asio::ip::tcp::endpoint& remoteEndpoint, Handler&& handler)
	{
		_socket.async_connect(remoteEndpoint, std::bind([this](Handler& handler, const boost::system::error_code& ec)
		{
			if (!ec)
			{
				set_internal_non_blocking();
			}
			result res = { 0, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1));
		return false;
	}

	template <typename Handler>
	bool async_connect(const char* remoteIp, unsigned short remotePort, Handler&& handler)
	{
		return async_connect(make_endpoint(remoteIp, remotePort), std::forward<Handler>(handler));
	}

	/*!
	@brief 异步模式下，往缓冲区内读取数据，直到读满
	*/
	template <typename Handler>
	bool async_read(void* buff, size_t length, Handler&& handler)
	{
#ifdef ENABLE_ASIO_PRE_OP
		size_t trySize = 0;
		if (is_pre_option())
		{
			result res = try_read_same(buff, length);
			if (!res.ok)
			{
				if (!try_again(res))
				{
					handler(res);
					return true;
				}
			}
			else if (res.s == length)
			{
				handler(res);
				return true;
			}
			else
			{
				trySize = res.s;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_socket.async_read_some(boost::asio::buffer((char*)buff + trySize, length - trySize),
				async_read_op<Handler>(handler, _socket, buff, trySize, length));
			return false;
#endif
		}
		boost::asio::async_read(_socket, boost::asio::buffer((char*)buff + trySize, length - trySize), std::bind([trySize](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { trySize + s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
#else
		boost::asio::async_read(_socket, boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
#endif
		return false;
	}

	/*!
	@brief 异步模式下，往读取数据，有多少读多少
	*/
	template <typename Handler>
	bool async_read_some(void* buff, size_t length, Handler&& handler)
	{
#ifdef ENABLE_ASIO_PRE_OP
		if (is_pre_option())
		{
			result res = try_read_same(buff, length);
			if (res.ok || !try_again(res))
			{
				handler(res);
				return true;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_socket.async_read_some(boost::asio::buffer(buff, length), wrap_tried(std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
			{
				result res = { s, ec.value(), !ec };
				handler(res);
			}, std::forward<Handler>(handler), __1, __2)));
			return false;
#endif
		}
#endif
		_socket.async_read_some(boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
		return false;
	}

	/*!
	@brief 异步模式下，将数据全部发送出去
	*/
	template <typename Handler>
	bool async_write(const void* buff, size_t length, Handler&& handler)
	{
#ifdef ENABLE_ASIO_PRE_OP
		size_t trySize = 0;
		if (is_pre_option())
		{
			result res = try_write_same(buff, length);
			if (!res.ok)
			{
				if (!try_again(res))
				{
					handler(res);
					return true;
				}
			}
			else if (res.s == length)
			{
				handler(res);
				return true;
			}
			else
			{
				trySize = res.s;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_socket.async_write_some(boost::asio::buffer((const char*)buff + trySize, length - trySize),
				async_write_op<Handler>(handler, _socket, buff, trySize, length));
			return false;
#endif
		}
		boost::asio::async_write(_socket, boost::asio::buffer((const char*)buff + trySize, length - trySize), std::bind([trySize](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { trySize + s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
#else
		boost::asio::async_write(_socket, boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
#endif
		return false;
	}

	/*!
	@brief 异步模式下，将数据发送出去，能发多少是多少
	*/
	template <typename Handler>
	bool async_write_some(const void* buff, size_t length, Handler&& handler)
	{
#ifdef ENABLE_ASIO_PRE_OP
		if (is_pre_option())
		{
			result res = try_write_same(buff, length);
			if (res.ok || !try_again(res))
			{
				handler(res);
				return true;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_socket.async_write_some(boost::asio::buffer(buff, length), wrap_tried(std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
			{
				result res = { s, ec.value(), !ec };
				handler(res);
			}, std::forward<Handler>(handler), __1, __2)));
			return false;
#endif
		}
#endif
		_socket.async_write_some(boost::asio::buffer(buff, length), std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
		return false;
	}

#ifdef HAS_ASIO_SEND_FILE
	/*!
	@brief 发送一个文件
	*/
#ifdef __linux__
	template <typename Handler>
	bool async_send_file(int fd, unsigned long long* offset, size_t length, Handler&& handler)
	{
		assert(!_sendFileState.fd);
		result res = try_send_file_same(fd, offset, length);
		if (res.ok && res.s && res.s != length)
		{
			_sendFileState.offset = offset;
			_sendFileState.count = length - res.s;
			_sendFileState.fd = fd;
			_socket.async_write_some(boost::asio::buffer((const char*)&_sendFileState, -1), async_send_file_op<Handler>(handler, *this, res.s));
			return false;
		}
#ifdef ENABLE_ASIO_PRE_OP
		if (is_pre_option())
		{
			handler(res);
			return true;
		}
#endif
		_socket.get_io_service().post(std::bind([](Handler& handler, result& res)
		{
			handler(res);
		}, std::forward<Handler>(handler), res));
		return false;
	}
#elif WIN32
	template <typename Handler>
	bool async_send_file(HANDLE hFile, unsigned long long* offset, size_t length, Handler&& handler)
	{
		if (!init_send_file(hFile, offset, length) || 0 == length)
		{
			result res = { 0, length ? ::WSAGetLastError() : 0, 0 == length };
#ifdef ENABLE_ASIO_PRE_OP
			if (is_pre_option())
			{
				handler(res);
				return true;
			}
#endif
			_socket.get_io_service().post(std::bind([](Handler& handler, result& res)
			{
				handler(res);
			}, std::forward<Handler>(handler), res));
			return false;
		}
		boost::asio::detail::send_file_pck pck = { hFile, (DWORD)length };
		_socket.async_write_some(boost::asio::buffer((const char*)&pck, -1), std::bind([offset](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			if (offset)
			{
				*offset += (unsigned long long)s;
			}
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
		return false;
	}
#endif
#endif

	/*!
	@brief 非阻塞尝试写入数据
	*/
	result try_write_same(const void* buff, size_t length);

	/*!
	@brief 非阻塞尝试读取数据
	*/
	result try_read_same(void* buff, size_t length);

	/*!
	@brief 非阻塞尝试一次写入多个数据
	@param lastBytes，不为 NULL 时返回写入的最后一个缓存实际写了多少字节
	@return lastBytes 不为 NULL 时返回实际写入了多少个缓存数据，为 NULL 时总共写了多少字节数据
	*/
	result try_mwrite_same(const void* const* buffs, const size_t* lengths, size_t count, size_t* lastBytes = NULL);

	/*!
	@brief 非阻塞尝试一次读取多个数据
	@param lastBytes，不为 NULL 时返回读取的最后一个缓存实际写了多少字节
	@return lastBytes 不为 NULL 时返回实际读取了多少个缓存数据，为 NULL 时总共读取了多少字节数据
	*/
	result try_mread_same(void* const* buffs, const size_t* lengths, size_t count, size_t* lastBytes = NULL);

	/*!
	@brief try_io操作失败是否是因为EAGAIN
	*/
	static bool try_again(const result& res);
private:
#ifdef HAS_ASIO_SEND_FILE
#ifdef __linux__
	result try_send_file_same(int fd, unsigned long long* offset, size_t& length);
#elif WIN32
	bool init_send_file(HANDLE hFile, unsigned long long* offset, size_t& length);
#endif
#endif
	result _try_mwrite_same(const void* const* buffs, const size_t* lengths, size_t count);
	result _try_mread_same(void* const* buffs, const size_t* lengths, size_t count);
	void set_internal_non_blocking();
private:
	boost::asio::ip::tcp::socket _socket;
#ifdef HAS_ASIO_SEND_FILE
#ifdef __linux__
	boost::asio::detail::socket_ops::send_file_pck _sendFileState;
#endif
#endif
	bool _nonBlocking;
#ifdef ENABLE_ASIO_PRE_OP
	bool _preOption;
#endif
	NONE_COPY(tcp_socket);
};

/*!
@brief 服务器侦听器
*/
class tcp_acceptor
{
public:
	tcp_acceptor(io_engine& ios);
	~tcp_acceptor();
public:
	/*!
	@brief 只允许特定ip连接该服务器
	*/
	tcp_socket::result open(const char* ip, unsigned short port);

	/*!
	@brief ip v4下打开服务器
	*/
	tcp_socket::result open_v4(unsigned short port);

	/*!
	@brief ip v6下打开服务器
	*/
	tcp_socket::result open_v6(unsigned short port);

	/*!
	@brief 用原始句柄构造
	*/
	tcp_socket::result assign(boost::asio::detail::socket_type accFd);
	tcp_socket::result assign_v6(boost::asio::detail::socket_type accFd);

	/*!
	@brief 关闭侦听器
	*/
	tcp_socket::result close();

	/*!
	@brief linux下优化异步返回（如果有数据，在async_xxx操作中直接回调）
	*/
	void pre_option();

	/*!
	@brief
	*/
	bool is_pre_option();

	/*!
	@brief 用socket侦听客户端连接
	*/
	tcp_socket::result accept(my_actor* host, tcp_socket& socket);

	/*!
	@brief 在ms时间范围内，用socket侦听客户端连接
	*/
	tcp_socket::result timed_accept(my_actor* host, int ms, tcp_socket& socket);

	/*!
	@brief 异步模式下，用socket侦听客户端连接
	*/
	template <typename Handler>
	bool async_accept(tcp_socket& socket, Handler&& handler)
	{
#ifdef ENABLE_ASIO_PRE_OP
		if (is_pre_option())
		{
			tcp_socket::result res = try_accept(socket);
			if (res.ok || !tcp_socket::try_again(res))
			{
				if (res.ok)
				{
					socket.set_internal_non_blocking();
				}
				handler(res);
				return true;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_acceptor->async_accept(socket._socket, wrap_tried(std::bind([&socket](Handler& handler, const boost::system::error_code& ec)
			{
				if (!ec)
				{
					socket.set_internal_non_blocking();
				}
				tcp_socket::result res = { 0, ec.value(), !ec };
				handler(res);
			}, std::forward<Handler>(handler), __1)));
			return false;
#endif
		}
#endif
		_acceptor->async_accept(socket._socket, std::bind([&socket](Handler& handler, const boost::system::error_code& ec)
		{
			if (!ec)
			{
				socket.set_internal_non_blocking();
			}
			tcp_socket::result res = { 0, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1));
		return false;
	}
private:
	void set_internal_non_blocking();
	tcp_socket::result try_accept(tcp_socket& socket);
private:
	io_engine& _ios;
	stack_obj<boost::asio::ip::tcp::acceptor> _acceptor;
	bool _nonBlocking;
#ifdef ENABLE_ASIO_PRE_OP
	bool _preOption;
#endif
	NONE_COPY(tcp_acceptor);
};

/*!
@brief udp通信
*/
class udp_socket
{
public:
	struct result
	{
		size_t s;///<字节数/缓存数
		int code;///<错误码
		bool ok;///<是否成功
	};
public:
	udp_socket(io_engine& ios);
	~udp_socket();
public:
	/*!
	@brief 关闭socket
	*/
	result close();

	/*!
	@brief ip v4模式打开socket
	*/
	result open_v4();

	/*!
	@brief ip v6模式打开socket
	*/
	result open_v6();

	/*!
	@brief 绑定本地一个ip下某个端口接收发送数据
	*/
	result bind(const char* ip, unsigned short port);

	/*!
	@brief 绑定ip v4下某个端口接收发送数据
	*/
	result bind_v4(unsigned short port);

	/*!
	@brief 绑定ip v6下某个端口接收发送数据
	*/
	result bind_v6(unsigned short port);

	/*!
	@brief 打开并绑定ip v4下某个端口接收发送数据
	*/
	result open_bind_v4(unsigned short port);

	/*!
	@brief 打开并绑定ip v6下某个端口接收发送数据
	*/
	result open_bind_v6(unsigned short port);
	
	/*!
	@brief 用原始句柄构造
	*/
	result assign(boost::asio::detail::socket_type sckFd);
	result assign_v6(boost::asio::detail::socket_type sckFd);

	/*!
	@brief linux下优化异步返回（如果有数据，在async_xxx操作中直接回调）
	*/
	void pre_option();

	/*!
	@brief 
	*/
	bool is_pre_option();

	/*!
	@brief 设定一个远程端口作为默认发送接收目标
	*/
	result connect(const char* remoteIp, unsigned short remotePort);
	result connect(const boost::asio::ip::udp::endpoint& remoteEndpoint);

	/*!
	@brief 发送数据到指定目标
	*/
	result send_to(my_actor* host, const boost::asio::ip::udp::endpoint& remoteEndpoint, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 发送数据到指定目标
	*/
	result send_to(my_actor* host, const char* remoteIp, unsigned short remotePort, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 在connect成功后，发送数据到默认目标
	*/
	result send(my_actor* host, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 接收远端发送的数据到缓冲区，并记录下远端地址
	*/
	result receive_from(my_actor* host, void* buff, size_t length, int flags = 0);

	/*!
	@brief 接收远端发送的数据到缓冲区
	*/
	result receive(my_actor* host, void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，发送数据到指定目标（本地系统缓存满了会导致发送阻塞）
	*/
	result timed_send_to(my_actor* host, int ms, const boost::asio::ip::udp::endpoint& remoteEndpoint, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，发送数据到指定目标（本地系统缓存满了会导致发送阻塞）
	*/
	result timed_send_to(my_actor* host, int ms, const char* remoteIp, unsigned short remotePort, const void* buff, size_t length, int flags = 0);

	/*!
	@brief connect成功后，在ms时间范围内，发送数据到默认目标（本地系统缓存满了会导致发送阻塞）
	*/
	result timed_send(my_actor* host, int ms, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，接收远端发送的数据到缓冲区，并记录下远端地址
	*/
	result timed_receive_from(my_actor* host, int ms, void* buff, size_t length, int flags = 0);

	/*!
	@brief 在ms时间范围内，接收远端发送的数据到缓冲区
	*/
	result timed_receive(my_actor* host, int ms, void* buff, size_t length, int flags = 0);

	/*!
	@brief 创建一个远端目标
	*/
	static boost::asio::ip::udp::endpoint make_endpoint(const char* remoteIp, unsigned short remotePort);

	/*!
	@brief receive_from完成后获取远端地址
	*/
	const boost::asio::ip::udp::endpoint& last_remote_sender_endpoint();

	/*!
	@brief 重置远端地址
	*/
	void reset_remote_sender_endpoint();

	/*!
	@brief 异步模式下，发送数据到指定目标
	*/
	template <typename Handler>
	bool async_send_to(const boost::asio::ip::udp::endpoint& remoteEndpoint, const void* buff, size_t length, Handler&& handler, int flags = 0)
	{
#ifdef ENABLE_ASIO_PRE_OP
		if (is_pre_option())
		{
			result res = try_send_to(remoteEndpoint, buff, length, flags);
			if (res.ok || !try_again(res))
			{
				handler(res);
				return true;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_socket.async_send_to(boost::asio::buffer(buff, length), remoteEndpoint, flags, wrap_tried(std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
			{
				result res = { s, ec.value(), !ec };
				handler(res);
			}, std::forward<Handler>(handler), __1, __2)));
			return false;
#endif
		}
#endif
		_socket.async_send_to(boost::asio::buffer(buff, length), remoteEndpoint, flags, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
		return false;
	}

	/*!
	@brief 异步模式下，发送数据到指定目标
	*/
	template <typename Handler>
	bool async_send_to(const char* remoteIp, unsigned short remotePort, const void* buff, size_t length, Handler&& handler, int flags = 0)
	{
		return async_send_to(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(remoteIp), remotePort), buff, length, std::forward<Handler>(handler), flags);
	}

	/*!
	@brief 异步模式下，发送数据到默认目标(connect成功后)
	*/
	template <typename Handler>
	bool async_send(const void* buff, size_t length, Handler&& handler, int flags = 0)
	{
#ifdef ENABLE_ASIO_PRE_OP
		if (is_pre_option())
		{
			result res = try_send(buff, length, flags);
			if (res.ok || !try_again(res))
			{
				handler(res);
				return true;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_socket.async_send(boost::asio::buffer(buff, length), flags, wrap_tried(std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
			{
				result res = { s, ec.value(), !ec };
				handler(res);
			}, std::forward<Handler>(handler), __1, __2)));
			return false;
#endif
		}
#endif
		_socket.async_send(boost::asio::buffer(buff, length), flags, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
		return false;
	}

	/*!
	@brief 异步模式下，接收远端发送的数据到缓冲区，并记录下远端地址
	*/
	template <typename Handler>
	bool async_receive_from(boost::asio::ip::udp::endpoint& remoteEndpoint, void* buff, size_t length, Handler&& handler, int flags = 0)
	{
#ifdef ENABLE_ASIO_PRE_OP
		if (is_pre_option())
		{
			result res = try_receive_from(remoteEndpoint, buff, length, flags);
			if (res.ok || !try_again(res))
			{
				handler(res);
				return true;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_socket.async_receive_from(boost::asio::buffer(buff, length), remoteEndpoint, flags, wrap_tried(std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
			{
				result res = { s, ec.value(), !ec };
				handler(res);
			}, std::forward<Handler>(handler), __1, __2)));
			return false;
#endif
		}
#endif
		_socket.async_receive_from(boost::asio::buffer(buff, length), remoteEndpoint, flags, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
		return false;
	}

	template <typename Handler>
	bool async_receive_from(void* buff, size_t length, Handler&& handler, int flags = 0)
	{
		return async_receive_from(_remoteSenderEndpoint, buff, length, std::forward<Handler>(handler), flags);
	}

	/*!
	@brief 异步模式下，接收远端发送的数据到缓冲区
	*/
	template <typename Handler>
	bool async_receive(void* buff, size_t length, Handler&& handler, int flags = 0)
	{
#ifdef ENABLE_ASIO_PRE_OP
		if (is_pre_option())
		{
			result res = try_receive(buff, length, flags);
			if (res.ok || !try_again(res))
			{
				handler(res);
				return true;
			}
#ifdef HAS_ASIO_HANDLER_IS_TRIED
			_socket.async_receive(boost::asio::buffer(buff, length), flags, wrap_tried(std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
			{
				result res = { s, ec.value(), !ec };
				handler(res);
			}, std::forward<Handler>(handler), __1, __2)));
			return false;
#endif
		}
#endif
		_socket.async_receive(boost::asio::buffer(buff, length), flags, std::bind([](Handler& handler, const boost::system::error_code& ec, size_t s)
		{
			result res = { s, ec.value(), !ec };
			handler(res);
		}, std::forward<Handler>(handler), __1, __2));
		return false;
	}

	/*!
	@brief 非阻塞尝试发送数据
	*/
	result try_send(const void* buff, size_t length, int flags = 0);

	/*!
	@brief 非阻塞尝试发送数据到指定目标
	*/
	result try_send_to(const char* remoteIp, unsigned short remotePort, const void* buff, size_t length, int flags = 0);
	result try_send_to(const boost::asio::ip::udp::endpoint& remoteEndpoint, const void* buff, size_t length, int flags = 0);

	/*!
	@brief 非阻塞尝试接收数据
	*/
	result try_receive(void* buff, size_t length, int flags = 0);

	/*!
	@brief 非阻塞尝试接收数据，并记录下远端地址
	*/
	result try_receive_from(void* buff, size_t length, int flags = 0);

	/*!
	@brief 非阻塞尝试接收数据，并记录下远端地址
	*/
	result try_receive_from(boost::asio::ip::udp::endpoint& remoteEndpoint, void* buff, size_t length, int flags = 0);

	/*!
	@brief 非阻塞尝试一次发送多条数据
	@brief bytes, 每块数据实际发送字节数
	@return 实际发送数据条数
	*/
	result try_msend(const void* const* buffs, const size_t* lengths, size_t count, size_t* bytes = NULL, int flags = 0);

	/*!
	@brief 非阻塞尝试一次发送多条数据到指定目标
	@brief bytes, 每块数据实际发送字节数
	@return 实际发送数据条数
	*/
	result try_msend_to(const boost::asio::ip::udp::endpoint* remoteEndpoints, const void* const* buffs, const size_t* lengths, size_t count, size_t* bytes = NULL, int flags = 0);
	result try_msend_to(const boost::asio::ip::udp::endpoint& remoteEndpoint, const void* const* buffs, const size_t* lengths, size_t count, size_t* bytes = NULL, int flags = 0);

	/*!
	@brief 非阻塞尝试一次接收多条数据
	@brief bytes, 每块缓存实际接收字节数
	@return 实际接收数据条数
	*/
	result try_mreceive(void* const* buffs, const size_t* lengths, size_t count, size_t* bytes = NULL, int flags = 0);

	/*!
	@brief 非阻塞尝试一次接收多条数据，并记录下远端地址
	@brief bytes, 每块缓存实际接收字节数
	@return 实际接收数据条数
	*/
	result try_mreceive_from(boost::asio::ip::udp::endpoint* remoteEndpoints, void* const* buffs, const size_t* lengths, size_t count, size_t* bytes = NULL, int flags = 0);

	/*!
	@brief try_io操作失败是否是因为EAGAIN
	*/
	static bool try_again(const result& res);
private:
	void set_internal_non_blocking();
private:
	boost::asio::ip::udp::socket _socket;
	boost::asio::ip::udp::endpoint _remoteSenderEndpoint;
	bool _nonBlocking;
#ifdef ENABLE_ASIO_PRE_OP
	bool _preOption;
#endif
	NONE_COPY(udp_socket);
};

#endif