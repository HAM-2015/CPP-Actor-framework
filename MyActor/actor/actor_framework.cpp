#include "actor_framework.h"
#include "async_buffer.h"
#include "sync_msg.h"
#include "context_pool.h"

typedef ContextPool_::coro_push_interface actor_push_type;
typedef ContextPool_::coro_pull_interface actor_pull_type;

template <typename Handler>
void make_coro(actor_pull_type* pull, Handler&& handler)
{
	pull->_param = &handler;
	pull->_currentHandler = [](actor_push_type& push, void* p)
	{
		std::function<void(actor_push_type&)> mh((Handler&&)(*(Handler*)p));
		mh(push);
	};
	(*pull)();
}
//////////////////////////////////////////////////////////////////////////

#ifdef CHECK_SELF
#ifndef ENABLE_TLS_CHECK_SELF
msg_map<void*, my_actor*> s_stackLine(100000);
std::mutex s_stackLineMutex;
struct initStackLine
{
	initStackLine()
	{
		s_stackLine.insert(make_pair((void*)NULL, (my_actor*)NULL));
		s_stackLine.insert(make_pair((void*)-1, (my_actor*)NULL));
	}
} s_initStackLine;
#endif
#endif

#define CORO_CONTEXT_STATE_SPACE	(8 kB)

typedef unsigned long long dirty_data_type;
#define DIRTY_DATA ((dirty_data_type)0xF7F6F5F4F3F2F1F0LL)

std::atomic<my_actor::id> s_actorIDCount(0);//ID计数
std::shared_ptr<shared_obj_pool<bool>> s_sharedBoolPool(create_shared_pool<bool>(100000, [](void*){}));
msg_list_shared_alloc<actor_handle>::shared_node_alloc my_actor::_childActorListAll(100000);
msg_list_shared_alloc<std::function<void()> >::shared_node_alloc my_actor::_quitExitCallbackAll(100000);
msg_list_shared_alloc<my_actor::suspend_resume_option>::shared_node_alloc my_actor::_suspendResumeQueueAll(100000);
msg_map_shared_alloc<my_actor::msg_pool_status::id_key, std::shared_ptr<my_actor::msg_pool_status::pck_base> >::shared_node_alloc my_actor::msg_pool_status::_msgTypeMapAll(100000);

#ifdef ENABLE_CHECK_LOST
mem_alloc_mt<CheckLost_> s_checkLostObjAlloc(100000);
mem_alloc_mt<CheckPumpLost_> s_checkPumpLostObjAlloc(100000);
mem_alloc_base* s_checkLostRefCountAlloc = NULL;
mem_alloc_base* s_checkPumpLostRefCountAlloc = NULL;

struct makeLostRefCountAlloc
{
	makeLostRefCountAlloc()
	{
		auto& s_checkLostObjAlloc_ = s_checkLostObjAlloc;
		auto& s_checkPumpLostObjAlloc_ = s_checkPumpLostObjAlloc;
		s_checkLostRefCountAlloc = make_ref_count_alloc<CheckLost_, std::mutex>(100000, [&s_checkLostObjAlloc_](CheckLost_*){});
		s_checkPumpLostRefCountAlloc = make_ref_count_alloc<CheckPumpLost_, std::mutex>(100000, [&s_checkPumpLostObjAlloc_](CheckPumpLost_*){});
	}

	~makeLostRefCountAlloc()
	{
		delete s_checkLostRefCountAlloc;
		delete s_checkPumpLostRefCountAlloc;
	}
} s_makeLostRefCountAlloc;
#endif

//////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_CHECK_LOST
CheckLost_::CheckLost_(const shared_strand& strand, actor_msg_handle_base* msgHandle)
:_strand(strand), _handle(msgHandle), _closed(msgHandle->_closed) {}

CheckLost_::~CheckLost_()
{
	if (!(*_closed))
	{
		auto& closed_ = _closed;
		auto& handle_ = _handle;
		_strand->try_tick([closed_, handle_]()
		{
			if (!(*closed_))
			{
				handle_->lost_msg();
			}
		});
	}
}

//////////////////////////////////////////////////////////////////////////
CheckPumpLost_::CheckPumpLost_(const actor_handle& hostActor, MsgPoolBase_* pool)
:_hostActor(hostActor), _pool(pool) {}

CheckPumpLost_::~CheckPumpLost_()
{
	_pool->lost_msg(_hostActor);
}
#endif

//////////////////////////////////////////////////////////////////////////

child_actor_handle::child_actor_handle(const child_actor_handle&)
{
	assert(false);
}

child_actor_handle::child_actor_handle(child_actor_handle&& s)
:_started(false), _quited(true)
{
	*this = std::move(s);
}

child_actor_handle::child_actor_handle(const actor_handle& actor)
:_actor(actor), _started(false), _quited(false)
{
	my_actor* parent = _actor->_parentActor.get();
	parent->_childActorList.push_front(_actor);
	_actorIt = parent->_childActorList.begin();
	_actor->_exitCallback.push_front(parent->make_trig_notifer_to_self(_quiteAth));
	_athIt = _actor->_exitCallback.begin();
}

child_actor_handle::child_actor_handle()
:_started(false), _quited(true)
{
}

void child_actor_handle::operator=(const child_actor_handle&)
{
	assert(false);
}

void child_actor_handle::operator =(child_actor_handle&& s)
{
	assert(!_started && _quited);
	if (!s->_quited)
	{
		my_actor* parent = s._actor->_parentActor.get();
		assert(parent->_strand->running_in_this_thread());
		assert(parent->_inActor);
		assert(!s._started);
		assert(!s._actor->_started);
		_actor = s._actor;
		_quited = s._quited;
		_actorIt = s._actorIt;
		s._actor.reset();
		s._started = false;
		s._quited = true;
		_started = false;
		_quited = false;
		_actor->_exitCallback.erase(s._athIt);
		parent->close_trig_notifer(s._quiteAth);
		_actor->_exitCallback.push_front(parent->make_trig_notifer_to_self(_quiteAth));
		_athIt = _actor->_exitCallback.begin();
	}
}

child_actor_handle::~child_actor_handle()
{
	if (!_quited)
	{
		my_actor* parent = _actor->_parentActor.get();
		assert(parent->_strand->running_in_this_thread());
		if (!parent->is_quited())
		{
			assert(parent->_inActor);
			assert(_started);
#ifdef _MSC_VER
			parent->wait_trig(_quiteAth);
#elif __GNUG__
			try
			{
				parent->wait_trig(_quiteAth);
			}
			catch (...) {}
#endif
			peel();
		}
	}
}

const actor_handle& child_actor_handle::get_actor() const
{
#if (_DEBUG || DEBUG)
	if (_actor)
	{
		my_actor* parent = _actor->_parentActor.get();
		assert(parent->_strand->running_in_this_thread());
		assert(parent->_inActor);
	}
#endif
	return _actor;
}

my_actor* child_actor_handle::operator ->() const
{
	assert(_actor);
	my_actor* parent = _actor->_parentActor.get();
	assert(parent->_strand->running_in_this_thread());
	assert(parent->_inActor);
	return _actor.get();
}

void child_actor_handle::peel()
{
	if (!_quited)
	{
		my_actor* parent = _actor->_parentActor.get();
		assert(parent->_strand->running_in_this_thread());
		assert(parent->_inActor);
		parent->_childActorList.erase(_actorIt);
		_actor.reset();
		_started = false;
		_quited = true;
	}
}

child_actor_handle::ptr child_actor_handle::make_ptr()
{
	return ptr(new child_actor_handle);
}

bool child_actor_handle::empty() const
{
#if (_DEBUG || DEBUG)
	if (_actor)
	{
		my_actor* parent = _actor->_parentActor.get();
		assert(parent->_strand->running_in_this_thread());
		assert(parent->_inActor);
	}
#endif
	return _quited;
}

//////////////////////////////////////////////////////////////////////////
actor_msg_handle_base::actor_msg_handle_base()
:_waiting(false), _losted(false), _checkLost(false), _hostActor(NULL)
{

}

void actor_msg_handle_base::set_actor(const actor_handle& hostActor)
{
	_hostActor = hostActor.get();
	DEBUG_OPERATION(_strand = hostActor->self_strand());
}

void actor_msg_handle_base::check_lost(bool checkLost)
{
	assert(_strand->running_in_this_thread());
#ifndef ENABLE_CHECK_LOST
	assert(!checkLost);
#endif
	_checkLost = checkLost;
}

