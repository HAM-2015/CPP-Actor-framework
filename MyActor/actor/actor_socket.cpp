#include "actor_socket.h"

tcp_socket::tcp_socket(boost::asio::io_service& ios)
:_socket(ios) {}

tcp_socket::~tcp_socket()
{}

bool tcp_socket::close()
{
	boost::system::error_code ec;
	_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	_socket.close(ec);
	return !ec;
}

bool tcp_socket::no_delay()
{
	boost::system::error_code ec;
	_socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);
	return !ec;
}

std::string tcp_socket::local_endpoint(unsigned short& port)
{
	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint ep = _socket.local_endpoint(ec);
	if (!ec)
	{
		port = ep.port();
		return ep.address().to_string(ec);
	}
	port = 0;
	return std::string();
}

std::string tcp_socket::remote_endpoint(unsigned short& port)
{
	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint ep = _socket.remote_endpoint(ec);
	if (!ec)
	{
		port = ep.port();
		return ep.address().to_string(ec);
	}
	port = 0;
	return std::string();
}

bool tcp_socket::connect(my_actor* host, const char* ip, unsigned short port)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, ip, port](trig_once_notifer<bool>&& h)
	{
		async_connect(ip, port, std::move(h));
	});
}

bool tcp_socket::read(my_actor* host, void* buff, size_t length)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, buff, length](trig_once_notifer<bool>&& h)
	{
		async_read(buff, length, std::move(h));
	});
}

size_t tcp_socket::read_some(my_actor* host, void* buff, size_t length)
{
	my_actor::quit_guard qg(host);
	return host->trig<size_t>([this, buff, length](trig_once_notifer<size_t>&& h)
	{
		async_read_some(buff, length, std::move(h));
	});
}

bool tcp_socket::write(my_actor* host, const void* buff, size_t length)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, buff, length](trig_once_notifer<bool>&& h)
	{
		async_write(buff, length, std::move(h));
	});
}

size_t tcp_socket::write_some(my_actor* host, const void* buff, size_t length)
{
	my_actor::quit_guard qg(host);
	return host->trig<size_t>([this, buff, length](trig_once_notifer<size_t>&& h)
	{
		async_write_some(buff, length, std::move(h));
	});
}

bool tcp_socket::timed_connect(my_actor* host, int tm, bool& timed, const char* ip, unsigned short port)
{
	timed = false;
	bool ok = false;
	async_connect(ip, port, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}

bool tcp_socket::timed_read(my_actor* host, int tm, bool& timed, void* buff, size_t length)
{
	timed = false;
	bool ok = false;
	async_read(buff, length, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}

size_t tcp_socket::timed_read_some(my_actor* host, int tm, bool& timed, void* buff, size_t length)
{
	timed = false;
	size_t s = 0;
	async_read_some(buff, length, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, s));
	if (timed)
	{
		s = 0;
	}
	return s;
}

bool tcp_socket::timed_write(my_actor* host, int tm, bool& timed, const void* buff, size_t length)
{
	timed = false;
	bool ok = false;
	async_write(buff, length, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}

size_t tcp_socket::timed_write_some(my_actor* host, int tm, bool& timed, const void* buff, size_t length)
{
	timed = false;
	size_t s = 0;
	async_write_some(buff, length, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, s));
	if (timed)
	{
		s = 0;
	}
	return s;
}
//////////////////////////////////////////////////////////////////////////

tcp_acceptor::tcp_acceptor(boost::asio::io_service& ios)
:_ios(ios), _opend(false) {}

tcp_acceptor::~tcp_acceptor()
{
	assert(!_opend);
}

bool tcp_acceptor::open(const char* ip, unsigned short port)
{
	if (!_opend)
	{
		try
		{
			_acceptor.create(_ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port), false);
			_opend = true;
			return true;
		}
		catch (...)
		{
		}
	}
	return false;
}

bool tcp_acceptor::open_v4(unsigned short port)
{
	if (!_opend)
	{
		try
		{
			_acceptor.create(_ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), port), false);
			_opend = true;
			return true;
		}
		catch (...)
		{
		}
	}
	return false;
}

bool tcp_acceptor::open_v6(unsigned short port)
{
	if (!_opend)
	{
		try
		{
			_acceptor.create(_ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v6(), port), false);
			_opend = true;
			return true;
		}
		catch (...)
		{
		}
	}
	return false;
}

