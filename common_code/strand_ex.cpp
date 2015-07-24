#include "strand_ex.h"

#ifdef ENABLE_STRAND_IMPL_POOL
struct get_impl_ready_empty_strand_ex
{
	bool _empty;
};

struct get_impl_waiting_empty_strand_ex
{
	bool _empty;
};

template <>
inline void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_ready_empty_strand_ex& fh)
{
	fh._empty = impl->ready_queue_.empty();
}

template <>
inline void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_waiting_empty_strand_ex& fh)
{
	fh._empty = impl->waiting_queue_.empty();
}
//////////////////////////////////////////////////////////////////////////

StrandEx_::StrandEx_(ios_proxy& ios) : _ios(ios), _service(boost::asio::use_service<boost::asio::detail::strand_service>(ios))
{
	_impl = (boost::asio::detail::strand_service::implementation_type)_ios.getImpl();
}

StrandEx_::~StrandEx_()
{
	_ios.freeImpl(_impl);
}

boost::asio::io_service& StrandEx_::get_io_service()
{
	return _service.get_io_service();
}

bool StrandEx_::running_in_this_thread() const
{
	return _service.running_in_this_thread(_impl);
}

bool StrandEx_::empty()
{
	return ready_empty() && waiting_empty();
}

bool StrandEx_::ready_empty()
{
	get_impl_ready_empty_strand_ex t;
	_service.post(_impl, t);
	return t._empty;
}

bool StrandEx_::waiting_empty()
{
	get_impl_waiting_empty_strand_ex t;
	_service.post(_impl, t);
	return t._empty;
}
#endif