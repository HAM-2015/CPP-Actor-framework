#include "actor_framework.h"
#include "async_buffer.h"
#include "sync_msg.h"
#include "context_pool.h"
#include "bind_qt_run.h"
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#if (WIN32 && __GNUG__)
#include <fibersapi.h>
#endif
#ifdef __linux__
#include "sigsegv.h"
#include <sys/mman.h>
#endif

typedef ContextPool_::coro_push_interface actor_push_type;
typedef ContextPool_::coro_pull_interface actor_pull_type;

DEBUG_OPERATION(static std::thread::id s_installID);
static bool s_inited = false;
static bool s_isSelfInitFiber = false;
#ifdef __linux__
static std::mutex* s_sigsegvMutex = NULL;
#endif
#ifdef ENABLE_CHECK_LOST
static mem_alloc_mt<CheckLost_>* s_checkLostObjAlloc = NULL;
static mem_alloc_mt<CheckPumpLost_>* s_checkPumpLostObjAlloc = NULL;
static mem_alloc_base* s_checkLostRefCountAlloc = NULL;
static mem_alloc_base* s_checkPumpLostRefCountAlloc = NULL;
#endif

struct autoActorStackMng
{
	size_t get_stack_size(size_t key)
	{
		boost::shared_lock<boost::shared_mutex> sl(_mutex);
		auto it = _table.find(key);
		if (_table.end() != it)
		{
			return it->second;
		}
		return 0;
	}

	void update_stack_size(size_t key, size_t ns)
	{
		boost::unique_lock<boost::shared_mutex> ul(_mutex);
		_table[key] = ns;
	}

	std::map<size_t, size_t> _table;
	boost::shared_mutex _mutex;
};
static autoActorStackMng* s_autoActorStackMng = NULL;
shared_obj_pool<bool>* shared_bool::_sharedBoolPool = NULL;
std::mutex* TraceMutex_::_mutex = NULL;
std::atomic<my_actor::id>* my_actor::_actorIDCount = NULL;
msg_list_shared_alloc<actor_handle>::shared_node_alloc* my_actor::_childActorListAll = NULL;
msg_list_shared_alloc<std::function<void()> >::shared_node_alloc* my_actor::_quitExitCallbackAll = NULL;
msg_list_shared_alloc<my_actor::suspend_resume_option>::shared_node_alloc* my_actor::_suspendResumeQueueAll = NULL;
msg_map_shared_alloc<my_actor::msg_pool_status::id_key, std::shared_ptr<my_actor::msg_pool_status::pck_base> >::shared_node_alloc* my_actor::msg_pool_status::_msgTypeMapAll = NULL;

void my_actor::install()
{
	install(cpu_tick());
}

void my_actor::install(id aid)
{
	if (!s_inited)
	{
		s_inited = true;
		TraceMutex_::_mutex = new std::mutex;
		DEBUG_OPERATION(s_installID = std::this_thread::get_id());
		io_engine::install();
		install_check_stack();
		s_isSelfInitFiber = context_yield::convert_thread_to_fiber();
		ContextPool_::install();
#ifdef ENABLE_QT_UI
		bind_qt_run_base::install();
#endif
#ifdef ENABLE_CHECK_LOST
		auto& s_checkLostObjAlloc_ = s_checkLostObjAlloc;
		auto& s_checkPumpLostObjAlloc_ = s_checkPumpLostObjAlloc;
		s_checkLostObjAlloc_ = new mem_alloc_mt<CheckLost_>(100000);
		s_checkPumpLostObjAlloc_ = new mem_alloc_mt<CheckPumpLost_>(100000);
		s_checkLostRefCountAlloc = make_ref_count_alloc<CheckLost_, std::mutex>(100000, [s_checkLostObjAlloc_](CheckLost_*){});
		s_checkPumpLostRefCountAlloc = make_ref_count_alloc<CheckPumpLost_, std::mutex>(100000, [s_checkPumpLostObjAlloc_](CheckPumpLost_*){});
#endif
		s_autoActorStackMng = new autoActorStackMng;
#ifdef __linux__
		s_sigsegvMutex = new std::mutex;
#endif
		shared_bool::_sharedBoolPool = create_shared_pool_mt<bool, std::mutex>(100000);
		my_actor::_actorIDCount = new std::atomic<my_actor::id>(aid);
		my_actor::_childActorListAll = new msg_list_shared_alloc<actor_handle>::shared_node_alloc(100000);
		my_actor::_quitExitCallbackAll = new msg_list_shared_alloc<std::function<void()> >::shared_node_alloc(100000);
		my_actor::_suspendResumeQueueAll = new msg_list_shared_alloc<my_actor::suspend_resume_option>::shared_node_alloc(100000);
		my_actor::msg_pool_status::_msgTypeMapAll = new msg_map_shared_alloc<my_actor::msg_pool_status::id_key, std::shared_ptr<my_actor::msg_pool_status::pck_base> >::shared_node_alloc(100000);
	}
}

void my_actor::uninstall()
{
	if (s_inited)
	{
		s_inited = false;
		assert(std::this_thread::get_id() == s_installID);
		delete shared_bool::_sharedBoolPool;
		shared_bool::_sharedBoolPool = NULL;
		delete my_actor::_actorIDCount;
		my_actor::_actorIDCount = NULL;
		delete my_actor::_childActorListAll;
		my_actor::_childActorListAll = NULL;
		delete my_actor::_quitExitCallbackAll;
		my_actor::_quitExitCallbackAll = NULL;
		delete my_actor::_suspendResumeQueueAll;
		my_actor::_suspendResumeQueueAll = NULL;
		delete my_actor::msg_pool_status::_msgTypeMapAll;
		my_actor::msg_pool_status::_msgTypeMapAll = NULL;
		delete s_autoActorStackMng;
		s_autoActorStackMng = NULL;
#ifdef __linux__
		delete s_sigsegvMutex;
		s_sigsegvMutex = NULL;
#endif
#ifdef ENABLE_CHECK_LOST
		delete s_checkLostRefCountAlloc;
		s_checkLostRefCountAlloc = NULL;
		delete s_checkPumpLostRefCountAlloc;
		s_checkPumpLostRefCountAlloc = NULL;
		delete s_checkLostObjAlloc;
		s_checkLostObjAlloc = NULL;
		delete s_checkPumpLostObjAlloc;
		s_checkPumpLostObjAlloc = NULL;
#endif
#ifdef ENABLE_QT_UI
		bind_qt_run_base::uninstall();
#endif
		ContextPool_::uninstall();
		if (s_isSelfInitFiber)
			context_yield::convert_fiber_to_thread();
		uninstall_check_stack();
		io_engine::uninstall();
		delete TraceMutex_::_mutex;
		TraceMutex_::_mutex = NULL;
	}
}

#define ACTOR_TLS_INDEX 0
//////////////////////////////////////////////////////////////////////////

void shared_bool::reset()
{
	_ptr.reset();
}

bool shared_bool::empty() const
{
	return !_ptr;
}

void shared_bool::operator=(bool b)
{
	assert(!empty());
	*_ptr = b;
}

shared_bool::operator bool() const
{
	assert(!empty());
	return *_ptr;
}

void shared_bool::operator=(shared_bool&& s)
{
	_ptr = std::move(s._ptr);
}

void shared_bool::operator=(const shared_bool& s)
{
	_ptr = s._ptr;
}

shared_bool::shared_bool(std::shared_ptr<bool>&& pb)
:_ptr(std::move(pb)) {}

shared_bool::shared_bool(const std::shared_ptr<bool>& pb)
: _ptr(pb) {}

shared_bool::shared_bool()
{
}

shared_bool::shared_bool(const shared_bool& s)
:_ptr(s._ptr) {}

shared_bool::shared_bool(shared_bool&& s)
: _ptr(std::move(s._ptr)) {}

shared_bool shared_bool::new_(bool b)
{
	assert(_sharedBoolPool);
	shared_bool r(_sharedBoolPool->pick());
	r = b;
	return r;
}

