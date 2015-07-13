#include "strand_ex.h"

#ifdef ENABLE_STRAND_IMPL_POOL
struct get_impl_empty_strand_ex
{
	bool _empty;
};

template <>
inline void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_empty_strand_ex& fh)
{
	fh._empty = impl->ready_queue_.empty() && impl->waiting_queue_.empty();
}
//////////////////////////////////////////////////////////////////////////

strand_ex::strand_ex(ios_proxy& ios) : _ios(ios), _service(boost::asio::use_service<boost::asio::detail::strand_service>(ios))
{
	_impl = (boost::asio::detail::strand_service::implementation_type)_ios.getImpl();
}

strand_ex::~strand_ex()
{
	_ios.freeImpl(_impl);
}

boost::asio::io_service& strand_ex::get_io_service()
{
	return _service.get_io_service();
}

bool strand_ex::running_in_this_thread() const
{
	return _service.running_in_this_thread(_impl);
}

bool strand_ex::empty()
{
	get_impl_empty_strand_ex t;
	_service.post(_impl, t);
	return t._empty;
}
#endif