void actor_msg_handle_base::lost_msg()
{
	assert(_strand->running_in_this_thread());
	if (!ActorFunc_::is_quited(_hostActor))
	{
		_losted = true;
		if (_waiting && _checkLost)
		{
			_waiting = false;
			ActorFunc_::pull_yield(_hostActor);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
long long MutexBlock_::actor_id(my_actor* host)
{
	assert(host);
	return host->self_id();
}

//////////////////////////////////////////////////////////////////////////

MsgPoolVoid_::MsgPoolVoid_(const shared_strand& strand)
{
	_strand = strand;
	_waiting = false;
	_closed = false;
	_losted = false;
	_sendCount = 0;
	_msgBuff = 0;
}

MsgPoolVoid_::~MsgPoolVoid_()
{

}

MsgPoolVoid_::pump_handler MsgPoolVoid_::connect_pump(const std::shared_ptr<msg_pump_type>& msgPump, bool& losted)
{
	assert(msgPump);
	assert(_strand->running_in_this_thread());
	_msgPump = msgPump;
	pump_handler compHandler;
	compHandler._thisPool = _weakThis.lock();
	compHandler._msgPump = msgPump;
	_waiting = false;
	_sendCount = 0;
	losted = _losted && !_msgBuff;
	return compHandler;
}

void MsgPoolVoid_::push_msg(const actor_handle& hostActor)
{
	//if (_closed) return;

	if (_strand->running_in_this_thread())
	{
		post_msg(hostActor);
	}
	else
	{
		auto shared_this = _weakThis.lock();
		_strand->post([=]
		{
			shared_this->send_msg(hostActor);
		});
	}
}

void MsgPoolVoid_::lost_msg(const actor_handle& hostActor)
{
	if (!_closed)
	{
		auto shared_this = _weakThis.lock();
		_strand->try_tick([shared_this, hostActor]()
		{
			if (!shared_this->_closed && shared_this->_msgPump)
			{
				shared_this->_msgPump->lost_msg(hostActor);
			}
			else
			{
				shared_this->_losted = true;
			}
		});
	}
}

void MsgPoolVoid_::send_msg(const actor_handle& hostActor)
{
	//if (_closed) return;

	_losted = false;
	if (_waiting)
	{
		_waiting = false;
		assert(_msgPump);
		assert(0 == _msgBuff);
		_sendCount++;
		_msgPump->receive_msg(hostActor);
	}
	else
	{
		_msgBuff++;
	}
}

void MsgPoolVoid_::post_msg(const actor_handle& hostActor)
{
	_losted = false;
	if (_waiting)
	{
		_waiting = false;
		assert(_msgPump);
		assert(0 == _msgBuff);
		_sendCount++;
		_msgPump->receive_msg_tick(hostActor);
	}
	else
	{
		_msgBuff++;
	}
}

void MsgPoolVoid_::disconnect()
{
	assert(_strand->running_in_this_thread());
	_msgPump.reset();
	_waiting = false;
}

void MsgPoolVoid_::backflow(stack_obj<msg_type>& suck)
{
	assert(_strand->running_in_this_thread());
	assert(suck.has());
	_msgBuff++;
}

void MsgPoolVoid_::pump_handler::clear()
{
	_thisPool.reset();
	_msgPump.reset();
}

bool MsgPoolVoid_::pump_handler::empty()
{
	return !_thisPool;
}

void MsgPoolVoid_::pump_handler::pump_msg(unsigned char pumpID, const actor_handle& hostActor)
{
	assert(_msgPump == _thisPool->_msgPump);
	if (!_thisPool->_waiting)//上次取消息超时后取消了等待，此时取还没消息
	{
		if (pumpID == _thisPool->_sendCount)
		{
			if (_thisPool->_msgBuff)
			{
				_thisPool->_msgBuff--;
				_thisPool->_sendCount++;
				_msgPump->receive_msg(hostActor);
			}
#ifdef ENABLE_CHECK_LOST
			else if (_thisPool->_losted)
			{
				_msgPump->lost_msg(hostActor);
			}
#endif
			else
			{
				_thisPool->_waiting = true;
			}
		}
		else
		{//上次消息没取到，重新取，但实际中间已经post出去了
			assert(!_thisPool->_waiting);
			assert(pumpID + 1 == _thisPool->_sendCount);
		}
	}
	else
	{
		assert(!_thisPool->_msgBuff);
		assert(pumpID == _thisPool->_sendCount);
	}
}

bool MsgPoolVoid_::pump_handler::try_pump(my_actor* host, unsigned char pumpID, bool& wait)
{
	assert(_thisPool);
	auto& refThis_ = *this;
	host->lock_quit();
	bool r = host->send<bool>(_thisPool->_strand, [&wait, pumpID, refThis_]()->bool
	{
		bool ok = false;
		auto& thisPool_ = refThis_._thisPool;
		if (refThis_._msgPump == thisPool_->_msgPump)
		{
			if (pumpID == thisPool_->_sendCount)
			{
				if (thisPool_->_msgBuff)
				{
					thisPool_->_msgBuff--;
					ok = true;
				}
#ifdef ENABLE_CHECK_LOST
				else// if (thisPool_->_losted)
				{
					wait = thisPool_->_losted;
				}
#endif
			}
			else
			{//上次消息没取到，重新取，但实际中间已经post出去了
				assert(!thisPool_->_waiting);
				assert(pumpID + 1 == thisPool_->_sendCount);
				wait = true;
				ok = true;
			}
		}
		return ok;
	});
	host->unlock_quit();
	return r;
}

size_t MsgPoolVoid_::pump_handler::size(my_actor* host, unsigned char pumpID)
{
	assert(_thisPool);
	auto& refThis_ = *this;
	host->lock_quit();
	size_t r = host->send<size_t>(_thisPool->_strand, [pumpID, refThis_]()->size_t
	{
		auto& thisPool_ = refThis_._thisPool;
		if (refThis_._msgPump == thisPool_->_msgPump)
		{
			if (pumpID == thisPool_->_sendCount)
			{
				return thisPool_->_msgBuff;
			}
			else
			{
				return thisPool_->_msgBuff + 1;
			}
		}
		return 0;
	});
	host->unlock_quit();
	return r;
}

size_t MsgPoolVoid_::pump_handler::snap_size(unsigned char pumpID)
{
	assert(_thisPool);
	if (_thisPool->_strand->running_in_this_thread())
	{
		if (_msgPump == _thisPool->_msgPump)
		{
			if (pumpID == _thisPool->_sendCount)
			{
				return _thisPool->_msgBuff;
			}
			else
			{
				return _thisPool->_msgBuff + 1;
			}
		}
		return 0;
	}
	return _thisPool->_msgBuff;
}

void MsgPoolVoid_::pump_handler::post_pump(unsigned char pumpID)
{
	assert(!empty());
	auto& refThis_ = *this;
	auto hostActor = _msgPump->_hostActor->shared_from_this();
	_thisPool->_strand->post([=]
	{
		if (refThis_._msgPump == refThis_._thisPool->_msgPump)
		{
			((pump_handler&)refThis_).pump_msg(pumpID, hostActor);
		}
	});
}

void MsgPoolVoid_::pump_handler::start_pump(unsigned char pumpID)
{
	assert(_thisPool);
	if (_thisPool->_strand->running_in_this_thread())
	{
		if (_msgPump == _thisPool->_msgPump)
		{
			pump_msg(pumpID, _msgPump->_hostActor->shared_from_this());
		}
	}
	else
	{
		post_pump(pumpID);
	}
}
//////////////////////////////////////////////////////////////////////////

MsgPumpVoid_::MsgPumpVoid_(const actor_handle& hostActor, bool checkLost)
{
#ifndef ENABLE_CHECK_LOST
	assert(!checkLost);
#endif
	_waiting = false;
	_waitConnect = false;
	_hasMsg = false;
	_checkDis = false;
	_losted = false;
	_checkLost = checkLost;
	_dstRec = NULL;
	_pumpCount = 0;
	_hostActor = hostActor.get();
	_strand = hostActor->self_strand();
}

MsgPumpVoid_::~MsgPumpVoid_()
{
	assert(!_hasMsg);
}

void MsgPumpVoid_::clear()
{
	assert(_strand->running_in_this_thread());
	_pumpHandler.clear();
	if (!ActorFunc_::is_quited(_hostActor) && _checkDis)
	{
		assert(_waiting);
		_waiting = false;
		_dstRec = NULL;
		ActorFunc_::pull_yield(_hostActor);
	}
}

void MsgPumpVoid_::close()
{
	assert(_strand->running_in_this_thread());
	_hasMsg = false;
	_waiting = false;
	_waitConnect = false;
	_checkDis = false;
	_losted = false;
	_checkLost = false;
	_dstRec = NULL;
	_pumpCount = 0;
	_pumpHandler.clear();
	_hostActor = NULL;
}

void MsgPumpVoid_::backflow(stack_obj<msg_type>& suck)
{
	if (_hasMsg)
	{
		_hasMsg = false;
		suck.create();
	}
}

void MsgPumpVoid_::connect(const pump_handler& pumpHandler, const bool& losted)
{
	assert(_strand->running_in_this_thread());
	assert(_hostActor);
	_pumpHandler = pumpHandler;
	_pumpCount = 0;
	_losted = losted;
	if (_waiting)
	{
#ifdef ENABLE_CHECK_LOST
		if (_losted && _checkLost)
		{
			_waiting = false;
			ActorFunc_::pull_yield(_hostActor);
		} 
		else
#endif
		{
			_pumpHandler.start_pump(_pumpCount);
		}
	}
	else if (_waitConnect)
	{
		_waitConnect = false;
		ActorFunc_::pull_yield(_hostActor);
	}
}

void MsgPumpVoid_::receiver()
{
	if (_hostActor)
	{
		assert(!_hasMsg);
		_losted = false;
		_pumpCount++;
		if (_waiting)
		{
			_waiting = false;
			_checkDis = false;
			if (_dstRec)
			{
				*_dstRec = true;
				_dstRec = NULL;
			}
			ActorFunc_::pull_yield(_hostActor);
		}
		else
		{
			_hasMsg = true;
		}
	}
}

void MsgPumpVoid_::lost_msg(const actor_handle& hostActor)
{
	if (_strand->running_in_this_thread())
	{
		_losted = true;
		if (_waiting && _checkLost)
		{
			_waiting = false;
			_checkDis = false;
			ActorFunc_::pull_yield(_hostActor);
		}
	}
	else
	{
		auto shared_this = _weakThis.lock();
		_strand->post([shared_this, hostActor]()
		{
			shared_this->_losted = true;
			if (shared_this->_waiting && shared_this->_checkLost)
			{
				shared_this->_waiting = false;
				shared_this->_checkDis = false;
				ActorFunc_::pull_yield(shared_this->_hostActor);
			}
		});
	}
}

bool MsgPumpVoid_::read_msg()
{
	assert(_strand->running_in_this_thread());
	assert(!_waiting);
	assert(!_dstRec);
	if (_hasMsg)
	{
		_hasMsg = false;
		return true;
	}
	if (!_pumpHandler.empty())
	{
		_pumpHandler.start_pump(_pumpCount);
		_waiting = !_hasMsg;
		_hasMsg = false;
		return !_waiting;
	}
	_waiting = true;
	return false;
}

bool MsgPumpVoid_::read_msg(bool& dst)
{
	assert(_strand->running_in_this_thread());
	assert(!_dstRec);
	assert(!_waiting);
	if (_hasMsg)
	{
		_hasMsg = false;
		dst = true;
		return true;
	}
	_dstRec = &dst;
	if (!_pumpHandler.empty())
	{
		_pumpHandler.start_pump(_pumpCount);
		_waiting = !_hasMsg;
		if (_hasMsg)
		{
			_hasMsg = false;
			*_dstRec = true;
			_dstRec = NULL;
		}
		return !_waiting;
	}
	_waiting = true;
	return false;
}

bool MsgPumpVoid_::try_read()
{
	assert(_strand->running_in_this_thread());
	assert(!_waiting);
	assert(!_dstRec);
	if (_hasMsg)
	{
		_hasMsg = false;
		return true;
	}
#ifdef ENABLE_CHECK_LOST
	if (!_losted && !_pumpHandler.empty())
#else
	if (!_pumpHandler.empty())
#endif
	{
		bool wait = false;
		if (_pumpHandler.try_pump(_hostActor, _pumpCount, wait))
		{
			if (wait)
			{
				if (!_hasMsg)
				{
					_waiting = true;
					ActorFunc_::push_yield(_hostActor);
					assert(_hasMsg);
					assert(!_waiting);
				}
				_hasMsg = false;
			}
			return true;
		}
#ifdef ENABLE_CHECK_LOST
		_losted = wait;
#endif
	}
	return false;
}

size_t MsgPumpVoid_::size()
{
	assert(_strand->running_in_this_thread());
	if (!_pumpHandler.empty())
	{
		return (_hasMsg ? 1 : 0) + _pumpHandler.size(_hostActor, _pumpCount);
	}
	return 0;
}

size_t MsgPumpVoid_::snap_size()
{
	assert(_strand->running_in_this_thread());
	if (!_pumpHandler.empty())
	{
		return (_hasMsg ? 1 : 0) + _pumpHandler.snap_size(_pumpCount);
	}
	return 0;
}

void MsgPumpVoid_::stop_waiting()
{
	_waiting = false;
	_waitConnect = false;
	_checkDis = false;
	_dstRec = NULL;
}

void MsgPumpVoid_::receive_msg(const actor_handle& hostActor)
{
	if (_strand->running_in_this_thread())
	{
		receiver();
	}
	else
	{
		auto shared_this = _weakThis.lock();
		_strand->post([shared_this, hostActor]
		{
			shared_this->receiver();
		});
	}
}

void MsgPumpVoid_::receive_msg_tick(const actor_handle& hostActor)
{
	auto shared_this = _weakThis.lock();
	_strand->try_tick([shared_this, hostActor]
	{
		shared_this->receiver();
	});
}

bool MsgPumpVoid_::isDisconnected()
{
	return _pumpHandler.empty();
}
//////////////////////////////////////////////////////////////////////////

void TrigOnceBase_::tick_handler(bool* sign) const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->tick_handler(sign);
	reset();
}

void TrigOnceBase_::dispatch_handler(bool* sign) const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->dispatch_handler(sign);
	reset();
}

void TrigOnceBase_::operator =(const TrigOnceBase_&)
{
	assert(false);
}
//////////////////////////////////////////////////////////////////////////
template <typename _Ty>
struct actor_ref_count_alloc;

template<>
class actor_ref_count_alloc<void>
{
public:
	typedef size_t      size_type;
	typedef void*       pointer;
	typedef const void* const_pointer;
	typedef void        value_type;

	template<class Other>
	struct rebind
	{
		typedef actor_ref_count_alloc<Other> other;
	};

	actor_ref_count_alloc(ContextPool_::coro_pull_interface* pull)
		:_pull(pull) {}

	ContextPool_::coro_pull_interface* _pull;
};

template <typename _Ty>
struct actor_ref_count_alloc
{
	typedef size_t     size_type;
	typedef _Ty*       pointer;
	typedef const _Ty* const_pointer;
	typedef _Ty&       reference;
	typedef const _Ty& const_reference;
	typedef _Ty        value_type;

	template<class Other>
	struct rebind
	{
		typedef actor_ref_count_alloc<Other> other;
	};

	actor_ref_count_alloc()
	{
	}

	template<class Other>
	actor_ref_count_alloc(const actor_ref_count_alloc<Other>& s)
		:_pull(s._pull)
	{
	}

	template<class Other>
	bool operator==(const actor_ref_count_alloc<Other>& s)
	{
		_pull = s._pull;
		return true;
	}

	void deallocate(pointer _Ptr, size_type _Count)
	{
		assert(1 == _Count);
		ContextPool_::recovery(_pull);
	}

	pointer allocate(size_type _Count)
	{
		assert(1 == _Count);
		assert(sizeof(my_actor)+sizeof(_Ty) <= _pull->_spaceSize);
		return (pointer)((char*)_pull->_space + sizeof(my_actor));
	}

	void construct(_Ty *_Ptr, const _Ty& _Val)
	{
		new ((void *)_Ptr) _Ty(_Val);
	}

	void construct(_Ty *_Ptr, _Ty&& _Val)
	{
		new ((void *)_Ptr) _Ty(std::move(_Val));
	}

	template<class _Uty>
	void destroy(_Uty *_Ptr)
	{
		_Ptr->~_Uty();
	}

	size_t max_size() const
	{
		return ((size_t)(-1) / sizeof (_Ty));
	}

	ContextPool_::coro_pull_interface* _pull;
};
//////////////////////////////////////////////////////////////////////////

class my_actor::boost_actor_run
{
public:
	boost_actor_run(my_actor& actor)
		: _actor(actor)
	{

	}

	void operator ()(actor_push_type& actorPush)
	{
		assert(!_actor._quited);
		assert(_actor._mainFunc);
		_actor._actorPush = &actorPush;
		try
		{
			{
				_actor._yieldCount++;
				_actor._inActor = false;
				(*(actor_push_type*)_actor._actorPush)();
				if (_actor._notifyQuited)
				{
					throw force_quit_exception();
				}
				_actor._inActor = true;
			}
			assert(_actor._strand->running_in_this_thread());
			_actor._mainFunc(&_actor);
			assert(_actor._inActor);
			assert(_actor._childActorList.empty());
			assert(_actor._quitHandlerList.empty());
			assert(_actor._suspendResumeQueue.empty());
		}
		catch (my_actor::force_quit_exception&)
		{//捕获Actor被强制退出异常
			assert(!_actor._inActor);
			_actor._inActor = true;
		}
		catch (my_actor::pump_disconnected_exception&)
		{
			assert(false);
			exit(11);
		}
		catch (std::shared_ptr<std::string> msg)
		{
			assert(false);
			exit(12);
		}
		catch (async_buffer_close_exception&)
		{
			assert(false);
			exit(13);
		}
		catch (sync_csp_exception&)
		{
			assert(false);
			exit(14);
		}
		catch (ntf_lost_exception&)
		{
			assert(false);
			exit(15);
		}
		catch (boost::exception&)
		{
			assert(false);
			exit(16);
		}
		catch (std::exception&)
		{
			assert(false);
			exit(17);
		}
		catch (...)
		{
			assert(false);
			exit(-1);
		}
		_actor._quited = true;
		_actor._notifyQuited = true;
		clear_function(_actor._mainFunc);
		clear_function(_actor._timerState._timerCb);
		if (_actor._timer && !_actor._timerState._timerCompleted)
		{
			_actor._timerState._timerCompleted = true;
			if (_actor._timerState._timerTime)
			{
				_actor._timer->cancel(_actor._timerState._timerHandle);
			}
		}
		_actor._msgPoolStatus.clear(&_actor);//yield now
		_actor._inActor = false;
		_actor._exited = true;
		DEBUG_OPERATION(size_t yc = _actor.yield_count());
		while (!_actor._exitCallback.empty())
		{
			assert(_actor._exitCallback.front());
			CHECK_EXCEPTION(_actor._exitCallback.front());
			_actor._exitCallback.pop_front();
		}
		assert(_actor.yield_count() == yc);
	}
private:
	my_actor& _actor;
};

my_actor::my_actor()
:_suspendResumeQueue(_suspendResumeQueueAll),
_exitCallback(_quitExitCallbackAll),
_quitHandlerList(_quitExitCallbackAll),
_childActorList(_childActorListAll)
{
	_actorPull = NULL;
	_actorPush = NULL;
	_timer = NULL;
	_quited = false;
	_exited = false;
	_started = false;
	_inActor = false;
	_suspended = false;
	_hasNotify = false;
	_isForce = false;
	_notifyQuited = false;
	_timerState._timerSuspend = false;
	_timerState._timerCompleted = true;
	_lockQuit = 0;
	_stackTop = NULL;
	_stackSize = 0;
	_yieldCount = 0;
	_lastYield = -1;
	_childOverCount = 0;
	_childSuspendResumeCount = 0;
#ifdef ENABLE_CHECK_FUNC_STACK
	_checkStackDepth = 0;
#endif
	_timerState._timerCount = 0;
	_timerState._timerTime = 0;
	_timerState._timerStampBegin = 0;
	_timerState._timerStampEnd = 0;
	_selfID = ++s_actorIDCount;
}

my_actor::my_actor(const my_actor&)
{

}

my_actor::~my_actor()
{
	assert(_quited);
	assert(_exited);
	assert(!_mainFunc);
	assert(!_childOverCount);
	assert(!_lockQuit);
	assert(!_childSuspendResumeCount);
	assert(_suspendResumeQueue.empty());
	assert(_quitHandlerList.empty());
	assert(_suspendResumeQueue.empty());
	assert(_exitCallback.empty());
	assert(_childActorList.empty());
#ifdef CHECK_SELF
#ifndef ENABLE_TLS_CHECK_SELF
	s_stackLineMutex.lock();
	s_stackLine.erase(_btIt);
	s_stackLine.erase(_topIt);
	s_stackLineMutex.unlock();
#endif
#endif
#ifdef CHECK_ACTOR_STACK
	dirty_data_type* sp = (dirty_data_type*)((char*)_stackTop - _stackSize - STACK_RESERVED_SPACE_SIZE);
	size_t i = 0;
	while (DIRTY_DATA == *(sp++))
	{
		i += sizeof(dirty_data_type);
	}
	if (_checkStackFree || STACK_RESERVED_SPACE_SIZE >= i)
	{
		stack_overflow_format(STACK_RESERVED_SPACE_SIZE - (int)i, _createStack);
	}
#endif
}

my_actor& my_actor::operator =(const my_actor&)
{
	return *this;
}

actor_handle my_actor::create(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	return my_actor::create(actorStrand, main_func(mainFunc), stackSize);
}

actor_handle my_actor::create(const shared_strand& actorStrand, main_func&& mainFunc, size_t stackSize)
{
	actor_pull_type* pull = ContextPool_::getContext(stackSize + STACK_RESERVED_SPACE_SIZE);
	actor_handle newActor(new(pull->_space)my_actor(), [](my_actor* p){p->~my_actor(); }, actor_ref_count_alloc<void>(pull));
	newActor->_weakThis = newActor;
	newActor->_mainFunc = std::move(mainFunc);
	newActor->_strand = actorStrand;
	newActor->_timer = actorStrand->get_timer();
	newActor->_stackTop = pull->_coroInfo->stackTop;
	newActor->_actorPull = pull;
	newActor->_stackSize = stackSize;
	make_coro(pull, boost_actor_run(*newActor));

#ifdef CHECK_SELF
#ifndef ENABLE_TLS_CHECK_SELF
	s_stackLineMutex.lock();
	newActor->_topIt = s_stackLine.insert(make_pair((char*)newActor->_stackTop, (my_actor*)NULL)).first;
	newActor->_btIt = s_stackLine.insert(newActor->_topIt, make_pair((char*)newActor->_stackTop - newActor->_stackSize - STACK_RESERVED_SPACE_SIZE, newActor.get()));
	s_stackLineMutex.unlock();
#endif
#endif

#ifdef CHECK_ACTOR_STACK
	newActor->_checkStackFree = false;
	size_t sl = (newActor->_stackSize + STACK_RESERVED_SPACE_SIZE - CORO_CONTEXT_STATE_SPACE) / sizeof(dirty_data_type);
	dirty_data_type* sp = (dirty_data_type*)((char*)newActor->_stackTop - newActor->_stackSize - STACK_RESERVED_SPACE_SIZE);
	for (size_t i = 0; i < sl; i++)
	{
		sp[i] = DIRTY_DATA;
	}
	newActor->_createStack = std::shared_ptr<list<stack_line_info>>(new list<stack_line_info>(get_stack_list(8, 1)));
#endif
	return newActor;
}

child_actor_handle my_actor::create_child_actor(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	assert_enter();
	actor_handle childActor = my_actor::create(actorStrand, mainFunc, stackSize);
	childActor->_parentActor = shared_from_this();
	return child_actor_handle(childActor);
}

child_actor_handle my_actor::create_child_actor(const shared_strand& actorStrand, main_func&& mainFunc, size_t stackSize)
{
	assert_enter();
	actor_handle childActor = my_actor::create(actorStrand, std::move(mainFunc), stackSize);
	childActor->_parentActor = shared_from_this();
	return child_actor_handle(childActor);
}

child_actor_handle my_actor::create_child_actor(const main_func& mainFunc, size_t stackSize)
{
	return create_child_actor(_strand, mainFunc, stackSize);
}

child_actor_handle my_actor::create_child_actor(main_func&& mainFunc, size_t stackSize)
{
	return create_child_actor(_strand, std::move(mainFunc), stackSize);
}

void my_actor::child_actor_run(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(!actorHandle._started);
	assert(!actorHandle._quited);
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
	actorHandle._started = true;
	actorHandle._actor->notify_run();
}

void my_actor::child_actors_run(list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		child_actor_run(*actorHandle);
	}
}