//////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_CHECK_LOST
CheckLost_::CheckLost_(const shared_strand& strand, actor_msg_handle_base* msgHandle)
:_strand(strand), _handle(msgHandle), _closed(msgHandle->_closed) {}

CheckLost_::~CheckLost_()
{
	if (!_closed)
	{
		auto& handle_ = _handle;
		_strand->try_tick(std::bind([handle_](const shared_bool& closed)
		{
			if (!closed)
			{
				handle_->lost_msg();
			}
		}, std::move(_closed)));
	}
}

//////////////////////////////////////////////////////////////////////////
CheckPumpLost_::CheckPumpLost_(const actor_handle& hostActor, MsgPoolBase_* pool)
:_hostActor(hostActor), _pool(pool) {}

CheckPumpLost_::CheckPumpLost_(actor_handle&& hostActor, MsgPoolBase_* pool)
: _hostActor(std::move(hostActor)), _pool(pool) {}

CheckPumpLost_::~CheckPumpLost_()
{
	_pool->lost_msg(std::move(_hostActor));
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

child_actor_handle::child_actor_handle(actor_handle&& actor)
:_actor(std::move(actor)), _started(false), _quited(false)
{
	my_actor* parent = _actor->_parentActor.get();
	parent->_childActorList.push_front(_actor);
	_actorIt = parent->_childActorList.begin();
	_actor->_quitCallback.push_front(parent->make_trig_notifer_to_self(_quiteAth));
	_athIt = _actor->_quitCallback.begin();
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
		_actor->_quitCallback.erase(s._athIt);
		parent->close_trig_notifer(s._quiteAth);
		_actor->_quitCallback.push_front(parent->make_trig_notifer_to_self(_quiteAth));
		_athIt = _actor->_quitCallback.begin();
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

void actor_msg_handle_base::set_actor(my_actor* hostActor)
{
	_hostActor = hostActor;
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

const shared_bool& actor_msg_handle_base::dead_sign()
{
	return _closed;
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
		_strand->post(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPoolVoid_>& sharedThis)
		{
			sharedThis->send_msg(std::move((actor_handle&)hostActor));
		}, hostActor, _weakThis.lock()));
	}
}

void MsgPoolVoid_::_lost_msg(const actor_handle& hostActor)
{
	if (!_closed && _msgPump)
	{
		_msgPump->lost_msg(std::move((actor_handle&)hostActor));
	}
	else
	{
		_losted = true;
	}
}

void MsgPoolVoid_::lost_msg(const actor_handle& hostActor)
{
	if (!_closed)
	{
		_strand->try_tick(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPoolVoid_>& sharedThis)
		{
			sharedThis->_lost_msg(hostActor);
		}, hostActor, _weakThis.lock()));
	}
}

void MsgPoolVoid_::lost_msg(actor_handle&& hostActor)
{
	if (!_closed)
	{
		_strand->try_tick(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPoolVoid_>& sharedThis)
		{
			sharedThis->_lost_msg(hostActor);
		}, std::move(hostActor), _weakThis.lock()));
	}
}

void MsgPoolVoid_::send_msg(actor_handle&& hostActor)
{
	//if (_closed) return;

	_losted = false;
	if (_waiting)
	{
		_waiting = false;
		assert(_msgPump);
		assert(0 == _msgBuff);
		_sendCount++;
		_msgPump->receive_msg(std::move(hostActor));
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

void MsgPoolVoid_::pump_handler::pump_msg(unsigned char pumpID, actor_handle&& hostActor)
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
				_msgPump->receive_msg(std::move(hostActor));
			}
#ifdef ENABLE_CHECK_LOST
			else if (_thisPool->_losted)
			{
				_msgPump->lost_msg(std::move(hostActor));
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
	host->lock_quit();
	auto h = [&wait, pumpID](pump_handler& pump)->bool
	{
		bool ok = false;
		auto& thisPool_ = pump._thisPool;
		if (pump._msgPump == thisPool_->_msgPump)
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
	};
	bool r = false;
	if (host->self_strand() == _thisPool->_strand)
	{
		r = h(*this);
	}
	else
	{
		r = host->async_send<bool>(_thisPool->_strand, std::bind(h, *this));
	}
	host->unlock_quit();
	return r;
}

size_t MsgPoolVoid_::pump_handler::size(my_actor* host, unsigned char pumpID)
{
	assert(_thisPool);
	auto h = [pumpID](pump_handler& pump)->size_t
	{
		auto& thisPool_ = pump._thisPool;
		if (pump._msgPump == thisPool_->_msgPump)
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
	};
	size_t r = 0;
	if (host->self_strand() == _thisPool->_strand)
	{
		r = h(*this);
	}
	else
	{
		host->lock_quit();
		r = host->async_send<size_t>(_thisPool->_strand, std::bind(h, *this));
		host->unlock_quit();
	}
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
	_thisPool->_strand->post(std::bind([pumpID](pump_handler& pump)
	{
		if (pump._msgPump == pump._thisPool->_msgPump)
		{
			pump.pump_msg(pumpID, pump._msgPump->_hostActor->shared_from_this());
		}
	}, *this));
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

MsgPoolVoid_::pump_handler::pump_handler()
{}

MsgPoolVoid_::pump_handler::pump_handler(const pump_handler& s)
:_thisPool(s._thisPool), _msgPump(s._msgPump) {}

MsgPoolVoid_::pump_handler::pump_handler(pump_handler&& s)
: _thisPool(std::move(s._thisPool)), _msgPump(std::move(s._msgPump)) {}

void MsgPoolVoid_::pump_handler::operator=(const pump_handler& s)
{
	_thisPool = s._thisPool;
	_msgPump = s._msgPump;
}

void MsgPoolVoid_::pump_handler::operator=(pump_handler&& s)
{
	_thisPool = std::move(s._thisPool);
	_msgPump = std::move(s._msgPump);
}
//////////////////////////////////////////////////////////////////////////

MsgPumpVoid_::MsgPumpVoid_(my_actor* hostActor, bool checkLost)
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
	_hostActor = hostActor;
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

void MsgPumpVoid_::_lost_msg()
{
	_losted = true;
	if (_waiting && _checkLost)
	{
		_waiting = false;
		_checkDis = false;
		ActorFunc_::pull_yield(_hostActor);
	}
}

void MsgPumpVoid_::lost_msg(const actor_handle& hostActor)
{
	if (_strand->running_in_this_thread())
	{
		_lost_msg();
	}
	else
	{
		_strand->post(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPumpVoid_>& sharedThis)
		{
			sharedThis->_lost_msg();
		}, hostActor, _weakThis.lock()));
	}
}

void MsgPumpVoid_::lost_msg(actor_handle&& hostActor)
{
	if (_strand->running_in_this_thread())
	{
		_lost_msg();
	}
	else
	{
		_strand->post(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPumpVoid_>& sharedThis)
		{
			sharedThis->_lost_msg();
		}, std::move(hostActor), _weakThis.lock()));
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
		_strand->post(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPumpVoid_>& sharedThis)
		{
			sharedThis->receiver();
		}, hostActor, _weakThis.lock()));
	}
}

void MsgPumpVoid_::receive_msg(actor_handle&& hostActor)
{
	if (_strand->running_in_this_thread())
	{
		receiver();
	}
	else
	{
		_strand->post(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPumpVoid_>& sharedThis)
		{
			sharedThis->receiver();
		}, std::move(hostActor), _weakThis.lock()));
	}
}

void MsgPumpVoid_::receive_msg_tick(const actor_handle& hostActor)
{
	_strand->try_tick(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPumpVoid_>& sharedThis)
	{
		sharedThis->receiver();
	}, hostActor, _weakThis.lock()));
}

void MsgPumpVoid_::receive_msg_tick(actor_handle&& hostActor)
{
	_strand->try_tick(std::bind([](const actor_handle& hostActor, const std::shared_ptr<MsgPumpVoid_>& sharedThis)
	{
		sharedThis->receiver();
	}, std::move(hostActor), _weakThis.lock()));
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

void TrigOnceBase_::tick_handler(const shared_bool& closed, bool* sign) const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->tick_handler(closed, sign);
	reset();
}

