#include "acceptor_socket.h"

acceptor_socket::acceptor_socket()
{
	_acceptor = NULL;
}

acceptor_socket::~acceptor_socket()
{
	delete _acceptor;
}

accept_handle acceptor_socket::create(shared_strand strand, size_t port, const boost::function<void(socket_handle)>& h, bool reuse)
{
	boost::asio::ip::tcp::acceptor* accept = NULL;
	try
	{
		accept = new boost::asio::ip::tcp::acceptor(strand->get_io_service(),
			boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), (unsigned short)port), reuse);
	}
	catch (...)
	{//acceptor¹¹ÔìÒì³£
		return accept_handle();
	}

	accept_handle shared_accept(new acceptor_socket());
	shared_accept->_acceptor = accept;
	my_actor::create(strand, [shared_accept, h](my_actor* self)
	{
		async_trig_handle<boost::system::error_code> ath;
		while (true)
		{
			socket_handle newSocket = socket_io::create(shared_accept->_acceptor->get_io_service());
			shared_accept->_acceptor->async_accept((boost::asio::ip::tcp::socket&)*newSocket, self->begin_trig(ath));
			if (boost::system::error_code() != self->wait_trig(ath))
			{
				boost::system::error_code ec;
				shared_accept->_acceptor->close(ec);
				h(socket_handle());
				break;
			}
			newSocket->ip();
			h(newSocket);
		}
	})->notify_start_run();
	return shared_accept;
}

void acceptor_socket::close()
{
	boost::system::error_code ec;
	_acceptor->close(ec);
}