void my_actor::child_actors_run(list<child_actor_handle>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		child_actor_run(actorHandle);
	}
}

void my_actor::child_actor_force_quit(child_actor_handle& actorHandle)
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());

		my_actor* actor = actorHandle._actor.get();
		if (actor->self_strand() == _strand)
		{
			actor->force_quit(std::function<void()>());
		}
		else
		{
			actor->notify_quit();
		}
		wait_trig(actorHandle._quiteAth);
		actorHandle.peel();
	}
}

void my_actor::child_actors_force_quit(list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		if (!actorHandle->_quited)
		{
			assert(actorHandle->get_actor());
			assert(actorHandle->get_actor()->parent_actor()->self_id() == self_id());
			my_actor* actor = actorHandle->_actor.get();
			if (actor->self_strand() == _strand)
			{
				actor->force_quit(std::function<void()>());
			}
			else
			{
				actor->notify_quit();
			}
		}
	}
	for (auto& actorHandle : actorHandles)
	{
		if (!actorHandle->_quited)
		{
			wait_trig(actorHandle->_quiteAth);
			actorHandle->peel();
		}
	}
}

void my_actor::child_actors_force_quit(list<child_actor_handle>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		if (!actorHandle._quited)
		{
			assert(actorHandle.get_actor());
			assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
			my_actor* actor = actorHandle._actor.get();
			if (actor->self_strand() == _strand)
			{
				actor->force_quit(std::function<void()>());
			}
			else
			{
				actor->notify_quit();
			}
		}
	}
	for (auto& actorHandle : actorHandles)
	{
		if (!actorHandle._quited)
		{
			wait_trig(actorHandle._quiteAth);
			actorHandle.peel();
		}
	}
}

