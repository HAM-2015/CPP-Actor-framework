#include "uv_strand.h"

#ifdef ENABLE_UV_ACTOR

uv_strand::uv_tls::uv_tls()
:_uvStack(64)
{
	_count = 0;
	memset(_tlsBuff, 0, sizeof(_tlsBuff));
}

uv_strand::uv_tls::~uv_tls()
{
	assert(0 == _count);
}

void uv_strand::uv_tls::init()
{
	void** const tlsBuff = io_engine::getTlsValueBuff();
	if (tlsBuff)
	{
		uv_tls*& uvTls = (uv_tls*&)tlsBuff[UV_TLS_INDEX];
		assert(uvTls);
		uvTls->_count++;
	}
	else
	{
		uv_tls* uvTls = new uv_tls();
		uvTls->_count++;
		uvTls->_tlsBuff[UV_TLS_INDEX] = uvTls;
		io_engine::setTlsBuff(uvTls->_tlsBuff);
	}
}

void uv_strand::uv_tls::reset()
{
	void** const tlsBuff = io_engine::getTlsValueBuff();
	assert(tlsBuff);
	uv_tls* uvTls = (uv_tls*)tlsBuff[UV_TLS_INDEX];
	if (0 == --uvTls->_count)
	{
		assert(tlsBuff == uvTls->_tlsBuff);
		io_engine::setTlsBuff(NULL);
		delete uvTls;
	}
}

uv_strand::uv_tls* uv_strand::uv_tls::push_stack(uv_strand* s)
{
	assert(s);
	uv_tls* uvTls = (uv_tls*)io_engine::getTlsValue(UV_TLS_INDEX);
	assert(uvTls);
	uvTls->_uvStack.push_front(s);
	return uvTls;
}

uv_strand* uv_strand::uv_tls::pop_stack()
{
	return pop_stack((uv_tls*)io_engine::getTlsValue(UV_TLS_INDEX));
}

uv_strand* uv_strand::uv_tls::pop_stack(uv_tls* uvTls)
{
	assert(uvTls && uvTls == (uv_tls*)io_engine::getTlsValue(UV_TLS_INDEX));
	assert(!uvTls->_uvStack.empty());
	uv_strand* r = uvTls->_uvStack.front();
	uvTls->_uvStack.pop_front();
	return r;
}

bool uv_strand::uv_tls::running_in_this_thread(uv_strand* s)
{
	void** const tlsBuff = io_engine::getTlsValueBuff();
	if (tlsBuff && tlsBuff[UV_TLS_INDEX])
	{
		uv_tls* uvTls = (uv_tls*)tlsBuff[UV_TLS_INDEX];
		for (uv_strand* const ele : uvTls->_uvStack)
		{
			if (ele == s)
			{
				return true;
			}
		}
	}
	return false;
}

size_t uv_strand::uv_tls::running_depth()
{
	size_t dp = 0;
	void** const tlsBuff = io_engine::getTlsValueBuff();
	if (tlsBuff && tlsBuff[UV_TLS_INDEX])
	{
		uv_tls* uvTls = (uv_tls*)tlsBuff[UV_TLS_INDEX];
		dp = uvTls->_uvStack.size();
	}
	return dp;
}
//////////////////////////////////////////////////////////////////////////

uv_strand::uv_strand()
{
	_uvLoop = NULL;
	_waitCount = 0;
	_locked = false;
	_doRunWait = false;
	_doRunSign = false;
	_waitClose = false;
	_uvReq = new uv_work_t;
	_uvReq->data = NULL;
#if (ENABLE_QT_ACTOR && ENABLE_UV_ACTOR)
	_strandChoose = boost_strand::strand_uv;
#endif
	uv_tls::init();
}

uv_strand::~uv_strand()
{
	assert(!_uvLoop && !_uvReq);
}

shared_uv_strand uv_strand::create(io_engine& ioEngine, uv_loop_t* uvLoop)
{
	shared_uv_strand res = std::make_shared<uv_strand>();
	res->_ioEngine = &ioEngine;
	res->_uvLoop = uvLoop;
	res->_threadID = run_thread::this_thread_id();
	res->_actorTimer = new ActorTimer_(res);
	res->_overTimer = new overlap_timer(res);
	res->_weakThis = res;
	return res;
}

