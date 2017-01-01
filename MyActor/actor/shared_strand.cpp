#include "shared_strand.h"
#include "actor_timer.h"
#include "async_timer.h"

#define NEXT_TICK_SPACE_SIZE (sizeof(void*)*8)

boost_strand::boost_strand()
:_ioEngine(NULL), _strand(NULL), _actorTimer(NULL)
#ifdef ENABLE_NEXT_TICK
,_thisRoundCount(0)
,_reuMemAlloc(NULL)
#endif //ENABLE_NEXT_TICK
#if (ENABLE_QT_ACTOR && ENABLE_UV_ACTOR)
,_strandChoose(strand_default)
#endif
{
#ifdef ENABLE_NEXT_TICK
	_nextTickAlloc[0] = NULL;
	_nextTickAlloc[1] = NULL;
	_nextTickAlloc[2] = NULL;
#endif
}

boost_strand::~boost_strand()
{
#ifdef ENABLE_NEXT_TICK
	assert(_frontTickQueue.empty());
	assert(_backTickQueue.empty());
	assert(!_strand || (ready_empty() && waiting_empty()));
	delete _nextTickAlloc[0];
	delete _nextTickAlloc[1];
	delete _nextTickAlloc[2];
	delete _reuMemAlloc;
#endif //ENABLE_NEXT_TICK
	delete _actorTimer;
	delete _overTimer;
	delete _strand;
}

shared_strand boost_strand::create(io_engine& ioEngine)
{
	shared_strand res = ioEngine._strandPool->pick();
	res->_weakThis = res;
	if (!res->_ioEngine)
	{
		res->_ioEngine = &ioEngine;
		res->_strand = new strand_type(ioEngine);
#ifdef ENABLE_NEXT_TICK
		res->_reuMemAlloc = new reusable_mem();
		res->_nextTickAlloc[0] = new mem_alloc2<char[NEXT_TICK_SPACE_SIZE]>(ioEngine._poolSize);
		res->_nextTickAlloc[1] = new mem_alloc2<char[NEXT_TICK_SPACE_SIZE * 2]>(ioEngine._poolSize / 2);
		res->_nextTickAlloc[2] = new mem_alloc2<char[NEXT_TICK_SPACE_SIZE * 4]>(ioEngine._poolSize / 4);
#endif
		res->_actorTimer = new ActorTimer_(res);
		res->_overTimer = new overlap_timer(res);
	}
	return res;
}

std::vector<shared_strand> boost_strand::create_multi(size_t n, io_engine& ioEngine)
{
	assert(0 != n);
	std::vector<shared_strand> res(n);
	for (size_t i = 0; i < n; i++)
	{
		res[i] = boost_strand::create(ioEngine);
	}
	return res;
}

void boost_strand::create_multi(shared_strand* res, size_t n, io_engine& ioEngine)
{
	assert(0 != n);
	for (size_t i = 0; i < n; i++)
	{
		res[i] = boost_strand::create(ioEngine);
	}
}

void boost_strand::create_multi(std::vector<shared_strand>& res, size_t n, io_engine& ioEngine)
{
	assert(0 != n);
	res.resize(n);
	for (size_t i = 0; i < n; i++)
	{
		res[i] = boost_strand::create(ioEngine);
	}
}

void boost_strand::create_multi(std::list<shared_strand>& res, size_t n, io_engine& ioEngine)
{
	assert(0 != n);
	res.clear();
	for (size_t i = 0; i < n; i++)
	{
		res.push_front(boost_strand::create(ioEngine));
	}
}

shared_strand boost_strand::clone()
{
	assert(_ioEngine);
	return create(*_ioEngine);
}

bool boost_strand::in_this_ios()
{
	assert(_ioEngine);
	return _ioEngine->runningInThisIos();
}