void my_actor::child_actor_wait_quit(child_actor_handle& actorHandle)
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
		assert(actorHandle._started);
		wait_trig(actorHandle._quiteAth);
		actorHandle.peel();
	}
}

bool my_actor::timed_child_actor_wait_quit(int tm, child_actor_handle& actorHandle)
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
		if (!timed_wait_trig(tm, actorHandle._quiteAth))
		{
			return false;
		}
		actorHandle.peel();
	}
	return true;
}

void my_actor::child_actors_wait_quit(list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle->get_actor()->parent_actor()->self_id() == self_id());
		child_actor_wait_quit(*actorHandle);
	}
}

void my_actor::child_actors_wait_quit(list<child_actor_handle>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
		child_actor_wait_quit(actorHandle);
	}
}

void my_actor::child_actor_suspend(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
	my_actor* actor = actorHandle.get_actor().get();
	if (actorHandle.get_actor()->self_strand() == _strand)
	{
		trig([actor](const trig_once_notifer<>& h){actor->suspend(h); });
	}
	else
	{
		trig([actor](const trig_once_notifer<>& h){actor->notify_suspend(h); });
	}
}

void my_actor::child_actors_suspend(list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle->get_actor());
		assert(actorHandle->get_actor()->parent_actor()->self_id() == self_id());
		my_actor* actor = actorHandle->get_actor().get();
		if (actor->self_strand() == _strand)
		{
			actor->suspend(h);
		}
		else
		{
			actor->notify_suspend(h);
		}
	}
	for (size_t i = actorHandles.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
}

