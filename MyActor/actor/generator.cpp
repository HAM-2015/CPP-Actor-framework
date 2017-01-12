#include "generator.h"

mem_alloc_base* generator::_genObjAlloc = NULL;
std::atomic<long long>* generator::_id = NULL;
any_accept generator::__anyAccept;

void generator::install(std::atomic<long long>* id)
{
	_genObjAlloc = make_shared_space_alloc<generator, mem_alloc_tls<GENERATOR_ALLOC_INDEX, void>>(MEM_POOL_LENGTH, [](generator*){});
	_id = id;
}

void generator::uninstall()
{
	delete _genObjAlloc;
	_genObjAlloc = NULL;
	_id = NULL;
}

void generator::tls_init()
{
	_genObjAlloc->tls_init();
}

void generator::tls_uninit()
{
	_genObjAlloc->tls_uninit();
}

generator::generator()
: __ctx(NULL), __coNext(0), __coNextEx(0), __lockStop(0), __readyQuit(false), __asyncSign(false), __yieldSign(false)
#if (_DEBUG || DEBUG)
, _isRun(false), __inside(false), __awaitSign(false), __sharedAwaitSign(false)
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
	assert(_baseHandler);
	assert(!__ctx || !__inside);
	DEBUG_OPERATION(if (__ctx) __inside = true);
	__yieldSign = true;
	CHECK_EXCEPTION(!_callStack.empty() ? _callStack.front() : _baseHandler, *this);
	assert(!__ctx || __coNext);
	if (!__ctx)
	{
		if (_callStack.empty())
		{
			if (!_sharedSign.empty())
			{
				_sharedSign = true;
				_sharedSign.reset();
			}
			_strand->actor_timer()->cancel(_timerHandle);
			clear_function(_baseHandler);
			if (_notify)
			{
				CHECK_EXCEPTION(std::function<void()>(std::move(_notify)));
			}
			_sharedThis.reset();
			return true;
		}
		call_stack_pck& topStack = _callStack.front();
		__ctx = topStack._ctx;
		__coNextEx = topStack._coNextEx;
		__coNext = -1 == __coNext ? __coNext : topStack._coNext;
		_callStack.pop_front();
		return _done() ? true : _next();
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

generator_handle generator::create(shared_strand strand, co_function handler, std::function<void()> notify)
{
	void* space = _genObjAlloc->allocate();
	generator_handle res(new(space)generator(), [](generator* p)
	{
		p->~generator();
	}, ref_count_alloc2<generator>(space, _genObjAlloc));
	res->_weakThis = res;
	res->_strand = std::move(strand);
	res->_baseHandler = std::move(handler);
	res->_notify = std::move(notify);
	return res;
}

generator_handle generator::create(shared_strand strand, co_function handler, generator_done_sign& doneSign)
{
	assert(!doneSign._notified);
	if (!doneSign._strand)
	{
		doneSign._strand = strand;
	}
	return generator::create(std::move(strand), std::move(handler), [&doneSign]()
	{
		doneSign.notify_all();
	});
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
			clear_function(_baseHandler);
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
				clear_function(host->_baseHandler);
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
	__yieldSign = false;
	return _sharedThis;
}

generator_handle& generator::async_this()
{
	assert(_sharedThis);
	assert(__inside);
	assert(!__awaitSign && !__sharedAwaitSign);
	DEBUG_OPERATION(__awaitSign = true);
	__yieldSign = false;
	return _sharedThis;
}

const shared_bool& generator::shared_async_sign()
{
	assert(__ctx);
	assert(__inside);
	assert(!__awaitSign);
	DEBUG_OPERATION(__sharedAwaitSign = true);
	if (_sharedSign.empty())
	{
		_sharedSign = shared_bool::new_(false);
	}
	return _sharedSign;
}

const shared_strand& generator::self_strand()
{
	return _strand;
}

void generator::_co_next()
{
	assert(_strand->running_in_this_thread());
	if (__ctx)
	{
		assert(!__asyncSign);
		_next();
	}
	else
	{
		_sharedThis.reset();
	}
}

void generator::_co_tick_next()
{
	_strand->next_tick(std::bind([](generator_handle& host)
	{
		generator* const host_ = host.get();
		if (host_->__ctx)
		{
			assert(!host_->__asyncSign);
			host_->_revert_this(host)->_next();
		}
	}, std::move(_sharedThis)));
}

void generator::_co_async_next()
{
	if (_strand->running_in_this_thread())
	{
		if (__ctx)
		{
			_co_top_next();
		}
		else
		{
			_sharedThis.reset();
		}
	}
	else
	{
		_strand->post(std::bind([](generator_handle& host)
		{
			generator* const host_ = host.get();
			if (host_->__ctx)
			{
				host_->_revert_this(host)->_co_top_next();
			}
		}, std::move(_sharedThis)));
	}
}

void generator::_co_top_next()
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

void generator::_co_asio_next()
{
	_strand->dispatch(std::bind([](generator_handle& host)
	{
		assert(host->_strand->only_self());
		generator* const host_ = host.get();
		if (host_->__ctx)
		{
			host_->_revert_this(host)->_co_top_next();
		}
	}, std::move(_sharedThis)));
}

void generator::_co_shared_async_next(shared_bool& sign)
{
	if (sign.empty() || sign)
	{
		return;
	}
	if (_strand->running_in_this_thread())
	{
		assert(_sharedSign == sign && __ctx);
		_sharedSign = true;
		_sharedSign.reset();
		_co_top_next();
	}
	else
	{
		_strand->post(std::bind([](generator_handle& host, shared_bool& sign)
		{
			assert(!sign.empty());
			if (!sign)
			{
				generator* const host_ = host.get();
				assert(host_->_sharedSign == sign && host_->__ctx);
				host_->_sharedSign = true;
				host_->_sharedSign.reset();
				host_->_co_top_next();
			}
		}, _weakThis.lock(), std::move(sign)));
	}
}

void generator::_co_push_stack(int coNext, co_function&& handler)
{
	_callStack.push_front(call_stack_pck(coNext, __coNextEx, __ctx, std::move(handler)));
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
//////////////////////////////////////////////////////////////////////////

generator_done_sign::generator_done_sign()
: _notified(false)
{
}

generator_done_sign::generator_done_sign(shared_strand strand)
: _strand(std::move(strand)), _notified(false)
{
}

generator_done_sign::~generator_done_sign()
{
	assert(_notified);
	assert(_waitList.empty());
}

void generator_done_sign::append_listen(std::function<void()> notify)
{
	assert(_strand);
	if (_strand->running_in_this_thread())
	{
		if (_notified)
		{
			CHECK_EXCEPTION(notify);
		}
		else
		{
			_waitList.push_back(std::move(notify));
		}
	}
	else
	{
		_strand->post(std::bind([this](std::function<void()>& notify)
		{
			if (_notified)
			{
				CHECK_EXCEPTION(notify);
			}
			else
			{
				_waitList.push_back(std::move(notify));
			}
		}, std::move(notify)));
	}
}

void generator_done_sign::notify_all()
{
	assert(_strand);
	_strand->distribute([this]()
	{
		_notified = true;
		std::list<std::function<void()>> waitList(std::move(_waitList));
		while (!waitList.empty())
		{
			CHECK_EXCEPTION(waitList.front());
			waitList.pop_front();
		}
	});
}

const shared_strand& generator_done_sign::self_strand()
{
	return _strand;
}
//////////////////////////////////////////////////////////////////////////

CoGo_::CoGo_(shared_strand strand, std::function<void()> ntf)
:_strand(std::move(strand)), _ntf(std::move(ntf))
{
}

CoGo_::CoGo_(io_engine& ios, std::function<void()> ntf)
:_strand(boost_strand::create(ios)), _ntf(std::move(ntf))
{
}

CoGo_::CoGo_(shared_strand strand, generator_done_sign& doneSign)
:_strand(std::move(strand)), _ntf([&doneSign]()
{
	doneSign.notify_all();
})
{
	assert(!doneSign._notified);
	if (!doneSign._strand)
	{
		doneSign._strand = _strand;
	}
}

CoGo_::CoGo_(io_engine& ios, generator_done_sign& doneSign)
:_strand(boost_strand::create(ios)), _ntf([&doneSign]()
{
	doneSign.notify_all();
})
{
	assert(!doneSign._notified);
	if (!doneSign._strand)
	{
		doneSign._strand = _strand;
	}
}
//////////////////////////////////////////////////////////////////////////

CoCreate_::CoCreate_(shared_strand strand, std::function<void()> ntf)
:_strand(std::move(strand)), _ntf(std::move(ntf))
{
}

CoCreate_::CoCreate_(io_engine& ios, std::function<void()> ntf)
:_strand(boost_strand::create(ios)), _ntf(std::move(ntf))
{
}

CoCreate_::CoCreate_(shared_strand strand, generator_done_sign& doneSign)
:_strand(std::move(strand)), _ntf([&doneSign]()
{
	doneSign.notify_all();
})
{
	assert(!doneSign._notified);
	if (!doneSign._strand)
	{
		doneSign._strand = _strand;
	}
}

CoCreate_::CoCreate_(io_engine& ios, generator_done_sign& doneSign)
:_strand(boost_strand::create(ios)), _ntf([&doneSign]()
{
	doneSign.notify_all();
})
{
	assert(!doneSign._notified);
	if (!doneSign._strand)
	{
		doneSign._strand = _strand;
	}
}