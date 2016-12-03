#include "generator.h"

mem_alloc_base* generator::_genObjAlloc = NULL;
std::atomic<long long>* generator::_id = NULL;
any_accept generator::__anyAccept;

void generator::install(std::atomic<long long>* id)
{
	_genObjAlloc = make_shared_space_alloc<generator, mem_alloc_mt<void>>(MEM_POOL_LENGTH, [](generator*){});
	_id = id;
}

void generator::uninstall()
{
	delete _genObjAlloc;
	_genObjAlloc = NULL;
	_id = NULL;
}

generator::generator()
: __ctx(NULL), __coNext(0), __lockStop(0), __readyQuit(false), __asyncSign(false)
#if (_DEBUG || DEBUG)
, _isRun(false), __inside(false), __awaitSign(false), __sharedAwaitSign(false), __yieldSign(false)
#endif
{
}

generator::~generator()
{
	assert(!__ctx);
	assert(_callStack.empty());
}

bool generator::_next()
{
	assert(_strand->running_in_this_thread());
	assert(_handler);
	assert(!__ctx || !__inside);
	DEBUG_OPERATION(if (__ctx) __inside = true);
	CHECK_EXCEPTION(_handler, *this);
	assert(!__ctx || __coNext);
	if (!__ctx)
	{
		if (_callStack.empty())
		{
			_strand->actor_timer()->cancel(_timerHandle);
			clear_function(_handler);
			if (_notify)
			{
				CHECK_EXCEPTION(_notify);
				clear_function(_notify);
			}
			_sharedThis.reset();
			return true;
		}
		call_stack_pck& topStack = _callStack.front();
		__ctx = topStack._ctx;
		__coNext = -1 == __coNext ? __coNext : topStack._coNext;
		_handler = std::move(topStack._handler);
		_callStack.pop_front();
		return _next();
	}
	else if (0 > __coNext)
	{
		DEBUG_OPERATION(__inside = false);
		__ctx = NULL;
		if (-2 == __coNext)
		{
			__coNext = 0;
		}
		else
		{
			assert(-1 == __coNext);
			assert(!_callStack.empty());
		}
		return _next();
	}
	return false;
}

generator_handle generator::create(shared_strand strand, std::function<void(generator&)> handler, std::function<void()> notify)
{
	void* space = _genObjAlloc->allocate();
	generator_handle res(new(space)generator(), [](generator* p)
	{
		p->~generator();
	}, ref_count_alloc2<generator>(space, _genObjAlloc));
	res->_weakThis = res;
	res->_strand = std::move(strand);
	res->_handler = std::move(handler);
	res->_notify = std::move(notify);
	return res;
}

long long generator::alloc_id()
{
	return ++(*_id);
}

void generator::run()
{
	if (_strand->running_in_this_thread())
	{
		assert(!_isRun);
		DEBUG_OPERATION(_isRun = true);
		_next();
	}
	else
	{
		_strand->post(std::bind([](generator_handle& host)
		{
			assert(!host->_isRun);
			DEBUG_OPERATION(host->_isRun = true);
			host->_next();
		}, _weakThis.lock()));
	}
}

void generator::stop()
{
	if (_strand->running_in_this_thread())
	{
		if (__ctx)
		{
			if (!__lockStop)
			{
				bool inside = 0 == __coNext;
				assert(inside == __inside);
				__coNext = -1;
				if (!inside)
				{
					_next();
				}
			}
			else
			{
				__readyQuit = true;
			}
		}
		else
		{
			clear_function(_handler);
			clear_function(_notify);
		}
	} 
	else
	{
		_strand->post(std::bind([](generator_handle& host)
		{
			if (host->__ctx)
			{
				host->__coNext = -1;
				host->_next();
			}
			else
			{
				clear_function(host->_handler);
				clear_function(host->_notify);
			}
		}, _weakThis.lock()));
	}
}