void my_actor::child_actors_suspend(list<child_actor_handle>& actorHandles)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
		my_actor* actor = actorHandle.get_actor().get();
		if (actor->self_strand() == _strand)
		{
			actor->suspend(h);
		}
		else
		{
			actor->notify_suspend(h);
		}
	}
	for (size_t i = actorHandles.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
}

void my_actor::child_actor_resume(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
	my_actor* actor = actorHandle.get_actor().get();
	if (actorHandle.get_actor()->self_strand() == _strand)
	{
		trig([actor](const trig_once_notifer<>& h){actor->resume(h); });
	}
	else
	{
		trig([actor](const trig_once_notifer<>& h){actor->notify_resume(h); });
	}
}

void my_actor::child_actors_resume(list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle->get_actor());
		assert(actorHandle->get_actor()->parent_actor()->self_id() == self_id());
		my_actor* actor = actorHandle->get_actor().get();
		if (actor->self_strand() == _strand)
		{
			actor->resume(h);
		}
		else
		{
			actor->notify_resume(h);
		}
	}
	for (size_t i = actorHandles.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
}

void my_actor::child_actors_resume(list<child_actor_handle>& actorHandles)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
		my_actor* actor = actorHandle.get_actor().get();
		if (actor->self_strand() == _strand)
		{
			actor->resume(h);
		}
		else
		{
			actor->notify_resume(h);
		}
	}
	for (size_t i = actorHandles.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
}

void my_actor::run_child_actor_complete(const shared_strand& actorStrand, const main_func& h, size_t stackSize)
{
	assert_enter();
	child_actor_handle actorHandle = create_child_actor(actorStrand, h, stackSize);
	child_actor_run(actorHandle);
	child_actor_wait_quit(actorHandle);
}

void my_actor::run_child_actor_complete(const main_func& h, size_t stackSize)
{
	run_child_actor_complete(self_strand(), h, stackSize);
}

void my_actor::sleep(int ms)
{
	assert_enter();
	if (ms < 0)
	{
		actor_handle lockActor = shared_from_this();
		push_yield();
	}
	else
	{
		if (0 == ms)
		{
			_strand->post(wrap_run_one(shared_from_this()));
		}
		else
		{
			assert(_timer);
			timeout(ms, [this]{run_one(); });
		}
		push_yield();
	}
}

void my_actor::sleep_guard(int ms)
{
	assert_enter();
	lock_quit();
	sleep(ms);
	unlock_quit();
}

void my_actor::yield()
{
	assert_enter();
	_strand->post(wrap_run_one(shared_from_this()));
	push_yield();
}

void my_actor::try_yield()
{
	if (_lastYield == _yieldCount)
	{
		yield();
	}
	_lastYield = _yieldCount;
}

void my_actor::yield_guard()
{
	assert_enter();
	lock_quit();
	_strand->next_tick(wrap_run_one(shared_from_this()));
	push_yield();
	unlock_quit();
}

void my_actor::try_yield_guard()
{
	if (_lastYield == _yieldCount)
	{
		yield_guard();
	}
	_lastYield = _yieldCount;
}

const actor_handle& my_actor::parent_actor()
{
	return _parentActor;
}

const msg_list_shared_alloc<actor_handle>& my_actor::child_actors()
{
	return _childActorList;
}

my_actor::quit_iterator my_actor::regist_quit_handler(const std::function<void()>& quitHandler)
{
	return regist_quit_handler(std::function<void()>(quitHandler));
}

my_actor::quit_iterator my_actor::regist_quit_handler(std::function<void()>&& quitHandler)
{
	assert_enter();
	_quitHandlerList.push_front(std::move(quitHandler));//后注册的先执行
	return _quitHandlerList.begin();
}

void my_actor::cancel_quit_handler(const quit_iterator& qh)
{
	assert_enter();
	_quitHandlerList.erase(qh);
}

void my_actor::cancel_delay_trig()
{
	assert_enter();
	cancel_timer();
}

void my_actor::dispatch_handler(bool* sign)
{
	_strand->dispatch(wrap_trig_run_one(shared_from_this(), sign));
}

void my_actor::tick_handler(bool* sign)
{
	_strand->try_tick(wrap_trig_run_one(shared_from_this(), sign));
}

void my_actor::next_tick_handler(bool* sign)
{
	_strand->next_tick(wrap_trig_run_one(shared_from_this(), sign));
}

const shared_strand& my_actor::self_strand()
{
	return _strand;
}

boost::asio::io_service& my_actor::self_io_service()
{
	return _strand->get_io_service();
}

actor_handle my_actor::shared_from_this()
{
	return _weakThis.lock();
}

my_actor::id my_actor::self_id()
{
	return _selfID;
}

size_t my_actor::yield_count()
{
	assert(_strand->running_in_this_thread());
	return _yieldCount;
}

void my_actor::reset_yield()
{
	assert_enter();
	_yieldCount = 0;
	_lastYield = -1;
}

void my_actor::notify_run()
{
	actor_handle shared_this = shared_from_this();
	_strand->try_tick([shared_this]
	{
		my_actor* self = shared_this.get();
		if (!self->_quited && !self->_started)
		{
			self->_started = true;
			self->pull_yield();
		}
	});
}

void my_actor::assert_enter()
{
	assert(_strand->running_in_this_thread());
	assert(!_quited);
	assert(_inActor);
	assert((size_t)get_sp() >= (size_t)_stackTop - _stackSize + 1024);
	check_self();
}

void my_actor::notify_quit()
{
	if (!_quited)
	{
		actor_handle shared_this = shared_from_this();
		_strand->try_tick([shared_this]{shared_this->force_quit(std::function<void()>()); });
	}
}

void my_actor::notify_quit(const std::function<void()>& h)
{
	notify_quit(std::function<void()>(h));
}

void my_actor::notify_quit(std::function<void()>&& h)
{
	struct wrap_quit
	{
		wrap_quit(const actor_handle& host, std::function<void()>&& h)
		:_sharedThis(host), _h(std::move(h)) {}

		wrap_quit(wrap_quit&& s)
			:_sharedThis(s._sharedThis), _h(std::move(s._h)) {}

		void operator()()
		{
			_sharedThis->force_quit(std::move(_h));
		}

		actor_handle _sharedThis;
		std::function<void()> _h;
	};
	_strand->try_tick(wrap_quit(shared_from_this(), std::move(h)));
}

void my_actor::force_quit(const std::function<void()>& h)
{
	force_quit(std::function<void()>(h));
}

void my_actor::force_quit(std::function<void()>&& h)
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);
	if (!_notifyQuited)
	{
		_notifyQuited = true;
		if (!_lockQuit)
		{
			_isForce = true;
			_quited = true;
			if (h) _exitCallback.push_back(std::move(h));
			if (!_childActorList.empty())
			{
				_childOverCount = _childActorList.size();
				while (!_childActorList.empty())
				{
					auto cc = _childActorList.front();
					_childActorList.pop_front();
					actor_handle shared_this = shared_from_this();
					if (cc->_strand == _strand)
					{
						cc->force_quit([shared_this]{shared_this->force_quit_cb_handler(); });
					}
					else
					{
						cc->notify_quit(_strand->wrap_post([shared_this]{shared_this->force_quit_cb_handler(); }));
					}
				}
			}
			else
			{
				exit_callback();
			}
			return;
		}
		else if (h)
		{
			_exitCallback.push_back(std::move(h));
		}
	}
	else if (h)
	{
		if (_exited)
		{
			DEBUG_OPERATION(size_t yc = yield_count());
			CHECK_EXCEPTION(h);
			assert(yield_count() == yc);
		}
		else
		{
			_exitCallback.push_back(std::move(h));
		}
	}
}