void TrigOnceBase_::tick_handler(shared_bool&& closed, bool* sign) const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->tick_handler(std::move(closed), sign);
	reset();
}

void TrigOnceBase_::dispatch_handler(bool* sign) const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->dispatch_handler(sign);
	reset();
}

void TrigOnceBase_::dispatch_handler(const shared_bool& closed, bool* sign) const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->dispatch_handler(closed, sign);
	reset();
}

void TrigOnceBase_::dispatch_handler(shared_bool&& closed, bool* sign) const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->dispatch_handler(std::move(closed), sign);
	reset();
}

void TrigOnceBase_::copy(const TrigOnceBase_& s)
{
	_hostActor = s._hostActor;
	DEBUG_OPERATION(_pIsTrig = s._pIsTrig);
}

void TrigOnceBase_::move(TrigOnceBase_&& s)
{
	_hostActor = std::move(s._hostActor);
	DEBUG_OPERATION(_pIsTrig = std::move(s._pIsTrig));
}
//////////////////////////////////////////////////////////////////////////

bool mutex_block_quit::ready()
{
	_host->_waitingQuit = true;
	return !_quitNtfed && _host->quit_msg();
}

void mutex_block_quit::cancel()
{
	_host->_waitingQuit = false;
}

void mutex_block_quit::check_lost()
{
}

bool mutex_block_quit::go(bool& isRun)
{
	if (!_quitNtfed && _host->quit_msg())
	{
		_quitNtfed = true;
		isRun = true;
		return _quitHandler();
	}
	isRun = false;
	return false;
}

size_t mutex_block_quit::snap_id()
{
	return -1;
}

long long mutex_block_quit::host_id()
{
	return _host->self_id();
}

void mutex_block_quit::check_lock_quit()
{
	assert(_host->_lockQuit > 0);
}
//////////////////////////////////////////////////////////////////////////

bool mutex_block_sign::ready()
{
	_host->_waitingTrigMask |= _mask;
	return !_signNtfed && (_host->_trigSignMask & _mask);
}

void mutex_block_sign::cancel()
{
	_host->_waitingTrigMask &= (-1 ^ _mask);
}

void mutex_block_sign::check_lost()
{
}

bool mutex_block_sign::go(bool& isRun)
{
	if (!_signNtfed && (_host->_trigSignMask & _mask))
	{
		_signNtfed = true;
		isRun = true;
		return _signHandler();
	}
	isRun = false;
	return false;
}

size_t mutex_block_sign::snap_id()
{
	return (-1 ^ ((size_t)-1 >> 1)) | _mask;
}