void generator::_lockThis()
{
	if (!_sharedThis)
	{
		_sharedThis = _weakThis.lock();
	}
}

generator* generator::_revert_this(generator_handle& s)
{
	assert(s);
	_sharedThis = std::move(s);
	return _sharedThis.get();
}

generator_handle& generator::shared_this()
{
	assert(_sharedThis);
	return _sharedThis;
}

generator_handle& generator::async_this()
{
	assert(_sharedThis);
	assert(__inside);
	assert(!__awaitSign && !__sharedAwaitSign);
	DEBUG_OPERATION(__awaitSign = true);
	return _sharedThis;
}

const shared_bool& generator::shared_async_sign()
{
	assert(__ctx);
	assert(__inside);
	assert(!__awaitSign);
	DEBUG_OPERATION(__sharedAwaitSign = true);
	if (__sharedSign.empty())
	{
		__sharedSign = shared_bool::new_(false);
	}
	return __sharedSign;
}

const shared_strand& generator::gen_strand()
{
	return _strand;
}

void generator::_co_next()
{
	_strand->distribute(std::bind([](generator_handle& host)
	{
		if (host->__ctx)
		{
			assert(!host->__asyncSign);
			host->_revert_this(host)->_next();
		}
	}, std::move(shared_this())));
}

void generator::_co_tick_next()
{
	_strand->next_tick(std::bind([](generator_handle& host)
	{
		if (host->__ctx)
		{
			assert(!host->__asyncSign);
			host->_revert_this(host)->_next();
		}
	}, std::move(shared_this())));
}

void generator::_co_async_next()
{
	_strand->distribute(std::bind([](generator_handle& host)
	{
		if (host->__ctx)
		{
			generator* host_ = host->_revert_this(host);
			if (host_->__asyncSign)
			{
				host_->__asyncSign = false;
				host_->_next();
			}
			else
			{
				host_->__asyncSign = true;
			}
		}
	}, std::move(shared_this())));
}

void generator::_co_async_next2()
{
	assert(_strand->running_in_this_thread());
	assert(__ctx);
	if (__asyncSign)
	{
		__asyncSign = false;
		_next();
	}
	else
	{
		__asyncSign = true;
	}
}

void generator::_co_reset_shared_sign()
{
	if (!__sharedSign.empty())
	{
		__sharedSign = true;
		__sharedSign.reset();
	}
}

void generator::_co_shared_async_next(shared_bool& sign)
{
	_strand->distribute(std::bind([](generator_handle& host, shared_bool& sign)
	{
		if (sign)
		{
			return;
		}
		if (host->__ctx)
		{
			if (host->__asyncSign)
			{
				host->__asyncSign = false;
				host->_next();
			}
			else
			{
				host->__asyncSign = true;
			}
		}
	}, _weakThis.lock(), std::move(sign)));
}

void generator::_co_push_stack(int coNext, std::function<void(generator&)>&& handler)
{
	_callStack.push_front(call_stack_pck(coNext, __ctx, std::move(_handler)));
	_handler = std::move(handler);
}

bool generator::_done()
{
	assert(_strand->running_in_this_thread());
	return !__ctx;
}

void generator::_co_sleep(int ms)
{
	_co_usleep((long long)ms * 1000);
}

void generator::_co_usleep(long long us)
{
	assert(us > 0);
	assert(_strand->running_in_this_thread());
	assert(_timerHandle.is_null());
	_timerHandle = _strand->actor_timer()->timeout(us, _weakThis.lock());
}

void generator::_co_dead_sleep(long long ms)
{
	_co_dead_usleep(ms * 1000);
}

void generator::_co_dead_usleep(long long us)
{
	assert(_strand->running_in_this_thread());
	assert(_timerHandle.is_null());
	_timerHandle = _strand->actor_timer()->timeout(us, _weakThis.lock(), true);
}

void generator::timeout_handler()
{
	assert(__ctx);
	_timerHandle.reset();
	_next();
}