bool tcp_acceptor::close()
{
	if (_opend)
	{
		_opend = false;
		boost::system::error_code ec;
		_acceptor->close(ec);
		_acceptor.destroy();
		return !ec;
	}
	return false;
}

bool tcp_acceptor::accept(my_actor* host, tcp_socket& socket)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, &socket](trig_once_notifer<bool>&& h)
	{
		async_accept(socket, std::move(h));
	});
}

bool tcp_acceptor::timed_accept(my_actor* host, int tm, bool& timed, tcp_socket& socket)
{
	timed = false;
	bool ok = false;
	async_accept(socket, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}
//////////////////////////////////////////////////////////////////////////

udp_socket::udp_socket(boost::asio::io_service& ios)
:_socket(ios) {}

udp_socket::~udp_socket()
{}

bool udp_socket::open_v4()
{
	boost::system::error_code ec;
	_socket.open(boost::asio::ip::udp::v4(), ec);
	return !ec;
}

bool udp_socket::open_v6()
{
	boost::system::error_code ec;
	_socket.open(boost::asio::ip::udp::v6(), ec);
	return !ec;
}

bool udp_socket::bind(const char* ip, unsigned short port)
{
	boost::system::error_code ec;
	_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip), port), ec);
	return !ec;
}

bool udp_socket::bind_v4(unsigned short port)
{
	boost::system::error_code ec;
	_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), port), ec);
	return !ec;
}

bool udp_socket::bind_v6(unsigned short port)
{
	boost::system::error_code ec;
	_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6(), port), ec);
	return !ec;
}

bool udp_socket::close()
{
	boost::system::error_code ec;
	_socket.shutdown(boost::asio::ip::udp::socket::shutdown_both, ec);
	_socket.close(ec);
	return !ec;
}

const udp_socket::remote_sender_endpoint& udp_socket::last_remote_sender_endpoint()
{
	return _remoteSenderEndpoint;
}

bool udp_socket::connect(my_actor* host, const char* remoteIp, unsigned short remotePort)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, remoteIp, remotePort](trig_once_notifer<bool>&& h)
	{
		async_connect(remoteIp, remotePort, std::move(h));
	});
}

bool udp_socket::send_to(my_actor* host, const remote_sender_endpoint& remoteEndpoint, const void* buff, size_t length)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, &remoteEndpoint, buff, length](trig_once_notifer<bool>&& h)
	{
		async_send_to(remoteEndpoint, buff, length, std::move(h));
	});
}

bool udp_socket::send(my_actor* host, const void* buff, size_t length)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, buff, length](trig_once_notifer<bool>&& h)
	{
		async_send(buff, length, std::move(h));
	});
}

bool udp_socket::receive_from(my_actor* host, void* buff, size_t length)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, buff, length](trig_once_notifer<bool>&& h)
	{
		async_receive_from(buff, length, std::move(h));
	});
}

bool udp_socket::receive(my_actor* host, void* buff, size_t length)
{
	my_actor::quit_guard qg(host);
	return host->trig<bool>([this, buff, length](trig_once_notifer<bool>&& h)
	{
		async_receive(buff, length, std::move(h));
	});
}

bool udp_socket::timed_connect(my_actor* host, int tm, bool& timed, const char* remoteIp, unsigned short remotePort)
{
	timed = false;
	bool ok = false;
	async_connect(remoteIp, remotePort, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}

bool udp_socket::timed_send_to(my_actor* host, int tm, bool& timed, const remote_sender_endpoint& remoteEndpoint, const void* buff, size_t length)
{
	timed = false;
	bool ok = false;
	async_send_to(remoteEndpoint, buff, length, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}

bool udp_socket::timed_send(my_actor* host, int tm, bool& timed, const void* buff, size_t length)
{
	timed = false;
	bool ok = false;
	async_send(buff, length, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}

bool udp_socket::timed_receive_from(my_actor* host, int tm, bool& timed, void* buff, size_t length)
{
	timed = false;
	bool ok = false;
	async_receive_from(buff, length, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}

bool udp_socket::timed_receive(my_actor* host, int tm, bool& timed, void* buff, size_t length)
{
	timed = false;
	bool ok = false;
	async_receive(buff, length, host->make_asio_timed_context(tm, [&]()
	{
		timed = true;
		close();
	}, ok));
	if (timed)
	{
		ok = false;
	}
	return ok;
}