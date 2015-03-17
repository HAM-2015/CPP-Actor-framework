#ifndef __ACCEPTOR_SOCKET_H
#define __ACCEPTOR_SOCKET_H

#include "socket_io.h"
#include "shared_strand.h"
#include "actor_framework.h"
#include <boost/asio/ip/tcp.hpp>

class acceptor_socket;
typedef boost::shared_ptr<acceptor_socket> accept_handle;

class acceptor_socket
{
private:
	acceptor_socket();
public:
	~acceptor_socket();
	static accept_handle create(shared_strand  strand, size_t port, const boost::function<void(socket_handle)>& h, bool reuse = true);
public:
	void close();
private:
	boost::asio::ip::tcp::acceptor* _acceptor;
};

#endif