long long mutex_block_sign::host_id()
{
	return _host->self_id();
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

my_actor::quit_guard::quit_guard(my_actor* self) :_self(self)
{
	_locked = true;
	_self->lock_quit();
}

my_actor::quit_guard::~quit_guard() __disable_noexcept
{
	if (_locked)
	{
		//可能在此析构函数内抛出 force_quit_exception 异常
		_self->unlock_quit();
	}
}

void my_actor::quit_guard::lock()
{
	assert(!_locked);
	_locked = true;
	_self->lock_quit();
}

void my_actor::quit_guard::unlock()
{
	assert(_locked);
	_locked = false;
	_self->unlock_quit();
}
//////////////////////////////////////////////////////////////////////////

my_actor::suspend_guard::suspend_guard(my_actor* self) :_self(self)
{
	_locked = true;
	_self->lock_suspend();
}

my_actor::suspend_guard::~suspend_guard() __disable_noexcept
{
	if (_locked && !_self->is_quited())
	{
		_self->unlock_suspend();
	}
}

void my_actor::suspend_guard::lock()
{
	assert(!_locked);
	_locked = true;
	_self->lock_suspend();
}

void my_actor::suspend_guard::unlock()
{
	assert(_locked);
	_locked = false;
	_self->unlock_suspend();
}
//////////////////////////////////////////////////////////////////////////

class my_actor::actor_run
{
public:
	actor_run(my_actor& actor)
		: _actor(actor) {}

	void actor_handler(actor_push_type& actorPush)
	{
		assert(!_actor._quited);
		assert(_actor._mainFunc);
		_actor._actorPush = &actorPush;
		try
		{
			{
				_actor._yieldCount++;
				_actor._inActor = false;
				((actor_push_type*)_actor._actorPush)->yield();
				if (_actor._notifyQuited)
				{
					throw force_quit_exception();
				}
				_actor._inActor = true;
			}
			assert(_actor._strand->running_in_this_thread());
			if (_actor._checkStack)
			{
				_actor.stack_decommit(false);
			}
			_actor._mainFunc(&_actor);
			assert(_actor._inActor);
			assert(!_actor._lockQuit);
			assert(!_actor._lockSuspend);
			assert(_actor._childActorList.empty());
			assert(_actor._atBeginQuitRegistExecutor.empty());
		}
		catch (my_actor::force_quit_exception&)
		{//捕获Actor被强制退出异常
			assert(!_actor._inActor);
			_actor._inActor = true;
		}
		catch (my_actor::pump_disconnected_exception&)
		{
			trace_line("\nerror: ", "pump_disconnected_exception");
			assert(false);
			exit(11);
		}
		catch (std::shared_ptr<std::string>& msg)
		{
			trace_line("\nerror: ", *msg);
			assert(false);
			exit(12);
		}
		catch (async_buffer_close_exception&)
		{
			trace_line("\nerror: ", "async_buffer_close_exception");
			assert(false);
			exit(13);
		}
		catch (sync_csp_exception&)
		{
			trace_line("\nerror: ", "sync_csp_exception");
			assert(false);
			exit(14);
		}
		catch (ntf_lost_exception&)
		{
			trace_line("\nerror: ", "ntf_lost_exception");
			assert(false);
			exit(15);
		}
		catch (boost::exception&)
		{
			trace_line("\nerror: ", "boost::exception");
			assert(false);
			exit(16);
		}
		catch (std::exception&)
		{
			trace_line("\nerror: ", "std::exception");
			assert(false);
			exit(17);
		}
		catch (...)
		{
			assert(false);
			exit(-1);
		}
		clear_function(_actor._mainFunc);
		if (_actor._timer)
		{
			_actor.cancel_timer();
		}
		_actor._quited = true;
		_actor._notifyQuited = true;
		_actor._msgPoolStatus.clear(&_actor);//yield now
		_actor._inActor = false;
	}

	void exit_notify()
	{
		DEBUG_OPERATION(size_t yc = _actor.yield_count());
		_actor._exited = true;
		_actor._lockSuspend = 0;
		if (_actor._holdedSuspendSign)
		{
			_actor._holdedSuspendSign = false;
			while (!_actor._suspendResumeQueue.empty())
			{
				if (_actor._suspendResumeQueue.front()._h)
				{
					CHECK_EXCEPTION(_actor._suspendResumeQueue.front()._h);
				}
				_actor._suspendResumeQueue.pop_front();
			}
		}
		while (!_actor._quitCallback.empty())
		{
			assert(_actor._quitCallback.front());
			CHECK_EXCEPTION(_actor._quitCallback.front());
			_actor._quitCallback.pop_front();
		}
		assert(_actor.yield_count() == yc);
	}

#ifdef WIN32
	static size_t clean_size(context_yield::context_info* const info)
	{
		char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
		size_t rangeA = 0;
		size_t rangeB = (info->stackSize + info->reserveSize) / PAGE_SIZE;
		do
		{
			MEMORY_BASIC_INFORMATION mbi;
			const size_t i = rangeA + (rangeB - rangeA) / 2;
			VirtualQuery(sb + i * PAGE_SIZE, &mbi, sizeof(mbi));
			if ((PAGE_READWRITE | PAGE_GUARD) == mbi.Protect)
			{
				return i * PAGE_SIZE + PAGE_SIZE;
			} 
			else if (MEM_RESERVE == mbi.State)
			{
				rangeA = i + 1;
			}
			else
			{
				rangeB = i;
			}
		} while (rangeA < rangeB);
		return 0;
	}

	void check_stack()
	{
		context_yield::context_info* const info = ((actor_pull_type*)_actor._actorPull)->_coroInfo;
		const size_t cleanSize = clean_size(info);
		_actor._usingStackSize = std::max(_actor._usingStackSize, info->stackSize + info->reserveSize - cleanSize);
		//记录本次实际消耗的栈空间
		s_autoActorStackMng->update_stack_size(_actor._actorKey, _actor._usingStackSize);
		if (cleanSize < info->reserveSize - PAGE_SIZE)
		{
			//释放物理内存，保留地址空间
			char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
			VirtualFree(sb + cleanSize - PAGE_SIZE, info->reserveSize - cleanSize, MEM_DECOMMIT);
			DWORD oldPro = 0;
			VirtualProtect(sb + info->reserveSize - PAGE_SIZE, PAGE_SIZE, PAGE_READWRITE | PAGE_GUARD, &oldPro);
		}
	}
#ifndef _MSC_VER
	void operator ()(actor_push_type& actorPush)
	{
		actor_handler(actorPush);
		if (_actor._checkStack)
		{
			_actor.async_send_after_quited(_actor.self_strand(), [this]
			{
				check_stack();
				exit_notify();
			});
		}
		else
		{
			exit_notify();
		}
	}
#else
	void operator ()(actor_push_type& actorPush)
	{
		__try
		{
			actor_handler(actorPush);			
			if (_actor._checkStack)
			{//从实际栈底查看有多少个PAGE的PAGE_GUARD标记消失和用了多少栈预留空间
				_actor.async_send_after_quited(_actor.self_strand(), [this]
				{
					check_stack();
					exit_notify();
				});
			}
			else
			{
				exit_notify();
			}
		}
		__except (seh_exception_handler(GetExceptionCode(), GetExceptionInformation()))
		{
			exit(100);
		}
	}

	DWORD seh_exception_handler(DWORD ecd, _EXCEPTION_POINTERS* eInfo)
	{
		context_yield::context_info* const info = ((actor_pull_type*)_actor._actorPull)->_coroInfo;
		char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
#ifdef _WIN64
		char* const sp = (char*)((size_t)eInfo->ContextRecord->Rsp & (0 - PAGE_SIZE));
#else
		char* const sp = (char*)((size_t)eInfo->ContextRecord->Esp & (0 - PAGE_SIZE));
#endif
		char* const violationAddr = (char*)((size_t)eInfo->ExceptionRecord->ExceptionInformation[1] & (0 - PAGE_SIZE));
		if (violationAddr >= sb && violationAddr < info->stackTop)
		{
			if (STATUS_STACK_OVERFLOW == ecd)
			{
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			else if (STATUS_ACCESS_VIOLATION == ecd)
			{
				if (violationAddr - PAGE_SIZE >= sb)
				{
					VirtualAlloc(violationAddr - PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD);
				}
				size_t l = 0;
				while (violationAddr + l < sp)
				{
					DWORD oldPro = 0;
					MEMORY_BASIC_INFORMATION mbi;
					VirtualQuery(violationAddr + l, &mbi, sizeof(mbi));
					if (MEM_RESERVE == mbi.State)
					{
						VirtualAlloc(violationAddr + l, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
					}
					else
					{
						VirtualProtect(violationAddr + l, PAGE_SIZE, PAGE_READWRITE, &oldPro);
						if ((PAGE_READWRITE | PAGE_GUARD) == oldPro)
						{
							break;
						}
					}
					l += PAGE_SIZE;
				}
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			else if (STATUS_GUARD_PAGE_VIOLATION == ecd)
			{
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}
#endif
#elif __linux__
#pragma GCC push_options
#pragma GCC optimize("O0")
	static size_t none_size(my_actor* self)
	{
		unsigned char mvec[256];
		context_yield::context_info* const info = ((actor_pull_type*)self->_actorPull)->_coroInfo;
		const size_t totalStackSize = info->stackSize + info->reserveSize;
		char* const sb = (char*)info->stackTop - totalStackSize;
		bool ok = 0 == mincore(sb, totalStackSize, mvec);
		assert(ok);
		size_t i = 0;
		size_t rangeA = 0;
		size_t rangeB = (info->stackSize + info->reserveSize) / PAGE_SIZE;
		do
		{
			self->_sigsegvSign = true;
			i = rangeA + (rangeB - rangeA) / 2;
			char* const pi = sb + i * PAGE_SIZE;
			char t = *pi;
			if (self->_sigsegvSign)
			{
				rangeB = i;
			}
			else
			{
				mprotect(pi, PAGE_SIZE, PROT_NONE);
				rangeA = i + 1;
			}
			if (0 == mvec[i])
			{
				madvise(pi, PAGE_SIZE, MADV_DONTNEED);
			}
		} while (rangeA < rangeB);
		const bool acc = self->_sigsegvSign;
		self->_sigsegvSign = false;
		return i * PAGE_SIZE + (acc ? 0 : PAGE_SIZE);
	}
#pragma GCC pop_options

	static size_t clean_size(context_yield::context_info* const info)
	{
		unsigned char mvec[256];
		const size_t totalStackSize = info->stackSize + info->reserveSize;
		char* const sb = (char*)info->stackTop - totalStackSize;
		bool ok = 0 == mincore(sb, totalStackSize, mvec);
		assert(ok);
		for (size_t i = 0; i < totalStackSize; i += PAGE_SIZE)
		{
			if (0 != mvec[i / PAGE_SIZE])
			{
				return i;
			}
		}
		return totalStackSize;
	}

	void check_stack()
	{
		context_yield::context_info* const info = ((actor_pull_type*)_actor._actorPull)->_coroInfo;
		const size_t cleanSize = clean_size(info);
		_actor._usingStackSize = std::max(_actor._usingStackSize, info->stackSize + info->reserveSize - cleanSize);
		//记录本次实际消耗的栈空间
		s_autoActorStackMng->update_stack_size(_actor._actorKey, _actor._usingStackSize);
		if (cleanSize < info->reserveSize)
		{
			//释放物理内存，保留地址空间
			char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
			madvise(sb + cleanSize, info->reserveSize - cleanSize, MADV_DONTNEED);
		}
	}

	void operator ()(actor_push_type& actorPush)
	{
		actor_handler(actorPush);
		if (_actor._checkStack)
		{
			_actor.async_send_after_quited(_actor.self_strand(), [this]
			{
				check_stack();
				exit_notify();
			});
		}
		else
		{
			exit_notify();
		}
	}

	static int sigsegv_handler(void* fault_address, int serious)
	{
		my_actor* self = my_actor::self_actor();
		if (self)
		{
			context_yield::context_info* const info = ((actor_pull_type*)self->_actorPull)->_coroInfo;
			char* const violationAddr = (char*)((size_t)fault_address & (0 - PAGE_SIZE));
			char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
			if (violationAddr >= sb && violationAddr < info->stackTop)
			{
				if (!self->_sigsegvSign)
				{
					if (violationAddr > sb)
					{
						size_t l = 0;
						char* const st = (char*)info->stackTop - self->_usingStackSize;
						while (violationAddr + l < st)
						{
							mprotect(violationAddr + l, PAGE_SIZE, PROT_READ | PROT_WRITE);
							l += PAGE_SIZE;
						}
						if (violationAddr < (char*)info->stackTop - self->_usingStackSize)
						{
							self->_usingStackSize = (char*)info->stackTop - violationAddr;
						}
						return -1;
					}
					else
					{
						//触碰栈哨兵
						exit(101);
						return 0;
					}
				} 
				else
				{
					self->_sigsegvSign = false;
					mprotect(violationAddr, PAGE_SIZE, PROT_READ | PROT_WRITE);
					return -1;
				}
			}
		}
		exit(100);
		return 0;
	}

	static void stackoverflow_handler(int emergency, stackoverflow_context_t scp)
	{

	}

	static void install_sigsegv(void* actorExtraStack, size_t size)
	{
		s_sigsegvMutex->lock();
		stackoverflow_install_handler(stackoverflow_handler, actorExtraStack, size);
		sigsegv_install_handler(sigsegv_handler);
		s_sigsegvMutex->unlock();
	}

	static void deinstall_sigsegv()
	{
		s_sigsegvMutex->lock();
		sigsegv_deinstall_handler();
		stackoverflow_deinstall_handler();
		s_sigsegvMutex->unlock();
	}
#endif

private:
	my_actor& _actor;
};

#ifdef __linux__
void my_actor::install_sigsegv(void* actorExtraStack, size_t size)
{
	actor_run::install_sigsegv(actorExtraStack, size);
}

void my_actor::deinstall_sigsegv()
{
	actor_run::deinstall_sigsegv();
}
#endif

//////////////////////////////////////////////////////////////////////////

my_actor::my_actor()
:_suspendResumeQueue(*_suspendResumeQueueAll),
_quitCallback(*_quitExitCallbackAll),
_atBeginQuitRegistExecutor(*_quitExitCallbackAll),
_childActorList(*_childActorListAll)
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
#ifdef PRINT_ACTOR_STACK
	_checkStackFree = false;
#endif
	_checkStack = false;
	_timerStateSuspend = false;
	_timerStateCompleted = true;
	_holdedSuspendSign = false;
	_waitingQuit = false;
#ifdef __linux__
	_sigsegvSign = false;
#endif
	_selfID = ++(*_actorIDCount);
	_actorKey = -1;
	_lockQuit = 0;
	_lockSuspend = 0;
	_yieldCount = 0;
	_lastYield = -1;
	_childOverCount = 0;
	_childSuspendResumeCount = 0;
	_returnCode = 0;
	_usingStackSize = 0;
	_trigSignMask = 0;
	_waitingTrigMask = 0;
	_timerStateCount = 0;
	_timerStateTime = 0;
	_timerStateStampBegin = 0;
	_timerStateStampEnd = 0;
	_timerStateCb = NULL;
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
	assert(!_lockSuspend);
	assert(!_childSuspendResumeCount);
	assert(_suspendResumeQueue.empty());
	assert(_atBeginQuitRegistExecutor.empty());
	assert(_quitCallback.empty());
	assert(_childActorList.empty());

#ifdef PRINT_ACTOR_STACK
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	if (_checkStackFree || _checkStack || _usingStackSize > info->stackSize)
	{
		stack_overflow_format(_usingStackSize - info->stackSize, _createStack);
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
	actor_pull_type* pull = ContextPool_::getContext(stackSize);
	actor_handle newActor(new(pull->_space)my_actor(), [](my_actor* p){p->~my_actor(); }, actor_ref_count_alloc<void>(pull));
	newActor->_weakThis = newActor;
	newActor->_strand = actorStrand;
	newActor->_mainFunc = std::move(mainFunc);
	newActor->_timer = actorStrand->actor_timer();
	newActor->_actorPull = pull;
#ifdef PRINT_ACTOR_STACK
	newActor->_createStack = std::shared_ptr<std::list<stack_line_info>>(new std::list<stack_line_info>(get_stack_list(8, 1)));
#endif

	pull->_param = newActor.get();
	pull->_currentHandler = [](actor_push_type& push, void* p) {actor_run(*(my_actor*)p)(push); };
	pull->yield();
	return newActor;
}

actor_handle my_actor::create(const shared_strand& actorStrand, AutoStackActorFace_&& wrapActor)
{
	actor_pull_type* pull = NULL;
	const size_t nsize = wrapActor.stack_size();
	bool checkStack = false;
	if (nsize)
	{
		if (IS_TRY_SIZE(nsize))
		{
			size_t lasts = s_autoActorStackMng->get_stack_size(wrapActor.key());
			checkStack = !lasts;
			pull = ContextPool_::getContext(lasts ? lasts : GET_TRY_SIZE(nsize));
		}
		else
		{
			pull = ContextPool_::getContext(nsize);
			checkStack = false;
		}
	}
	else
	{
		size_t lasts = s_autoActorStackMng->get_stack_size(wrapActor.key());
		checkStack = !lasts;
		pull = ContextPool_::getContext(lasts ? lasts : DEFAULT_STACKSIZE);
	}
	if (!pull)
	{
		error_trace_line("stack memory deficiented");
		exit(-2);
		return actor_handle();
	}
	actor_handle newActor(new(pull->_space)my_actor(), [](my_actor* p){p->~my_actor(); }, actor_ref_count_alloc<void>(pull));
	newActor->_weakThis = newActor;
	newActor->_strand = actorStrand;
	newActor->_checkStack = checkStack;
	wrapActor.swap(newActor->_mainFunc);
	newActor->_actorKey = wrapActor.key();
	newActor->_timer = actorStrand->actor_timer();
	newActor->_actorPull = pull;
#ifdef PRINT_ACTOR_STACK
	newActor->_createStack = std::shared_ptr<std::list<stack_line_info>>(new std::list<stack_line_info>(get_stack_list(8, 1)));
#endif

	pull->_param = newActor.get();
	pull->_currentHandler = [](actor_push_type& push, void* p) {actor_run(*(my_actor*)p)(push); };
	pull->yield();
	return newActor;
}

child_actor_handle my_actor::create_child_actor(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	assert_enter();
	actor_handle childActor = my_actor::create(actorStrand, mainFunc, stackSize);
	childActor->_parentActor = shared_from_this();
	return child_actor_handle(std::move(childActor));
}

child_actor_handle my_actor::create_child_actor(const shared_strand& actorStrand, main_func&& mainFunc, size_t stackSize)
{
	assert_enter();
	actor_handle childActor = my_actor::create(actorStrand, std::move(mainFunc), stackSize);
	childActor->_parentActor = shared_from_this();
	return child_actor_handle(std::move(childActor));
}

child_actor_handle my_actor::create_child_actor(const main_func& mainFunc, size_t stackSize)
{
	return create_child_actor(_strand, mainFunc, stackSize);
}

child_actor_handle my_actor::create_child_actor(main_func&& mainFunc, size_t stackSize)
{
	return create_child_actor(_strand, std::move(mainFunc), stackSize);
}

child_actor_handle my_actor::create_child_actor(const shared_strand& actorStrand, AutoStackActorFace_&& wrapActor)
{
	assert_enter();
	actor_handle childActor = my_actor::create(actorStrand, std::move(wrapActor));
	childActor->_parentActor = shared_from_this();
	return child_actor_handle(std::move(childActor));
}

child_actor_handle my_actor::create_child_actor(AutoStackActorFace_&& wrapActor)
{
	return create_child_actor(_strand, std::move(wrapActor));
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

void my_actor::child_actors_run(std::list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		child_actor_run(*actorHandle);
	}
}

void my_actor::child_actors_run(std::list<child_actor_handle>& actorHandles)
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

void my_actor::child_actors_force_quit(std::list<child_actor_handle::ptr>& actorHandles)
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

void my_actor::child_actors_force_quit(std::list<child_actor_handle>& actorHandles)
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

void my_actor::child_actors_wait_quit(std::list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle->get_actor()->parent_actor()->self_id() == self_id());
		child_actor_wait_quit(*actorHandle);
	}
}

void my_actor::child_actors_wait_quit(std::list<child_actor_handle>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
		child_actor_wait_quit(actorHandle);
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

void my_actor::child_actor_suspend(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
	my_actor* actor = actorHandle.get_actor().get();
	if (actorHandle.get_actor()->self_strand() == _strand)
	{
		trig([actor](trig_once_notifer<>&& h){actor->suspend(std::move(h)); });
	}
	else
	{
		trig([actor](trig_once_notifer<>&& h){actor->notify_suspend(std::move(h)); });
	}
}

void my_actor::child_actors_suspend(std::list<child_actor_handle::ptr>& actorHandles)
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

void my_actor::child_actors_suspend(std::list<child_actor_handle>& actorHandles)
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
		trig([actor](trig_once_notifer<>&& h){actor->resume(std::move(h)); });
	}
	else
	{
		trig([actor](trig_once_notifer<>&& h){actor->notify_resume(std::move(h)); });
	}
}

void my_actor::child_actors_resume(std::list<child_actor_handle::ptr>& actorHandles)
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

void my_actor::child_actors_resume(std::list<child_actor_handle>& actorHandles)
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
			_strand->post(std::bind([](const actor_handle& shared_this)
			{
				shared_this->run_one();
			}, shared_from_this()));
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
	_strand->post(std::bind([](const actor_handle& shared_this)
	{
		shared_this->run_one();
	}, shared_from_this()));
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
	_strand->next_tick(std::bind([](const actor_handle& shared_this)
	{
		shared_this->run_one();
	}, shared_from_this()));
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

my_actor::quit_iterator my_actor::regist_quit_executor(const std::function<void()>& quitHandler)
{
	return regist_quit_executor(std::function<void()>(quitHandler));
}

my_actor::quit_iterator my_actor::regist_quit_executor(std::function<void()>&& quitHandler)
{
	assert_enter();
	_atBeginQuitRegistExecutor.push_front(std::move(quitHandler));//后注册的先执行
	return _atBeginQuitRegistExecutor.begin();
}

void my_actor::cancel_quit_executor(const quit_iterator& qh)
{
	assert_enter();
	_atBeginQuitRegistExecutor.erase(qh);
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

void my_actor::dispatch_handler(const shared_bool& closed, bool* sign)
{
	_strand->dispatch(wrap_check_trig_run_one(closed, shared_from_this(), sign));
}

void my_actor::dispatch_handler(shared_bool&& closed, bool* sign)
{
	_strand->dispatch(wrap_check_trig_run_one(std::move(closed), shared_from_this(), sign));
}

void my_actor::tick_handler(bool* sign)
{
	_strand->try_tick(wrap_trig_run_one(shared_from_this(), sign));
}

void my_actor::tick_handler(const shared_bool& closed, bool* sign)
{
	_strand->try_tick(wrap_check_trig_run_one(closed, shared_from_this(), sign));
}

void my_actor::tick_handler(shared_bool&& closed, bool* sign)
{
	_strand->try_tick(wrap_check_trig_run_one(TRY_MOVE(closed), shared_from_this(), sign));
}

void my_actor::next_tick_handler(bool* sign)
{
	_strand->next_tick(wrap_trig_run_one(shared_from_this(), sign));
}

void my_actor::next_tick_handler(const shared_bool& closed, bool* sign)
{
	_strand->next_tick(wrap_check_trig_run_one(closed, shared_from_this(), sign));
}

void my_actor::next_tick_handler(shared_bool&& closed, bool* sign)
{
	_strand->next_tick(wrap_check_trig_run_one(std::move(closed), shared_from_this(), sign));
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

size_t my_actor::self_key()
{
	return _actorKey;
}

reusable_mem& my_actor::self_reusable()
{
	return _reuMem;
}

void my_actor::return_code(size_t cd)
{
	_returnCode = cd;
}

size_t my_actor::return_code()
{
	assert(_exited);
	return _returnCode;
}

size_t my_actor::using_stack_size()
{
	assert(_exited);
	return _usingStackSize;
}

size_t my_actor::stack_size()
{
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	return info->stackSize;
}

size_t my_actor::stack_total_size()
{
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	return info->stackSize + info->reserveSize;
}

void my_actor::enable_check_stack(bool decommit)
{
	if (!_checkStack)
	{
		_checkStack = true;
#ifdef WIN32
		if (decommit)
		{
			stack_decommit(false);
		}
		else
		{
			char* const sp = (char*)((size_t)get_sp() & (0 - PAGE_SIZE));
			begin_RUN_IN_THREAD_STACK(this);
			context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
			char* const sm = (char*)info->stackTop - info->stackSize;
			//分页加PAGE_GUARD标记
			size_t l = PAGE_SIZE;
			while (sp - l >= sm)
			{
				DWORD oldPro = 0;
				BOOL ok = VirtualProtect(sp - l, PAGE_SIZE, PAGE_READWRITE | PAGE_GUARD, &oldPro);
				if (!ok || (PAGE_READWRITE | PAGE_GUARD) == oldPro)
				{
					break;
				}
				l += PAGE_SIZE;
			}
			end_RUN_IN_THREAD_STACK();
		}
#elif __linux__
		if (decommit)
		{
			stack_decommit(false);
		}
		else
		{
			char* const sp = (char*)((size_t)get_sp() & (0 - PAGE_SIZE));
			begin_RUN_IN_THREAD_STACK(this);
			context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
			char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
			bool ok = 0 == mprotect(sb, sp - sb, PROT_NONE);
			assert(ok);
			end_RUN_IN_THREAD_STACK();
		}
#endif
	}
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
	_strand->try_tick(std::bind([](const actor_handle& shared_this)
	{
		my_actor* self = shared_this.get();
		if (!self->_quited && !self->_started)
		{
			self->_started = true;
			self->pull_yield();
		}
	}, shared_from_this()));
}

void my_actor::assert_enter()
{
#if (_DEBUG || DEBUG)
	assert(_strand->running_in_this_thread());
	assert(!_quited);
	assert(_inActor);
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	assert((size_t)get_sp() >= (size_t)info->stackTop - info->stackSize - info->reserveSize + 1024);
	check_self();
#endif
}

void my_actor::notify_quit()
{
	if (!_quited)
	{
		_strand->try_tick(std::bind([](const actor_handle& shared_this)
		{
			shared_this->force_quit(std::function<void()>());
		}, shared_from_this()));
	}
}

void my_actor::notify_quit(const std::function<void()>& h)
{
	notify_quit(std::function<void()>(h));
}

void my_actor::notify_quit(std::function<void()>&& h)
{
	_strand->try_tick(std::bind([](const actor_handle& shared_this, std::function<void()>& h)
	{
		shared_this->force_quit(std::move(h));
	}, shared_from_this(), std::move(h)));
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
			if (h) _quitCallback.push_back(std::move(h));
			if (!_childActorList.empty())
			{
				_childOverCount = _childActorList.size();
				while (!_childActorList.empty())
				{
					auto cc = _childActorList.front();
					_childActorList.pop_front();
					if (cc->_strand == _strand)
					{
						cc->force_quit(std::bind([](const actor_handle& shared_this)
						{
							shared_this->force_quit_cb_handler();
						}, shared_from_this()));
					}
					else
					{
						cc->notify_quit(_strand->wrap_post(std::bind([](const actor_handle& shared_this)
						{
							shared_this->force_quit_cb_handler();
						}, shared_from_this())));
					}
				}
			}
			else
			{
				at_begin_quit_execute();
			}
			return;
		}
		else if (h)
		{
			_quitCallback.push_back(std::move(h));
		}
		if (_waitingQuit)
		{
			_waitingQuit = false;
			pull_yield();
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
			_quitCallback.push_back(std::move(h));
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

void my_actor::lock_suspend()
{
	assert_enter();
	_lockSuspend++;
}

void my_actor::unlock_suspend()
{
	assert_enter();
	assert(_lockSuspend);
	if (0 == --_lockSuspend && _holdedSuspendSign)
	{
		_holdedSuspendSign = false;
		assert(!_suspendResumeQueue.empty());
		if (_suspendResumeQueue.front()._isSuspend)
		{
			_strand->next_tick(std::bind([](const actor_handle& shared_this)
			{
				shared_this->suspend();
			}, shared_from_this()));
		} 
		else
		{
			_strand->next_tick(std::bind([](const actor_handle& shared_this)
			{
				shared_this->resume();
			}, shared_from_this()));
		}
		yield_guard();
	}
}

bool my_actor::is_locked_quit()
{
	assert_enter();
	return 0 != _lockQuit;
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
	_strand->try_tick(std::bind([](const actor_handle& shared_this, std::function<void()>& h)
	{
		shared_this->suspend(std::move(h));
	}, shared_from_this(), std::move(h)));
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
	if (1 == _suspendResumeQueue.size())
	{
		suspend();
	}
}

void my_actor::suspend()
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);
	assert(!_childSuspendResumeCount);
	assert(!_holdedSuspendSign);
	if (_lockSuspend)
	{
		_holdedSuspendSign = true;
		return;
	}
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
					if (childActor->_strand == _strand)
					{
						childActor->suspend(std::bind([](const actor_handle& shared_this)
						{
							shared_this->child_suspend_cb_handler();
						}, shared_from_this()));
					}
					else
					{
						childActor->notify_suspend(_strand->wrap_post(std::bind([](const actor_handle& shared_this)
						{
							shared_this->child_suspend_cb_handler();
						}, shared_from_this())));
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
	_strand->try_tick(std::bind([](const actor_handle& shared_this, std::function<void()>& h)
	{
		shared_this->resume(std::move(h));
	}, shared_from_this(), std::move(h)));
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
	if (1 == _suspendResumeQueue.size())
	{
		resume();
	}
}

void my_actor::resume()
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);
	assert(!_childSuspendResumeCount);
	assert(!_holdedSuspendSign);
	if (_lockSuspend)
	{
		_holdedSuspendSign = true;
		return;
	}
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
					if (childActor->_strand == _strand)
					{
						childActor->resume(_strand->wrap_post(std::bind([](const actor_handle& shared_this)
						{
							shared_this->child_resume_cb_handler();
						}, shared_from_this())));
					}
					else
					{
						childActor->notify_resume(_strand->wrap_post(std::bind([](const actor_handle& shared_this)
						{
							shared_this->child_resume_cb_handler();
						}, shared_from_this())));
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
	_strand->try_tick(std::bind([](const actor_handle& shared_this, const std::function<void(bool)>& h)
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
	}, shared_from_this(), h));
}

void my_actor::notify_trig_sign(int id)
{
	assert(id >= 0 && id < 8 * sizeof(void*));
	_strand->try_tick(std::bind([id](const actor_handle& shared_this)
	{
		my_actor* self = shared_this.get();
		if (!self->_quited)
		{
			const size_t mask = (size_t)1 << id;
			self->_trigSignMask |= mask;
			if (mask & self->_waitingTrigMask)
			{
				self->_waitingTrigMask ^= mask;
				self->pull_yield();
			}
		}
	}, shared_from_this()));
}

void my_actor::reset_trig_sign(int id)
{
	assert_enter();
	assert(id >= 0 && id < 8 * sizeof(void*));
	const size_t mask = (size_t)1 << id;
	_trigSignMask &= (-1 ^ mask);
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
			ref2->_quitCallback.push_back([&ref2_]()
			{
				std::lock_guard<std::mutex> lg(ref2_.mutex);
				ref2_.conVar.notify_one();
			});
		}
	});
	conVar.wait(ul);
}

