#include "shared_strand.h"

boost_strand::boost_strand()
{
	_iosProxy = NULL;
	_strand = NULL;
}

boost_strand::~boost_strand()
{
	if (_strand)
	{
		delete _strand;
	}
}

shared_strand boost_strand::create( ios_proxy& iosProxy )
{
	shared_strand res(new boost_strand);
	res->_iosProxy = &iosProxy;
	res->_strand = new strand_type(iosProxy);
	return res;
}

shared_strand boost_strand::clone()
{
	return create(*_iosProxy);
}

bool boost_strand::in_this_ios()
{
	assert(_iosProxy);
	return _iosProxy->runningInThisIos();
}

bool boost_strand::running_in_this_thread()
{
	assert(_strand);
	return _strand->running_in_this_thread();
}

size_t boost_strand::ios_thread_number()
{
	assert(_iosProxy);
	return _iosProxy->threadNumber();
}

ios_proxy& boost_strand::get_ios_proxy()
{
	assert(_iosProxy);
	return *_iosProxy;
}

boost::asio::io_service& boost_strand::get_io_service()
{
	assert(_iosProxy);
	return *_iosProxy;
}

#ifdef ENABLE_MFC_ACTOR
void boost_strand::_post( const boost::function<void ()>& h )
{
	assert(false);
}
#endif