bool my_actor::is_started()
{
	assert(_strand->running_in_this_thread());
	return _started;
}

bool my_actor::is_quited()
{
	assert(_strand->running_in_this_thread());
	return _quited;
}

bool my_actor::is_force()
{
	assert(_quited);
	return _isForce;
}

bool my_actor::in_actor()
{
	assert(_strand->running_in_this_thread());
	return _inActor;
}

bool my_actor::quit_msg()
{
	assert_enter();
	return _notifyQuited;
}

void my_actor::lock_quit()
{
	assert_enter();
	_lockQuit++;
}

void my_actor::unlock_quit()
{
	assert_enter();
	assert(_lockQuit);
	if (0 == --_lockQuit && _notifyQuited)
	{
		_notifyQuited = false;
		notify_quit();
		push_yield();
	}
}

bool my_actor::is_locked_quit()
{
	assert_enter();
	return 0 != _lockQuit;
}

void my_actor::try_quit()
{
#ifdef __GNUG__
	if (_notifyQuited && 0 == _lockQuit)
	{
		throw force_quit_exception();
	}
#elif _MSC_VER
	assert(!(_notifyQuited && 0 == _lockQuit));
#endif
}

void my_actor::notify_suspend()
{
	notify_suspend(std::function<void()>());
}

void my_actor::notify_suspend(const std::function<void()>& h)
{
	notify_suspend(std::function<void()>(h));
}

void my_actor::notify_suspend(std::function<void()>&& h)
{
	struct wrap_suspend
	{
		wrap_suspend(const actor_handle& host, std::function<void()>&& h)
		:_sharedThis(host), _h(std::move(h)) {}

		wrap_suspend(wrap_suspend&& s)
			:_sharedThis(s._sharedThis), _h(std::move(s._h)) {}

		void operator()()
		{
			_sharedThis->suspend(std::move(_h));
		}

		actor_handle _sharedThis;
		std::function<void()> _h;
	};
	_strand->try_tick(wrap_suspend(shared_from_this(), std::move(h)));
}

void my_actor::suspend(const std::function<void()>& h)
{
	suspend(std::function<void()>(h));
}

void my_actor::suspend(std::function<void()>&& h)
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);

	_suspendResumeQueue.push_back(suspend_resume_option());
	suspend_resume_option& tmp = _suspendResumeQueue.back();
	tmp._isSuspend = true;
	tmp._h = std::move(h);
	if (_suspendResumeQueue.size() == 1)
	{
		suspend();
	}
}

void my_actor::suspend()
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);

	assert(!_childSuspendResumeCount);
	if (!_quited)
	{
		if (!_suspended)
		{
			_suspended = true;
			if (_timer)
			{
				suspend_timer();
			}
			if (!_childActorList.empty())
			{
				_childSuspendResumeCount = _childActorList.size();
				for (auto& childActor : _childActorList)
				{
					actor_handle shared_this = shared_from_this();
					if (childActor->_strand == _strand)
					{
						childActor->suspend([shared_this]{shared_this->child_suspend_cb_handler(); });
					}
					else
					{
						childActor->notify_suspend(_strand->wrap_post([shared_this]{shared_this->child_suspend_cb_handler(); }));
					}
				}
				return;
			}
		}
	}
	_childSuspendResumeCount = 1;
	child_suspend_cb_handler();
}

void my_actor::notify_resume()
{
	notify_resume(std::function<void()>());
}

void my_actor::notify_resume(const std::function<void()>& h)
{
	notify_resume(std::function<void()>(h));
}

void my_actor::notify_resume(std::function<void()>&& h)
{
	struct wrap_resume
	{
		wrap_resume(const actor_handle& host, std::function<void()>&& h)
		:_sharedThis(host), _h(std::move(h)) {}

		wrap_resume(wrap_resume&& s)
			:_sharedThis(s._sharedThis), _h(std::move(s._h)) {}

		void operator()()
		{
			_sharedThis->resume(std::move(_h));
		}

		actor_handle _sharedThis;
		std::function<void()> _h;
	};
	_strand->try_tick(wrap_resume(shared_from_this(), std::move(h)));
}

void my_actor::resume(const std::function<void()>& h)
{
	resume(std::function<void()>(h));
}

void my_actor::resume(std::function<void()>&& h)
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);

	_suspendResumeQueue.push_back(suspend_resume_option());
	suspend_resume_option& tmp = _suspendResumeQueue.back();
	tmp._isSuspend = false;
	tmp._h = std::move(h);
	if (_suspendResumeQueue.size() == 1)
	{
		resume();
	}
}

void my_actor::resume()
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);

	assert(!_childSuspendResumeCount);
	if (!_quited)
	{
		if (_suspended)
		{
			_suspended = false;
			if (_timer)
			{
				resume_timer();
			}
			if (!_childActorList.empty())
			{
				_childSuspendResumeCount = _childActorList.size();
				for (auto& childActor : _childActorList)
				{
					actor_handle shared_this = shared_from_this();
					if (childActor->_strand == _strand)
					{
						childActor->resume(_strand->wrap_post([shared_this]{shared_this->child_resume_cb_handler(); }));
					}
					else
					{
						childActor->notify_resume(_strand->wrap_post([shared_this]{shared_this->child_resume_cb_handler(); }));
					}
				}
				return;
			}
		}
	}
	_childSuspendResumeCount = 1;
	child_resume_cb_handler();
}

void my_actor::switch_pause_play()
{
	switch_pause_play(std::function<void(bool isPaused)>());
}

void my_actor::switch_pause_play(const std::function<void(bool)>& h)
{
	actor_handle shared_this = shared_from_this();
	_strand->try_tick([shared_this, h]
	{
		assert(shared_this->_strand->running_in_this_thread());
		if (!shared_this->_quited)
		{
			if (shared_this->_suspended)
			{
				if (h)
				{
					auto& h_ = h;
					shared_this->resume([h_]{h_(false); });
				}
				else
				{
					shared_this->resume(std::function<void()>());
				}
			}
			else
			{
				if (h)
				{
					auto& h_ = h;
					shared_this->suspend([h_]{h_(true); });
				}
				else
				{
					shared_this->suspend(std::function<void()>());
				}
			}
		}
		else if (h)
		{
			DEBUG_OPERATION(size_t yc = shared_this->yield_count());
			CHECK_EXCEPTION1(h, true);
			assert(shared_this->yield_count() == yc);
		}
	});
}

void my_actor::outside_wait_quit()
{
	assert(!_strand->in_this_ios());
	std::mutex mutex;
	std::condition_variable conVar;
	std::unique_lock<std::mutex> ul(mutex);
	LAMBDA_THIS_REF2(ref2, conVar, mutex);
	_strand->post([&ref2]
	{
		assert(ref2->_strand->running_in_this_thread());
		if (ref2->_exited)
		{
			std::lock_guard<std::mutex> lg(ref2.mutex);
			ref2.conVar.notify_one();
		}
		else
		{
			auto& ref2_ = ref2;
			ref2->_exitCallback.push_back([&ref2_]()
			{
				std::lock_guard<std::mutex> lg(ref2_.mutex);
				ref2_.conVar.notify_one();
			});
		}
	});
	conVar.wait(ul);
}

void my_actor::append_quit_callback(const std::function<void()>& h)
{
	append_quit_callback(std::function<void()>(h));
}

void my_actor::append_quit_callback(std::function<void()>&& h)
{
	if (_strand->running_in_this_thread())
	{
		if (_exited)
		{
			DEBUG_OPERATION(size_t yc = yield_count());
			CHECK_EXCEPTION(h);
			assert(yield_count() == yc);
		}
		else
		{
			_exitCallback.push_back(std::move(h));
		}
	}
	else
	{
		struct wrap_append 
		{
			wrap_append(const actor_handle& host, std::function<void()>&& h)
			:_sharedThis(host), _h(std::move(h)) {}

			wrap_append(wrap_append&& s)
				:_sharedThis(s._sharedThis), _h(std::move(s._h)) {}

			void operator()()
			{
				_sharedThis->append_quit_callback(std::move(_h));
			}

			actor_handle _sharedThis;
			std::function<void()> _h;
		};
		_strand->post(wrap_append(shared_from_this(), std::move(h)));
	}
}

void my_actor::actors_start_run(const list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->notify_run();
	}
}

void my_actor::actor_force_quit(const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](const trig_once_notifer<>& h){anotherActor->notify_quit(h); });
}

void my_actor::actors_force_quit(const list<actor_handle>& anotherActors)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->notify_quit(h);
	}
	for (size_t i = anotherActors.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
}