void my_actor::append_quit_notify(const std::function<void()>& h)
{
	append_quit_notify(std::function<void()>(h));
}

void my_actor::append_quit_notify(std::function<void()>&& h)
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
			_quitCallback.push_back(std::move(h));
		}
	}
	else
	{
		_strand->post(std::bind([](const actor_handle& shared_this, std::function<void()>& h)
		{
			shared_this->append_quit_notify(std::move(h));
		}, shared_from_this(), std::move(h)));
	}
}

void my_actor::append_quit_executor(const std::function<void()>& h)
{
	append_quit_executor(std::function<void()>(h));
}

void my_actor::append_quit_executor(std::function<void()>&& h)
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
			_quitCallback.push_front(std::move(h));
		}
	}
	else
	{
		_strand->post(std::bind([](const actor_handle& shared_this, std::function<void()>& h)
		{
			shared_this->append_quit_executor(std::move(h));
		}, shared_from_this(), std::move(h)));
	}
}

void my_actor::actors_start_run(const std::list<actor_handle>& anotherActors)
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
	trig([anotherActor](trig_once_notifer<>&& h){anotherActor->notify_quit(std::move(h)); });
}

void my_actor::actors_force_quit(const std::list<actor_handle>& anotherActors)
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
	trig([anotherActor](trig_once_notifer<>&& h){anotherActor->append_quit_notify(std::move(h)); });
}