bool boost_strand::sync_safe()
{
	assert(_ioEngine);
	return 1 == _ioEngine->threadNumber();
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
	return _strand->empty() && (checkTick ? _frontTickQueue.empty() && _backTickQueue.empty() : true);
#else
	return _strand->empty();
#endif
}

bool boost_strand::is_running()
{
	assert(_strand);
	return _strand->running();
}

bool boost_strand::safe_is_running()
{
	assert(_strand);
	return _strand->safe_running();
}

size_t boost_strand::ios_thread_number()
{
	assert(_ioEngine);
	return _ioEngine->threadNumber();
}

bool boost_strand::only_self()
{
	assert(_strand);
	return _strand->only_self();
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

ActorTimer_* boost_strand::actor_timer()
{
	return _actorTimer;
}

overlap_timer* boost_strand::over_timer()
{
	return _overTimer;
}

std::shared_ptr<AsyncTimer_> boost_strand::make_timer()
{
	std::shared_ptr<AsyncTimer_> res = std::make_shared<AsyncTimer_>(_actorTimer);
	res->_weakThis = res;
	return res;
}

#ifdef ENABLE_NEXT_TICK
bool boost_strand::ready_empty()
{
	assert(_strand);
	return _strand->ready_empty();
}

bool boost_strand::waiting_empty()
{
	assert(_strand);
	return _strand->waiting_empty();
}

void boost_strand::push_next_tick(op_queue::face* handler)
{
	_backTickQueue.push_back(handler);
}

void boost_strand::run_tick_front()
{
	if (_thisRoundCount)
	{
		return;
	}
	while (!_frontTickQueue.empty())
	{
		wrap_next_tick_face* const tick = static_cast<wrap_next_tick_face*>(_frontTickQueue.pop_front());
		const size_t spaceSize = tick->invoke();
		switch (MEM_ALIGN(spaceSize, NEXT_TICK_SPACE_SIZE) / NEXT_TICK_SPACE_SIZE)
		{
		case 1: _nextTickAlloc[0]->deallocate(tick); break;
		case 2: _nextTickAlloc[1]->deallocate(tick); break;
		case 3: case 4: _nextTickAlloc[2]->deallocate(tick); break;
		default: _reuMemAlloc->deallocate(tick); break;
		}
	}
}

void boost_strand::run_tick_back()
{
	_thisRoundCount++;
	if (!ready_empty())
	{
		return;
	}
	size_t tickCount = _thisRoundCount;
	_thisRoundCount = 0;
	while (!_backTickQueue.empty() && tickCount--)
	{
		wrap_next_tick_face* const tick = static_cast<wrap_next_tick_face*>(_backTickQueue.pop_front());
		const size_t spaceSize = tick->invoke();
		switch (MEM_ALIGN(spaceSize, NEXT_TICK_SPACE_SIZE) / NEXT_TICK_SPACE_SIZE)
		{
		case 1: _nextTickAlloc[0]->deallocate(tick); break;
		case 2: _nextTickAlloc[1]->deallocate(tick); break;
		case 3: case 4: _nextTickAlloc[2]->deallocate(tick); break;
		default: _reuMemAlloc->deallocate(tick); break;
		}
	}
	if (!_backTickQueue.empty())
	{
		_backTickQueue.swap(_frontTickQueue);
		if (waiting_empty())
		{
			post(any_handler());
		}
	}
}

void* boost_strand::alloc_space(size_t size)
{
	switch (MEM_ALIGN(size, NEXT_TICK_SPACE_SIZE) / NEXT_TICK_SPACE_SIZE)
	{
	case 1: return !_nextTickAlloc[0]->overflow() ? _nextTickAlloc[0]->allocate() : NULL;
	case 2: return !_nextTickAlloc[1]->overflow() ? _nextTickAlloc[1]->allocate() : NULL;
	case 3: case 4: return !_nextTickAlloc[2]->overflow() ? _nextTickAlloc[2]->allocate() : NULL;
	}
	return NULL;
}

#endif //ENABLE_NEXT_TICK