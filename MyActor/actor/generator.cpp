#include "generator.h"

mem_alloc_mt<generator>* generator::_genObjAlloc = NULL;
mem_alloc_base* generator::_genObjRefCountAlloc = NULL;

void generator::install()
{
	_genObjAlloc = new mem_alloc_mt<generator>(MEM_POOL_LENGTH);
	mem_alloc_mt<generator>* genObjAlloc_ = _genObjAlloc;
	_genObjRefCountAlloc = make_ref_count_alloc<generator, mem_alloc_mt<void>>(MEM_POOL_LENGTH, [genObjAlloc_](generator*){});
}

void generator::uninstall()
{
	delete _genObjRefCountAlloc;
	_genObjRefCountAlloc = NULL;
	delete _genObjAlloc;
	_genObjAlloc = NULL;
}

generator::generator()
: _ctx(NULL)
{
	DEBUG_OPERATION(_isRun = false);
}

generator::~generator()
{
	assert(!_ctx);
}

bool generator::_next()
{
	assert(_strand->running_in_this_thread());
	assert(_handler);
	assert(!_ctx || !_ctx->__inside);
	DEBUG_OPERATION(if (_ctx) _ctx->__inside = true);
	CHECK_EXCEPTION(_handler, *this);
	assert(!_ctx || _ctx->__coNext);
	if (!_ctx)
	{
		clear_function(_handler);
		if (_notify)
		{
			CHECK_EXCEPTION(_notify);
			clear_function(_notify);
		}
		_sharedThis.reset();
		return true;
	}
	return false;
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
		if (_ctx)
		{
			if (!_ctx->__lockStop)
			{
				bool inside = 0 == _ctx->__coNext;
				assert(inside == _ctx->__inside);
				_ctx->__coNext = -1;
				if (!inside)
				{
					_next();
				}
			}
			else
			{
				_ctx->__readyQuit = true;
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
			if (host->_ctx)
			{
				host->_ctx->__coNext = -1;
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
	assert(_ctx->__inside);
	assert(!_ctx->__awaitSign && !_ctx->__sharedAwaitSign);
	DEBUG_OPERATION(_ctx->__awaitSign = true);
	return _sharedThis;
}

const shared_bool& generator::shared_async_sign()
{
	assert(_ctx);
	assert(_ctx->__inside);
	assert(!_ctx->__awaitSign);
	DEBUG_OPERATION(_ctx->__sharedAwaitSign = true);
	if (_ctx->_sharedSign.empty())
	{
		_ctx->_sharedSign = shared_bool::new_(false);
	}
	return _ctx->_sharedSign;
}

const shared_strand& generator::gen_strand()
{
	return _strand;
}

generator_handle generator::_begin_fork(std::function<void()> notify)
{
	return generator::create(_strand, _handler, std::move(notify));
}

void generator::_co_next()
{
	_strand->distribute(std::bind([](generator_handle& host)
	{
		if (host->_ctx)
		{
			assert(!host->_ctx->__asyncSign);
			host->_revert_this(host)->_next();
		}
	}, std::move(shared_this())));
}

void generator::_co_tick_next()
{
	_strand->next_tick(std::bind([](generator_handle& host)
	{
		if (host->_ctx)
		{
			assert(!host->_ctx->__asyncSign);
			host->_revert_this(host)->_next();
		}
	}, std::move(shared_this())));
}

void generator::_co_async_next()
{
	_strand->distribute(std::bind([](generator_handle& host)
	{
		struct co_context_base* __ctx = host->_ctx;
		if (__ctx)
		{
			generator* host_ = host->_revert_this(host);
			if (__ctx->__asyncSign)
			{
				__ctx->__asyncSign = false; host_->_next();
			}
			else
			{
				__ctx->__asyncSign = true;
			}
		}
	}, std::move(shared_this())));
}

void generator::_co_shared_async_next(shared_bool& sign)
{
	_strand->distribute(std::bind([](generator_handle& host, shared_bool& sign)
	{
		if (sign)
		{
			return;
		}
		struct co_context_base* __ctx = host->_ctx;
		if (__ctx)
		{
			if (__ctx->__asyncSign)
			{
				__ctx->__asyncSign = false; host->_next();
			}
			else
			{
				__ctx->__asyncSign = true;
			}
		}
	}, _weakThis.lock(), std::move(sign)));
}
//////////////////////////////////////////////////////////////////////////

void CoNotifyHandlerFace_::notify_state(reusable_mem& alloc, CoNotifyHandlerFace_* ntf, co_async_state state /*= co_async_ok*/)
{
	__space_align char space[sizeof(void*)* 256];
	assert(sizeof(space) >= ntf->size());
	CoNotifyHandlerFace_* ntf_ = ntf->move_out(space);
	alloc.deallocate(ntf);
	ntf_->notify(state);
}