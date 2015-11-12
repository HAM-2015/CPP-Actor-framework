#include "shared_strand.h"
#include "actor_timer.h"

boost_strand::boost_strand()
#ifdef ENABLE_NEXT_TICK
:_pCheckDestroy(NULL), _beginNextRound(false), _thisRoundCount(64), _nextTickAlloc(8192), _nextTickQueue1(8192), _nextTickQueue2(8192), _frontTickQueue(&_nextTickQueue1), _backTickQueue(&_nextTickQueue2)
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
	assert(!res->running_in_this_thread());
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
#ifdef ENABLE_NEXT_TICK
	return _strand->empty() && (checkTick ? _nextTickQueue1.empty() && _nextTickQueue2.empty() : true);
#else
	return _strand->empty();
#endif
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
	return _strand->ready_empty();
}

bool boost_strand::waiting_empty()
{
	return _strand->waiting_empty();
}

void boost_strand::run_tick_front()
{
	assert(_beginNextRound);
	_thisRoundCount = 64;
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
		size_t tickCount = 0;
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
		} while (!_backTickQueue->empty() && ++tickCount <= _thisRoundCount);
		std::swap(_frontTickQueue, _backTickQueue);
		if (!_frontTickQueue->empty() && waiting_empty())
		{
			post([lockThis]{});
		}
	}
}
#endif //ENABLE_NEXT_TICK

#if (defined ENABLE_MFC_ACTOR || defined ENABLE_WX_ACTOR || defined ENABLE_QT_ACTOR)
void boost_strand::_post(const std::function<void()>& h)
{
	assert(false);
}

void boost_strand::_post(std::function<void()>&& h)
{
	assert(false);
}
#endif