bool my_actor::timed_actor_wait_quit(int tm, const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	actor_trig_handle<> ath;
	anotherActor->append_quit_notify(make_trig_notifer_to_self(ath));
	return timed_wait_trig(tm, ath);
}

void my_actor::actors_wait_quit(const std::list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto& actorHandle : anotherActors)
	{
		actor_wait_quit(actorHandle);
	}
}

void my_actor::actor_suspend(const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](trig_once_notifer<>&& h){anotherActor->notify_suspend(std::move(h)); });
}

void my_actor::actors_suspend(const std::list<actor_handle>& anotherActors)
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
	trig([anotherActor](trig_once_notifer<>&& h){anotherActor->notify_resume(std::move(h)); });
}

void my_actor::actors_resume(const std::list<actor_handle>& anotherActors)
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
	return trig<bool>([anotherActor](trig_once_notifer<bool>&& h){anotherActor->switch_pause_play(std::move(h)); });
}

bool my_actor::actors_switch(const std::list<actor_handle>& anotherActors)
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
#if (WIN32 && ((_WIN32_WINNT >= 0x0502) || !(defined CHECK_SELF)))
	((actor_pull_type*)_actorPull)->yield();
#else//if (__linux__ || (WIN32 && (_WIN32_WINNT < 0x0502) && (defined CHECK_SELF)))
	void** tlsBuff = io_engine::getTlsValueBuff();
	void* old = tlsBuff[ACTOR_TLS_INDEX];
	tlsBuff[ACTOR_TLS_INDEX] = this;
	((actor_pull_type*)_actorPull)->yield();
	tlsBuff[ACTOR_TLS_INDEX] = old;
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
	((actor_push_type*)_actorPush)->yield();
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
	((actor_push_type*)_actorPush)->yield();
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
		at_begin_quit_execute();
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
				_strand->next_tick(std::bind([](const actor_handle& shared_this)
				{
					shared_this->resume();
				}, shared_from_this()));
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
				_strand->next_tick(std::bind([](const actor_handle& shared_this)
				{
					shared_this->suspend();
				}, shared_from_this()));
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