void my_actor::actor_wait_quit(const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](const trig_once_notifer<>& h){anotherActor->append_quit_callback(h); });
}

void my_actor::actors_wait_quit(const list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto& actorHandle : anotherActors)
	{
		actor_wait_quit(actorHandle);
	}
}

bool my_actor::timed_actor_wait_quit(int tm, const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	actor_trig_handle<> ath;
	anotherActor->append_quit_callback(make_trig_notifer_to_self(ath));
	return timed_wait_trig(tm, ath);
}

void my_actor::actor_suspend(const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](const trig_once_notifer<>& h){anotherActor->notify_suspend(h); });
}

void my_actor::actors_suspend(const list<actor_handle>& anotherActors)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->notify_suspend(h);
	}
	for (size_t i = anotherActors.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
}

void my_actor::actor_resume(const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](const trig_once_notifer<>& h){anotherActor->notify_resume(h); });
}

void my_actor::actors_resume(const list<actor_handle>& anotherActors)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->notify_resume(h);
	}
	for (size_t i = anotherActors.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
}

bool my_actor::actor_switch(const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	return trig<bool>([anotherActor](const trig_once_notifer<bool>& h){anotherActor->switch_pause_play(h); });
}

bool my_actor::actors_switch(const list<actor_handle>& anotherActors)
{
	assert_enter();
	bool isPause = true;
	actor_msg_handle<bool> amh;
	auto h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->switch_pause_play(h);
	}
	for (size_t i = anotherActors.size(); i > 0; i--)
	{
		isPause &= wait_msg(amh);
	}
	close_msg_notifer(amh);
	return isPause;
}

void my_actor::run_one()
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);
	if (!_quited)
	{
		pull_yield();
	}
}

void my_actor::pull_yield_tls()
{
#if (defined CHECK_SELF) && (defined ENABLE_TLS_CHECK_SELF)
	void** pval = io_engine::getTlsValuePtr(0);
	my_actor* old = (my_actor*)*pval;
	*pval = this;
	(*(actor_pull_type*)_actorPull)();
	*pval = old;
#else
	(*(actor_pull_type*)_actorPull)();
#endif
}

void my_actor::pull_yield()
{
	assert(!_exited);
	assert(!_inActor);
	if (!_suspended)
	{
		pull_yield_tls();
	}
	else
	{
		assert(!_hasNotify);
		_hasNotify = true;
	}
}

void my_actor::pull_yield_after_quited()
{
	pull_yield_tls();
}

void my_actor::push_yield()
{
	assert(!_exited);
	assert(_inActor);
	check_stack();
	_yieldCount++;
	_inActor = false;
	(*(actor_push_type*)_actorPush)();
	if (!_notifyQuited || _lockQuit)
	{
		_inActor = true;
		return;
	}
	throw force_quit_exception();
}

void my_actor::push_yield_after_quited()
{
	check_stack();
	_inActor = false;
	(*(actor_push_type*)_actorPush)();
	_inActor = true;
}

void my_actor::force_quit_cb_handler()
{
	assert(_notifyQuited);
	assert(_isForce);
	assert(!_inActor);
	assert((int)_childOverCount > 0);
	if (0 == --_childOverCount)
	{
		exit_callback();
	}
}

void my_actor::child_suspend_cb_handler()
{
	assert(_strand->running_in_this_thread());
	assert((int)_childSuspendResumeCount > 0);
	if (0 == --_childSuspendResumeCount)
	{
		assert(_suspendResumeQueue.front()._isSuspend);
		while (!_suspendResumeQueue.empty())
		{
			suspend_resume_option& tmp = _suspendResumeQueue.front();
			if (tmp._isSuspend)
			{
				if (tmp._h)
				{
					DEBUG_OPERATION(size_t yc = yield_count());
					CHECK_EXCEPTION(tmp._h);
					assert(yield_count() == yc);
				}
				_suspendResumeQueue.pop_front();
			}
			else
			{
				actor_handle shared_this = shared_from_this();
				_strand->next_tick([shared_this]{shared_this->resume(); });
				return;
			}
		}
	}
}

void my_actor::child_resume_cb_handler()
{
	assert(_strand->running_in_this_thread());
	assert((int)_childSuspendResumeCount > 0);
	if (0 == --_childSuspendResumeCount)
	{
		assert(!_suspendResumeQueue.front()._isSuspend);
		while (!_suspendResumeQueue.empty())
		{
			suspend_resume_option& tmp = _suspendResumeQueue.front();
			if (!tmp._isSuspend)
			{
				if (tmp._h)
				{
					DEBUG_OPERATION(size_t yc = yield_count());
					CHECK_EXCEPTION(tmp._h);
					assert(yield_count() == yc);
				}
				_suspendResumeQueue.pop_front();
			}
			else
			{
				actor_handle shared_this = shared_from_this();
				_strand->next_tick([shared_this]{shared_this->suspend(); });
				return;
			}
		}
		if (_hasNotify)
		{
			_hasNotify = false;
			run_one();
		}
	}
}

void my_actor::exit_callback()
{
	assert(_notifyQuited);
	assert(!_childOverCount);
	DEBUG_OPERATION(size_t yc = yield_count());
	while (!_quitHandlerList.empty())
	{
		CHECK_EXCEPTION(_quitHandlerList.front());
		_quitHandlerList.pop_front();
	}
	assert(yield_count() == yc);
	pull_yield_tls();
}

void my_actor::timeout_handler()
{
	assert(_timerState._timerCb);
	_timerState._timerCompleted = true;
	_timerState._timerHandle.reset();
	auto h = std::move(_timerState._timerCb);
	assert(!_timerState._timerCb);
	h();
}

void my_actor::cancel_timer()
{
	assert(_timer);
	if (!_timerState._timerCompleted)
	{
		_timerState._timerCompleted = true;
		_timerState._timerCount++;
		if (_timerState._timerTime)
		{
			_timer->cancel(_timerState._timerHandle);
			clear_function(_timerState._timerCb);
		}
	}
}

void my_actor::suspend_timer()
{
	if (!_timerState._timerSuspend)
	{
		_timerState._timerSuspend = true;
		if (!_timerState._timerCompleted && _timerState._timerTime)
		{
			_timer->cancel(_timerState._timerHandle);
			_timerState._timerStampEnd = get_tick_us();
			long long tt = _timerState._timerStampBegin + _timerState._timerTime;
			if (_timerState._timerStampEnd > tt)
			{
				_timerState._timerStampEnd = tt;
			}
		}
	}
}

void my_actor::resume_timer()
{
	if (_timerState._timerSuspend)
	{
		_timerState._timerSuspend = false;
		if (!_timerState._timerCompleted && _timerState._timerTime)
		{
			assert(_timerState._timerTime >= _timerState._timerStampEnd - _timerState._timerStampBegin);
			_timerState._timerTime -= _timerState._timerStampEnd - _timerState._timerStampBegin;
			_timerState._timerStampBegin = get_tick_us();
			_timerState._timerHandle = _timer->timeout(_timerState._timerTime, shared_from_this());
		}
	}
}

void my_actor::check_stack()
{
#ifdef CHECK_ACTOR_STACK
	if ((size_t)get_sp() < (size_t)_stackTop - _stackSize)
	{
		stack_overflow_format((int)((size_t)get_sp() - (size_t)_stackTop - _stackSize), _createStack);
	}
#endif
	check_self();
}

my_actor* my_actor::self_actor()
{
#ifdef CHECK_SELF
#ifdef ENABLE_TLS_CHECK_SELF
	return (my_actor*)io_engine::getTlsValue(0);
#else
	std::lock_guard<std::mutex> lg(s_stackLineMutex);
	auto eit = s_stackLine.insert(make_pair(get_sp(), (my_actor*)NULL));
	if (eit.second)
	{
		s_stackLine.erase(eit.first--);
		assert(eit.first != s_stackLine.end());
		return eit.first->second;
	}
#endif
#endif
	return NULL;
}

void my_actor::check_self()
{
#ifdef CHECK_SELF
	DEBUG_OPERATION(my_actor* self = self_actor());
	assert(self);
	assert(this == self);
#endif
}

my_actor::stack_info my_actor::self_stack()
{
	assert_enter();
	void* sp = get_sp();
	stack_info si = { _stackTop, sp, _stackSize, (size_t)_stackTop - (size_t)sp, (int)(_stackSize - ((size_t)_stackTop - (size_t)sp)) };
	return si;
}

void my_actor::begin_check_func_stack()
{
	assert_enter();
#ifdef ENABLE_CHECK_FUNC_STACK
	_checkStackDepth = _stackSize - ((size_t)_stackTop - (size_t)((size_t)get_sp() & (-1 & (0-sizeof(dirty_data_type)))));
	assert((int)_checkStackDepth > 0);
	const size_t dp = _checkStackDepth / sizeof(dirty_data_type);
	dirty_data_type* sb = (dirty_data_type*)((char*)_stackTop - _stackSize);
	for (int i = 0; i < (int)dp; i++)
	{
		sb[i] = DIRTY_DATA;
	}
#endif
}

