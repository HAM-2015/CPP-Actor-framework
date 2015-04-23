#include "acceptor_socket.h"
#include "scattered.h"

acceptor_socket::acceptor_socket()
{
	_acceptor = NULL;
}

acceptor_socket::~acceptor_socket()
{
	delete _acceptor;
}

accept_handle acceptor_socket::create(shared_strand strand, size_t port, const std::function<void(socket_handle)>& h, bool reuse)
{
	try
	{
		accept_handle shared_accept(new acceptor_socket(), [](acceptor_socket* p){delete p; });
		shared_accept->_acceptor = new boost::asio::ip::tcp::acceptor(strand->get_io_service(),
			boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), (unsigned short)port), reuse);
		shared_accept->_h = h;
		shared_accept->acceptor_run(shared_accept);
		return shared_accept;
	}
	catch (...)
	{//acceptor¹¹ÔìÒì³£
		return accept_handle();
	}
}

void acceptor_socket::close()
{
	boost::system::error_code ec;
	_acceptor->close(ec);
}

void acceptor_socket::acceptor_run(accept_handle shared_this)
{
	socket_handle newSocket = socket_io::create(shared_this->_acceptor->get_io_service());
	shared_this->_acceptor->async_accept((boost::asio::ip::tcp::socket&)*newSocket, [newSocket, shared_this]
		(const boost::system::error_code& err)
	{
		if (!err)
		{
			newSocket->ip();
			shared_this->_h(newSocket);
			shared_this->acceptor_run(shared_this);
		}
		else
		{
			shared_this->close();
			shared_this->_h(socket_handle());
			clear_function(shared_this->_h);
		}
	});
}