void my_actor::at_begin_quit_execute()
{
	assert(_notifyQuited);
	assert(!_childOverCount);
	DEBUG_OPERATION(size_t yc = yield_count());
	while (!_atBeginQuitRegistExecutor.empty())
	{
		CHECK_EXCEPTION(_atBeginQuitRegistExecutor.front());
		_atBeginQuitRegistExecutor.pop_front();
	}
	assert(yield_count() == yc);
	pull_yield_tls();
}

void my_actor::timeout_handler()
{
	assert(_timerStateCb);
	_timerStateCompleted = true;
	_timerStateHandle.reset();
	wrap_timer_handler_face* h = _timerStateCb;
	_timerStateCb = NULL;
	h->invoke();
	_reuMem.deallocate(h);
}

void my_actor::cancel_timer()
{
	assert(_timer);
	if (!_timerStateCompleted)
	{
		_timerStateCompleted = true;
		_timerStateCount++;
		if (_timerStateTime)
		{
			_timer->cancel(_timerStateHandle);
			_timerStateCb->destroy();
			_reuMem.deallocate(_timerStateCb);
			_timerStateCb = NULL;
		}
	}
}

void my_actor::suspend_timer()
{
	if (!_timerStateSuspend)
	{
		_timerStateSuspend = true;
		if (!_timerStateCompleted && _timerStateTime)
		{
			_timer->cancel(_timerStateHandle);
			_timerStateStampEnd = get_tick_us();
			long long tt = _timerStateStampBegin + _timerStateTime;
			if (_timerStateStampEnd > tt)
			{
				_timerStateStampEnd = tt;
			}
		}
	}
}

void my_actor::resume_timer()
{
	if (_timerStateSuspend)
	{
		_timerStateSuspend = false;
		if (!_timerStateCompleted && _timerStateTime)
		{
			assert(_timerStateTime >= _timerStateStampEnd - _timerStateStampBegin);
			_timerStateTime -= _timerStateStampEnd - _timerStateStampBegin;
			_timerStateStampBegin = get_tick_us();
			_timerStateHandle = _timer->timeout(_timerStateTime, shared_from_this());
		}
	}
}