size_t my_actor::end_check_func_stack()
{
	assert_enter();
#ifdef ENABLE_CHECK_FUNC_STACK
	dirty_data_type* sb = (dirty_data_type*)((char*)_stackTop - _stackSize);
	while (DIRTY_DATA == *(sb++) && _checkStackDepth)
	{
		_checkStackDepth -= sizeof(dirty_data_type);
	}
	return _checkStackDepth;
#else
	return 0;
#endif
}

size_t my_actor::stack_idle_space()
{
	assert_enter();
	size_t s = _stackSize - ((size_t)_stackTop - (size_t)get_sp());
	if ((int)s < 0)
	{
		return 0;
	}
	return s;
}

void my_actor::close_msg_notifer(actor_msg_handle_base& amh)
{
	assert_enter();
	amh.close();
}

bool my_actor::timed_wait_msg(int tm, actor_msg_handle<>& amh)
{
	assert_enter();
	assert(amh._hostActor && amh._hostActor->self_id() == self_id());
	if (!amh.read_msg())
	{
		OUT_OF_SCOPE(
		{
			amh.stop_waiting();
		});
#ifdef ENABLE_CHECK_LOST
		if (amh._losted && amh._checkLost)
		{
			amh.throw_lost_exception();
		}
#endif
		if (tm > 0)
		{
			bool timed = false;
			delay_trig(tm, [this, &timed]
			{
				timed = true;
				pull_yield();
			});
			push_yield();
			if (timed)
			{
				return false;
			}
			cancel_delay_trig();
		}
		else if (tm < 0)
		{
			push_yield();
		}
		else
		{
			return false;
		}
#ifdef ENABLE_CHECK_LOST
		if (amh._losted && amh._checkLost)
		{
			amh.throw_lost_exception();
		}
#endif
	}
	return true;
}

bool my_actor::try_wait_msg(actor_msg_handle<>& amh)
{
	return timed_wait_msg(0, amh);
}

bool my_actor::timed_pump_msg(int tm, const msg_pump_handle<>& pump)
{
	return timed_pump_msg(tm, false, pump);
}

bool my_actor::timed_pump_msg(int tm, bool checkDis, const msg_pump_handle<>& pump)
{
	assert_enter();
	assert(!pump.check_closed());
	assert(pump.get()->_hostActor && pump.get()->_hostActor->self_id() == self_id());
	if (!pump.get()->read_msg())
	{
		OUT_OF_SCOPE(
		{
			pump.get()->stop_waiting();
		});
		if (checkDis && pump.get()->isDisconnected())
		{
			throw pump_disconnected<>();
		}
#ifdef ENABLE_CHECK_LOST
		if (pump.get()->_losted && pump.get()->_checkLost)
		{
			throw msg_pump_handle<>::lost_exception(pump.get_id());
		}
#endif
		pump.get()->_checkDis = checkDis;
		if (tm >= 0)
		{
			bool timed = false;
			delay_trig(tm, [this, &timed]
			{
				timed = true;
				pull_yield();
			});
			push_yield();
			if (timed)
			{
				return false;
			}
			cancel_delay_trig();
		}
		else
		{
			push_yield();
		}
		if (pump.get()->_checkDis)
		{
			assert(checkDis);
			throw pump_disconnected<>();
		}
#ifdef ENABLE_CHECK_LOST
		if (pump.get()->_losted && pump.get()->_checkLost)
		{
			throw msg_pump_handle<>::lost_exception(pump.get_id());
		}
#endif
	}
	return true;
}

bool my_actor::try_pump_msg(const msg_pump_handle<>& pump)
{
	return try_pump_msg(false, pump);
}

bool my_actor::try_pump_msg(bool checkDis, const msg_pump_handle<>& pump)
{
	assert_enter();
	assert(!pump.check_closed());
	assert(pump.get()->_hostActor && pump.get()->_hostActor->self_id() == self_id());
	OUT_OF_SCOPE(
	{
		pump.get()->stop_waiting();
	});
	if (!pump.get()->try_read())
	{
		if (checkDis && pump.get()->isDisconnected())
		{
			throw pump_disconnected<>();
		}
#ifdef ENABLE_CHECK_LOST
		if (pump.get()->_losted && pump.get()->_checkLost)
		{
			throw msg_pump_handle<>::lost_exception(pump.get_id());
		}
#endif
		return false;
	}
	return true;
}

void my_actor::close_trig_notifer(actor_msg_handle_base& ath)
{
	assert_enter();
	ath.close();
}

bool my_actor::timed_wait_trig(int tm, actor_trig_handle<>& ath)
{
	assert_enter();
	assert(ath._hostActor && ath._hostActor->self_id() == self_id());
	if (!ath.read_msg())
	{
		OUT_OF_SCOPE(
		{
			ath.stop_waiting();
		});
#ifdef ENABLE_CHECK_LOST
		if (ath._losted && ath._checkLost)
		{
			ath.throw_lost_exception();
		}
#endif
		if (tm > 0)
		{
			bool timed = false;
			delay_trig(tm, [this, &timed]
			{
				timed = true;
				pull_yield();
			});
			push_yield();
			if (timed)
			{
				return false;
			}
			cancel_delay_trig();
		}
		else if (tm < 0)
		{
			push_yield();
		}
		else
		{
			return false;
		}
#ifdef ENABLE_CHECK_LOST
		if (ath._losted && ath._checkLost)
		{
			ath.throw_lost_exception();
		}
#endif
	}
	return true;
}

bool my_actor::try_wait_trig(actor_trig_handle<>& ath)
{
	return timed_wait_trig(0, ath);
}

void my_actor::wait_msg(actor_msg_handle<>& amh)
{
	timed_wait_msg(-1, amh);
}

void my_actor::wait_trig(actor_trig_handle<>& ath)
{
	timed_wait_trig(-1, ath);
}

void my_actor::pump_msg(bool checkDis, const msg_pump_handle<>& pump)
{
	timed_pump_msg(-1, checkDis, pump);
}

void my_actor::pump_msg(const msg_pump_handle<>& pump)
{
	return pump_msg(false, pump);
}
//////////////////////////////////////////////////////////////////////////

void ActorFunc_::lock_quit(my_actor* host)
{
	assert(host);
	host->lock_quit();
}

void ActorFunc_::unlock_quit(my_actor* host)
{
	assert(host);
	host->unlock_quit();
}

actor_handle ActorFunc_::shared_from_this(my_actor* host)
{
	assert(host);
	return host->shared_from_this();
}

const actor_handle& ActorFunc_::parent_actor(my_actor* host)
{
	assert(host);
	return host->parent_actor();
}

const shared_strand& ActorFunc_::self_strand(my_actor* host)
{
	assert(host);
	return host->self_strand();
}

void ActorFunc_::pull_yield(my_actor* host)
{
	assert(host);
	host->pull_yield();
}

void ActorFunc_::push_yield(my_actor* host)
{
	assert(host);
	host->push_yield();
}

bool ActorFunc_::is_quited(my_actor* host)
{
	assert(host);
	return host->is_quited();
}

std::shared_ptr<bool> ActorFunc_::new_bool(bool b)
{
	assert(s_sharedBoolPool);
	std::shared_ptr<bool> r = s_sharedBoolPool->pick();
	*r = b;
	return r;
}

#ifdef ENABLE_CHECK_LOST
std::shared_ptr<CheckLost_> ActorFunc_::new_check_lost(const shared_strand& strand, actor_msg_handle_base* msgHandle)
{
	auto& s_checkLostObjAlloc_ = s_checkLostObjAlloc;
	return std::shared_ptr<CheckLost_>(new(s_checkLostObjAlloc.allocate())CheckLost_(strand, msgHandle), [&s_checkLostObjAlloc_](CheckLost_* p)
	{
		p->~CheckLost_();
		s_checkLostObjAlloc_.deallocate(p);
	}, ref_count_alloc<void>(s_checkLostRefCountAlloc));
}

std::shared_ptr<CheckPumpLost_> ActorFunc_::new_check_pump_lost(const actor_handle& hostActor, MsgPoolBase_* pool)
{
	auto& s_checkPumpLostObjAlloc_ = s_checkPumpLostObjAlloc;
	return std::shared_ptr<CheckPumpLost_>(new(s_checkPumpLostObjAlloc.allocate())CheckPumpLost_(hostActor, pool), [&s_checkPumpLostObjAlloc_](CheckPumpLost_* p)
	{
		p->~CheckPumpLost_();
		s_checkPumpLostObjAlloc_.deallocate(p);
	}, ref_count_alloc<void>(s_checkPumpLostRefCountAlloc));
}
#endif