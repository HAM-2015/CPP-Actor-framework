#ifndef __ACCEPTOR_SOCKET_H
#define __ACCEPTOR_SOCKET_H

#include "socket_io.h"
#include "shared_strand.h"
#include <boost/asio/ip/tcp.hpp>

class acceptor_socket;
typedef std::shared_ptr<acceptor_socket> accept_handle;

class acceptor_socket
{
private:
	acceptor_socket();
public:
	~acceptor_socket();
	static accept_handle create(shared_strand  strand, size_t port, const std::function<void(socket_handle)>& h, bool reuse = true);
public:
	void close();
private:
	void acceptor_run(accept_handle shared_this);
private:
	boost::asio::ip::tcp::acceptor* _acceptor;
	std::function<void(socket_handle)> _h;
};

#endif