void my_actor::check_stack()
{
#ifdef PRINT_ACTOR_STACK
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	if ((size_t)get_sp() < (size_t)info->stackTop - info->stackSize)
	{
		stack_overflow_format((int)((size_t)get_sp() - (size_t)info->stackTop - info->stackSize), _createStack);
	}
#endif
	check_self();
}

my_actor* my_actor::self_actor()
{
#if (WIN32 && (defined CHECK_SELF) && (_WIN32_WINNT >= 0x0502))
	return (my_actor*)FlsGetValue(ContextPool_::coro_pull_interface::_actorFlsIndex);
#elif (__linux__ || (WIN32 && (defined CHECK_SELF)))
	void** buff = io_engine::getTlsValueBuff();
	if (buff)
	{
		return (my_actor*)buff[ACTOR_TLS_INDEX];
	}
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
	void* const sp = get_sp();
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	return { info->stackTop,
		sp, info->stackSize,
		(size_t)info->stackTop - (size_t)sp,
		_usingStackSize,
		info->reserveSize,
		info->stackSize + info->reserveSize - ((size_t)info->stackTop - (size_t)sp) };
}

size_t my_actor::stack_idle_space()
{
	assert_enter();
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	return info->stackSize + info->reserveSize - ((size_t)info->stackTop - (size_t)get_sp());
}

size_t my_actor::clean_stack_size()
{
	size_t s = 0;
	begin_RUN_IN_THREAD_STACK(this);
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	s = actor_run::clean_size(info);
	end_RUN_IN_THREAD_STACK();
	return s;
}

size_t my_actor::none_stack_size()
{
#ifdef WIN32
	return clean_stack_size();
#elif __linux__
	size_t s = 0;
	begin_RUN_IN_THREAD_STACK(this);
	void** tlsBuff = io_engine::getTlsValueBuff();
	void* old = tlsBuff[ACTOR_TLS_INDEX];
	tlsBuff[ACTOR_TLS_INDEX] = this;
	s = actor_run::none_size(this);
	tlsBuff[ACTOR_TLS_INDEX] = old;
	end_RUN_IN_THREAD_STACK();
	return s;
#endif
}

void my_actor::stack_decommit(bool calcUsingStack)
{
	char* const sp = (char*)((size_t)get_sp() & (0 - PAGE_SIZE));
	begin_RUN_IN_THREAD_STACK(this);
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
#ifdef WIN32
	if (sp > sb)
	{
		if (calcUsingStack)
		{
			_usingStackSize = std::max(_usingStackSize, info->stackSize + info->reserveSize - actor_run::clean_size(info));
		}
		DWORD oldPro = 0;
		VirtualProtect(sp - PAGE_SIZE, PAGE_SIZE, PAGE_READWRITE | PAGE_GUARD, &oldPro);
		size_t l = 2 * PAGE_SIZE;
		while (sp - l >= sb)
		{
			MEMORY_BASIC_INFORMATION mbi;
			VirtualQuery(sp - l, &mbi, sizeof(mbi));
			if (MEM_RESERVE != mbi.State)
			{
				VirtualFree(sp - l, PAGE_SIZE, MEM_DECOMMIT);
				l += PAGE_SIZE;
			}
			else
			{
				break;
			}
		}
	}
#elif __linux__
	if (sp > sb)
	{
		if (calcUsingStack)
		{
			_usingStackSize = std::max(_usingStackSize, info->stackSize + info->reserveSize - actor_run::clean_size(info));
		}
		madvise(sb, sp - sb, MADV_DONTNEED);
	}
#endif
	end_RUN_IN_THREAD_STACK();
}

void my_actor::none_stack_decommit(bool calcUsingStack)
{
#ifdef WIN32
	stack_decommit(calcUsingStack);
#elif __linux__
	char* const sp = (char*)((size_t)get_sp() & (0 - PAGE_SIZE));
	begin_RUN_IN_THREAD_STACK(this);
	context_yield::context_info* const info = ((actor_pull_type*)_actorPull)->_coroInfo;
	char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
	if (sp > sb)
	{
		if (calcUsingStack)
		{
			void** tlsBuff = io_engine::getTlsValueBuff();
			void* old = tlsBuff[ACTOR_TLS_INDEX];
			tlsBuff[ACTOR_TLS_INDEX] = this;
			_usingStackSize = std::max(_usingStackSize, info->stackSize + info->reserveSize - actor_run::none_size(this));
			tlsBuff[ACTOR_TLS_INDEX] = old;
		}
		madvise(sb, sp - sb, MADV_DONTNEED);
	}
	end_RUN_IN_THREAD_STACK();
#endif
}

void my_actor::close_msg_notifer(actor_msg_handle_base& amh)
{
	assert_enter();
	amh.close();
}

bool my_actor::timed_wait_msg(int tm, actor_msg_handle<>& amh)
{
	return timed_wait_msg(tm, [this]
	{
		pull_yield();
	}, amh);
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
	return timed_pump_msg(tm, [this]
	{
		pull_yield();
	}, checkDis, pump);
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
	return timed_wait_trig(tm, [this]
	{
		pull_yield();
	}, ath);
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

bool my_actor::timed_wait_trig_sign(int tm, int id)
{
	return timed_wait_trig_sign(tm, id, [this]
	{
		pull_yield();
	});
}

bool my_actor::try_wait_trig_sign(int id)
{
	assert_enter();
	assert(id >= 0 && id < 8 * sizeof(void*));
	const size_t mask = (size_t)1 << id;
	return 0 != (mask & _trigSignMask);
}

void my_actor::wait_trig_sign(int id)
{
	timed_wait_trig_sign(-1, id);
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

reusable_mem& ActorFunc_::reu_mem(my_actor* host)
{
	return host->self_reusable();
}

void ActorFunc_::cancel_timer(my_actor* host)
{
	assert(host);
	host->cancel_timer();
}

#ifdef ENABLE_CHECK_LOST
std::shared_ptr<CheckLost_> ActorFunc_::new_check_lost(const shared_strand& strand, actor_msg_handle_base* msgHandle)
{
	auto& s_checkLostObjAlloc_ = s_checkLostObjAlloc;
	return std::shared_ptr<CheckLost_>(new(s_checkLostObjAlloc->allocate())CheckLost_(strand, msgHandle), [s_checkLostObjAlloc_](CheckLost_* p)
	{
		p->~CheckLost_();
		s_checkLostObjAlloc_->deallocate(p);
	}, ref_count_alloc<void>(s_checkLostRefCountAlloc));
}

std::shared_ptr<CheckPumpLost_> ActorFunc_::new_check_pump_lost(const actor_handle& hostActor, MsgPoolBase_* pool)
{
	auto& s_checkPumpLostObjAlloc_ = s_checkPumpLostObjAlloc;
	return std::shared_ptr<CheckPumpLost_>(new(s_checkPumpLostObjAlloc->allocate())CheckPumpLost_(hostActor, pool), [s_checkPumpLostObjAlloc_](CheckPumpLost_* p)
	{
		p->~CheckPumpLost_();
		s_checkPumpLostObjAlloc_->deallocate(p);
	}, ref_count_alloc<void>(s_checkPumpLostRefCountAlloc));
}

std::shared_ptr<CheckPumpLost_> ActorFunc_::new_check_pump_lost(actor_handle&& hostActor, MsgPoolBase_* pool)
{
	auto& s_checkPumpLostObjAlloc_ = s_checkPumpLostObjAlloc;
	return std::shared_ptr<CheckPumpLost_>(new(s_checkPumpLostObjAlloc->allocate())CheckPumpLost_(std::move(hostActor), pool), [s_checkPumpLostObjAlloc_](CheckPumpLost_* p)
	{
		p->~CheckPumpLost_();
		s_checkPumpLostObjAlloc_->deallocate(p);
	}, ref_count_alloc<void>(s_checkPumpLostRefCountAlloc));
}
#endif