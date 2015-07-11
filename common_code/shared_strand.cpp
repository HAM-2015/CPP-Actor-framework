#include "shared_strand.h"
#include "actor_timer.h"

boost_strand::boost_strand()
:_pCheckDestroy(NULL), _nextTickAlloc(64), _nextTickQueue(64)
{
	_iosProxy = NULL;
	_strand = NULL;
	_timer = NULL;
}

boost_strand::~boost_strand()
{
	assert(_nextTickQueue.empty());
	delete _strand;
	delete _timer;
	if (_pCheckDestroy)
	{
		*_pCheckDestroy = true;
	}
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

void boost_strand::run_tick()
{
	assert(!_nextTickQueue.empty());
	shared_strand lockThis = _weakThis.lock();
	int tickCount = 0;
	do
	{
		if (++tickCount == 64)
		{
			struct null_invoke { void operator()()const {}; };
			post(null_invoke());
			break;
		}
		wrap_next_tick_base* tick = _nextTickQueue.front();
		_nextTickQueue.pop_front();
		tick->invoke();
		if (void* tp = tick->destroy())
		{
			_nextTickAlloc.deallocate(tp);
		}
	} while (!_nextTickQueue.empty());
}

#if (defined ENABLE_MFC_ACTOR || defined ENABLE_WX_ACTOR)
void boost_strand::_post( const std::function<void ()>& h )
{
	assert(false);
}
#endif