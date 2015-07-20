#include "shared_strand.h"
#include "actor_timer.h"

#ifndef ENABLE_STRAND_IMPL_POOL
struct get_impl
{
	mutable boost::asio::detail::strand_service* service_;
	mutable boost::asio::detail::strand_service::implementation_type impl_;
};

template <>
inline void boost::asio::io_service::strand::post(BOOST_ASIO_MOVE_ARG(get_impl) out)
{
	out.service_ = &service_;
	out.impl_ = impl_;
}

struct get_impl_ready_empty_shared_strand
{
	bool _empty;
};

struct get_impl_waiting_empty_shared_strand
{
	bool _empty;
};

template <>
inline void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_ready_empty_shared_strand& out)
{
	out._empty = impl->ready_queue_.empty();
}

template <>
inline void boost::asio::detail::strand_service::post(boost::asio::detail::strand_service::implementation_type& impl, get_impl_waiting_empty_shared_strand& out)
{
	out._empty = impl->waiting_queue_.empty();
}
#endif //ENABLE_STRAND_IMPL_POOL
//////////////////////////////////////////////////////////////////////////

boost_strand::boost_strand()
#ifdef ENABLE_NEXT_TICK
:_pCheckDestroy(NULL), _nextTickAlloc(64), _nextTickQueue(64)
#endif //ENABLE_NEXT_TICK
{
	_iosProxy = NULL;
	_strand = NULL;
	_timer = NULL;
}

boost_strand::~boost_strand()
{
#ifdef ENABLE_NEXT_TICK
	assert(_nextTickQueue.empty());
	if (_pCheckDestroy)
	{
		*_pCheckDestroy = true;
	}
#endif //ENABLE_NEXT_TICK
	delete _strand;
	delete _timer;
}

shared_strand boost_strand::create(ios_proxy& iosProxy, bool makeTimer /* = true */)
{
	shared_strand res(new boost_strand, [](boost_strand* p){delete p; });
	res->_iosProxy = &iosProxy;
	res->_strand = new strand_type(iosProxy);
	if (makeTimer)
	{
		res->_timer = new actor_timer(res);
	}
	res->_weakThis = res;
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

bool boost_strand::empty(bool checkTick)
{
	assert(_strand);
	assert(running_in_this_thread());
#ifdef ENABLE_STRAND_IMPL_POOL

#ifdef ENABLE_NEXT_TICK
	return _strand->empty() && (checkTick ? _nextTickQueue.empty() : true);
#else
	return _strand->empty();
#endif

#else
	get_impl t1;
	get_impl_ready_empty_shared_strand t2;
	get_impl_waiting_empty_shared_strand t3;
	_strand->post(std::move(t1));
	t1.service_->post(t1.impl_, t2);
	t1.service_->post(t1.impl_, t3);

#ifdef ENABLE_NEXT_TICK
	return t2._empty && t3._empty && (checkTick ? _nextTickQueue.empty() : true);
#else
	return t2._empty && t3._empty;
#endif

#endif //end ENABLE_SHARE_STRAND_IMPL
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

actor_timer* boost_strand::get_timer()
{
	return _timer;
}

#ifdef ENABLE_NEXT_TICK
bool boost_strand::ready_empty()
{
#ifdef ENABLE_STRAND_IMPL_POOL
	return _strand->ready_empty();
#else
	get_impl t1;
	get_impl_ready_empty_shared_strand t2;
	_strand->post(std::move(t1));
	t1.service_->post(t1.impl_, t2);
	return t2._empty;
#endif //end ENABLE_SHARE_STRAND_IMPL
}

bool boost_strand::waiting_empty()
{
#ifdef ENABLE_STRAND_IMPL_POOL
	return _strand->waiting_empty();
#else
	get_impl t1;
	get_impl_waiting_empty_shared_strand t2;
	_strand->post(std::move(t1));
	t1.service_->post(t1.impl_, t2);
	return t2._empty;
#endif //end ENABLE_SHARE_STRAND_IMPL
}

void boost_strand::run_tick()
{
	assert(ready_empty() && !_nextTickQueue.empty());
	shared_strand lockThis = _weakThis.lock();
	int tickCount = 0;
	do
	{
		wrap_next_tick_base* tick = _nextTickQueue.front();
		_nextTickQueue.pop_front();
		tick->invoke();
		if (void* tp = tick->destroy())
		{
			_nextTickAlloc.deallocate(tp);
		}
	} while (!_nextTickQueue.empty() && (++tickCount <= 64 || waiting_empty()));
}
#endif //ENABLE_NEXT_TICK

#if (defined ENABLE_MFC_ACTOR || defined ENABLE_WX_ACTOR)
void boost_strand::_post( const std::function<void ()>& h )
{
	assert(false);
}
#endif