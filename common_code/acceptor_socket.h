#ifndef __ACCEPTOR_SOCKET_H
#define __ACCEPTOR_SOCKET_H

#include "socket_io.h"
#include "shared_strand.h"
#include "boost_coroutine.h"
#include <boost/asio/ip/tcp.hpp>

class acceptor_socket
{
private:
	acceptor_socket(boost::asio::io_service& ios, size_t port, bool reuse);
public:
	~acceptor_socket();
	static boost::shared_ptr<acceptor_socket> create(shared_strand  strand, size_t port, const boost::function<void (boost::shared_ptr<socket_io>)>& h, bool reuse = true);
public:
	void close();
private:
	void acceptorCoro(boost_coro* coro);
private:
	boost::asio::ip::tcp::acceptor _acceptor;
	boost::function<void (boost::shared_ptr<socket_io>)> _socketNotify;
};

#endif