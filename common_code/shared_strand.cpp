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
:_pCheckDestroy(NULL), _isReset(true), _nextTickAlloc(128), _nextTickQueue1(128), _nextTickQueue2(128), _frontTickQueue(&_nextTickQueue1), _backTickQueue(&_nextTickQueue2)
#endif //ENABLE_NEXT_TICK
{
	_ioEngine = NULL;
	_strand = NULL;
	_timer = NULL;
}

boost_strand::~boost_strand()
{
#ifdef ENABLE_NEXT_TICK
	assert(_nextTickQueue1.empty());
	assert(_nextTickQueue2.empty());
	if (_pCheckDestroy)
	{
		*_pCheckDestroy = true;
	}
#endif //ENABLE_NEXT_TICK
	delete _strand;
	delete _timer;
}

shared_strand boost_strand::create(io_engine& ioEngine, bool makeTimer /* = true */)
{
	shared_strand res(new boost_strand);
	res->_ioEngine = &ioEngine;
	res->_strand = new strand_type(ioEngine);
	if (makeTimer)
	{
		res->_timer = new ActorTimer_(res);
	}
	res->_weakThis = res;
	return res;
}

shared_strand boost_strand::clone()
{
	return create(*_ioEngine);
}

bool boost_strand::in_this_ios()
{
	assert(_ioEngine);
	return _ioEngine->runningInThisIos();
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
	return _strand->empty() && (checkTick ? _nextTickQueue1.empty() && _nextTickQueue2.empty() : true);
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
	return t2._empty && t3._empty && (checkTick ? _nextTickQueue1.empty() && _nextTickQueue2.empty() : true);
#else
	return t2._empty && t3._empty;
#endif

#endif //end ENABLE_SHARE_STRAND_IMPL
}

size_t boost_strand::ios_thread_number()
{
	assert(_ioEngine);
	return _ioEngine->threadNumber();
}

io_engine& boost_strand::get_io_engine()
{
	assert(_ioEngine);
	return *_ioEngine;
}

boost::asio::io_service& boost_strand::get_io_service()
{
	assert(_ioEngine);
	return *_ioEngine;
}

ActorTimer_* boost_strand::get_timer()
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

void boost_strand::run_tick_front()
{
	assert(_isReset);
	while (!_frontTickQueue->empty())
	{
		wrap_next_tick_base* tick = _frontTickQueue->front();
		_frontTickQueue->pop_front();
		auto res = tick->invoke();
		if (-1 == res._size)
		{
			_nextTickAlloc.deallocate(res._ptr);
		}
		else
		{
			_reuMemAlloc.deallocate(res._ptr, res._size);
		}
	}
}

void boost_strand::run_tick_back()
{
	assert(ready_empty());
	if (!_backTickQueue->empty())
	{
		shared_strand lockThis = _weakThis.lock();
		int tickCount = 0;
		do
		{
			wrap_next_tick_base* tick = _backTickQueue->front();
			_backTickQueue->pop_front();
			auto res = tick->invoke();
			if (-1 == res._size)
			{
				_nextTickAlloc.deallocate(res._ptr);
			}
			else
			{
				_reuMemAlloc.deallocate(res._ptr, res._size);
			}
		} while (!_backTickQueue->empty() && ++tickCount <= 64);
		std::swap(_frontTickQueue, _backTickQueue);
		if (!_frontTickQueue->empty() && waiting_empty())
		{
			post([lockThis]{});
		}
	}
}
#endif //ENABLE_NEXT_TICK

#if (defined ENABLE_MFC_ACTOR || defined ENABLE_WX_ACTOR)
void boost_strand::_post( const std::function<void ()>& h )
{
	assert(false);
}
#endif