void uv_strand::release()
{
	assert(in_this_ios());
	assert(!running_in_this_thread());
	assert(!_locked);
	assert(!_doRunWait);
	assert(!_doRunSign);
	assert(!_waitClose);
	assert(!_waitCount);
	assert(_readyQueue.empty());
	assert(_waitQueue.empty());
	if (_uvReq->data)
	{
		_uvReq->data = NULL;
	}
	else
	{
		delete _uvReq;
	}
	_uvReq = NULL;
	_uvLoop = NULL;
	uv_tls::reset();
}

bool uv_strand::released()
{
	assert(in_this_ios());
	return !_uvLoop;
}

shared_strand uv_strand::clone()
{
	assert(_uvLoop);
	return create(*_ioEngine, _uvLoop);
}

bool uv_strand::in_this_ios()
{
	return run_thread::this_thread_id() == _threadID;
}

bool uv_strand::running_in_this_thread()
{
	assert(_uvLoop);
	return uv_tls::running_in_this_thread(this);
}

bool uv_strand::only_self()
{
	assert(running_in_this_thread());
	return 1 == uv_tls::running_depth();
}

bool uv_strand::sync_safe()
{
	return true;
}

bool uv_strand::is_running()
{
	return true;
}

void uv_strand::post_task_event()
{
	assert(!_uvReq->data);
	_uvReq->data = this;
	uv_queue_work(_uvLoop, _uvReq, [](uv_work_t*){}, [](uv_work_t* uv_op, int status)
	{
		if (uv_op->data)
		{
			uv_strand* this_ = (uv_strand*)uv_op->data;
			assert(this_->in_this_ios());
			uv_op->data = NULL;
			this_->run_one_task();
		}
		else
		{
			delete uv_op;
		}
	});
}

void uv_strand::append_task(wrap_handler_face* h)
{
	assert(_uvLoop);
	{
		std::lock_guard<std::mutex> lg(_queueMutex);
		if (_locked)
		{
			_waitQueue.push_back(h);
			return;
		}
		else
		{
			_locked = true;
			_readyQueue.push_back(h);
			if (_doRunWait)
			{
				_doRunWait = false;
				_doRunConVar.notify_one();
				return;
			}
		}
	}
	post_task_event();
}

void uv_strand::run_one_task()
{
	uv_tls* uvTls = uv_tls::push_stack(this);
	while (!_readyQueue.empty())
	{
		wrap_handler_face* h = static_cast<wrap_handler_face*>(_readyQueue.pop_front());
		h->invoke();
		_reuMem.deallocate(h);
	}
	_queueMutex.lock();
	if (!_waitQueue.empty())
	{
		_waitQueue.swap(_readyQueue);
		_queueMutex.unlock();
		post_task_event();
	}
	else
	{
		_locked = false;
		_queueMutex.unlock();
	}
	uv_strand* r = uv_tls::pop_stack(uvTls);
	assert(this == r);
}

void uv_strand::enter_wait_close()
{
	assert(in_this_ios());
	if (!_waitCount)
	{
		_doRunSign = true;
	}
	uv_tls* uvTls = uv_tls::push_stack(this);
	_waitClose = true;
	do
	{
		while (!_readyQueue.empty())
		{
			wrap_handler_face* h = static_cast<wrap_handler_face*>(_readyQueue.pop_front());
			h->invoke();
			_reuMem.deallocate(h);
		}
		std::unique_lock<std::mutex> ul(_queueMutex);
		if (!_waitQueue.empty())
		{
			_waitQueue.swap(_readyQueue);
		}
		else if (!_doRunSign)
		{
			_locked = false;
			_doRunWait = true;
			_doRunConVar.wait(ul);
		}
		else
		{
			_doRunSign = false;
			_locked = false;
			break;
		}
	} while (true);
	_waitClose = false;
	uv_strand* r = uv_tls::pop_stack(uvTls);
	assert(this == r);
}

uv_loop_t* uv_strand::uv_loop()
{
	return _uvLoop;
}

void uv_strand::check_close()
{
	assert(in_this_ios());
	assert(_waitCount > 0);
	if (0 == --_waitCount && _waitClose)
	{
		_doRunSign = true;
	}
}

std::function<void()> uv_strand::wrap_check_close()
{
	assert(in_this_ios());
	_waitCount++;
	return FUNCTION_ALLOCATOR(std::function<void()>, wrap_post_once([this]
	{
		check_close();
	}), (reusable_alloc<void, reusable_mem_mt<>>(_reuMem)));
}

bool uv_strand::is_wait_close()
{
	assert(in_this_ios());
	return _waitClose;
}

#endif