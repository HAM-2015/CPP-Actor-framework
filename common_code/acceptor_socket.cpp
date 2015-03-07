#include "acceptor_socket.h"

acceptor_socket::acceptor_socket(boost::asio::io_service& ios, size_t port, bool reuse)
	: _acceptor(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), (unsigned short)port), reuse)
{

}

acceptor_socket::~acceptor_socket()
{

}

boost::shared_ptr<acceptor_socket> acceptor_socket::create( shared_strand strand, size_t port, const boost::function<void (boost::shared_ptr<socket_io>)>& h, bool reuse )
{
	try
	{
		boost::shared_ptr<acceptor_socket> res(new acceptor_socket(strand->get_io_service(), port, reuse));
		res->_socketNotify = h;
		boost_actor::create(strand, boost::bind(&acceptor_socket::acceptorActor, res, _1))->notify_start_run();
		return res;
	}
	catch (...)
	{
		return boost::shared_ptr<acceptor_socket>();
	}
}

void acceptor_socket::close()
{
	boost::system::error_code ec;
	_acceptor.close(ec);
}

void acceptor_socket::acceptorActor(boost_actor* actor)
{
	while (true)
	{
		auto newSocket = socket_io::create(_acceptor.get_io_service());
		async_trig_handle<boost::system::error_code> ath;
		_acceptor.async_accept((boost::asio::ip::tcp::socket&)*newSocket, actor->begin_trig(ath));
		if (actor->wait_trig(ath))
		{
			_socketNotify(boost::shared_ptr<socket_io>());
			break;
		}
		newSocket->ip();
		_socketNotify(newSocket);
	}
	_socketNotify.clear();
}
