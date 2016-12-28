#include "strand_ex.h"
#include "io_engine.h"

struct get_impl_ready_empty_strand_ex
{
	bool _empty;
};

struct get_impl_waiting_empty_strand_ex
{
	bool _empty;
};

struct get_impl_running_strand_ex
{
	bool _running;
};

struct get_impl_safe_running_strand_ex
{
	bool _running;
};

namespace boost
{
	namespace asio
	{
		namespace detail
		{
			template <>
			void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_ready_empty_strand_ex& fh)
			{
				fh._empty = impl->ready_queue_.empty();
			}

			template <>
			void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_waiting_empty_strand_ex& fh)
			{
#ifdef ENABLE_POST_FRONT
				fh._empty = impl->waiting_queue_.empty() && impl->front_queue_.empty();
#else
				fh._empty = impl->waiting_queue_.empty();
#endif
			}

			template <>
			void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_running_strand_ex& fh)
			{
				fh._running = impl->locked_;
			}

			template <>
			void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_safe_running_strand_ex& fh)
			{
				impl->mutex_.lock();
				fh._running = impl->locked_;
				impl->mutex_.unlock();
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////

StrandEx_::StrandEx_(io_engine& ios)
: _service(boost::asio::use_service<boost::asio::detail::strand_service>(ios)),
_impl(new boost::asio::detail::strand_service::strand_impl()) {}

StrandEx_::~StrandEx_()
{
	delete _impl;
}

bool StrandEx_::running_in_this_thread() const
{
	return boost::asio::detail::call_stack<boost::asio::detail::strand_service::strand_impl>::contains(_impl) != 0;
}

bool StrandEx_::empty() const
{
	return ready_empty() && waiting_empty();
}

bool StrandEx_::ready_empty() const
{
	get_impl_ready_empty_strand_ex t;
	_service.post((boost::asio::detail::strand_service::implementation_type&)_impl, t);
	return t._empty;
}

bool StrandEx_::waiting_empty() const
{
	get_impl_waiting_empty_strand_ex t;
	_service.post((boost::asio::detail::strand_service::implementation_type&)_impl, t);
	return t._empty;
}

bool StrandEx_::running() const
{
	assert(running_in_this_thread());
	get_impl_running_strand_ex t;
	_service.post((boost::asio::detail::strand_service::implementation_type&)_impl, t);
	return t._running;
}

bool StrandEx_::safe_running() const
{
	assert(!running_in_this_thread());
	get_impl_safe_running_strand_ex t;
	_service.post((boost::asio::detail::strand_service::implementation_type&)_impl, t);
	return t._running;
}