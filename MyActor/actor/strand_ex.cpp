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

namespace boost
{
	namespace asio
	{
		namespace detail
		{
			template <>
			inline void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_ready_empty_strand_ex& fh)
			{
				fh._empty = impl->ready_queue_.empty();
			}

			template <>
			void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_waiting_empty_strand_ex& fh)
			{
				fh._empty = impl->waiting_queue_.empty();
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////

StrandEx_::StrandEx_(io_engine& ios)
: _service(boost::asio::use_service<boost::asio::detail::strand_service>(ios))
{
	_impl = new boost::asio::detail::strand_service::strand_impl();
}

StrandEx_::~StrandEx_()
{
	delete _impl;
}

bool StrandEx_::running_in_this_thread() const
{
	//return boost::asio::detail::call_stack<boost::asio::detail::strand_service::strand_impl>::contains(_impl) != 0;
	return _service.running_in_this_thread(_impl);
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