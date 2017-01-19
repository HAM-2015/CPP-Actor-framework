#include "strand_ex.h"
#include "io_engine.h"

namespace boost
{
	namespace asio
	{
		namespace detail
		{
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

			template <>
			void boost::asio::detail::strand_service::dispatch(boost::asio::detail::strand_service::implementation_type& impl, get_impl_ready_empty_strand_ex& fh)
			{
				fh._empty = impl->ready_queue_.empty();
			}

			template <>
			void boost::asio::detail::strand_service::dispatch(boost::asio::detail::strand_service::implementation_type& impl, get_impl_waiting_empty_strand_ex& fh)
			{
				fh._empty = impl->waiting_queue_.empty();
			}

			template <>
			void boost::asio::detail::strand_service::dispatch(boost::asio::detail::strand_service::implementation_type& impl, get_impl_running_strand_ex& fh)
			{
				fh._running = impl->locked_;
			}

			template <>
			void boost::asio::detail::strand_service::dispatch(boost::asio::detail::strand_service::implementation_type& impl, get_impl_safe_running_strand_ex& fh)
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
	boost::asio::detail::get_impl_ready_empty_strand_ex t;
	_service.dispatch((boost::asio::detail::strand_service::implementation_type&)_impl, t);
	return t._empty;
}

bool StrandEx_::waiting_empty() const
{
	boost::asio::detail::get_impl_waiting_empty_strand_ex t;
	_service.dispatch((boost::asio::detail::strand_service::implementation_type&)_impl, t);
	return t._empty;
}

bool StrandEx_::running() const
{
	assert(running_in_this_thread());
	boost::asio::detail::get_impl_running_strand_ex t;
	_service.dispatch((boost::asio::detail::strand_service::implementation_type&)_impl, t);
	return t._running;
}

bool StrandEx_::safe_running() const
{
	assert(!running_in_this_thread());
	boost::asio::detail::get_impl_safe_running_strand_ex t;
	_service.dispatch((boost::asio::detail::strand_service::implementation_type&)_impl, t);
	return t._running;
}

bool StrandEx_::only_self() const
{
	assert(running_in_this_thread());
#ifdef ASIO_CALL_STACK_DEPTH
	return 1 == boost::asio::detail::call_stack<boost::asio::detail::strand_service::strand_impl>::stack_depth();
#else
	return true;
#endif
}