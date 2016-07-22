#include "my_actor.h"
#include "async_buffer.h"
#include "sync_msg.h"
#include "bind_qt_run.h"
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) >= 61)
#include <shared_mutex>
typedef std::shared_mutex _shared_mutex;
#define _shared_lock std::shared_lock
#define _unique_lock std::unique_lock
#else
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
typedef boost::shared_mutex _shared_mutex;
#define _shared_lock boost::shared_lock
#define _unique_lock boost::unique_lock
#endif
#if (WIN32 && __GNUG__)
#include <fibersapi.h>
#endif
#ifdef __linux__
#include <signal.h>
#include <sys/mman.h>
#endif

DEBUG_OPERATION(static run_thread::thread_id s_installID);
static bool s_inited = false;
static bool s_isSelfInitFiber = false;
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
		_shared_lock<_shared_mutex> sl(_mutex);
		auto it = _table.find(key);
		if (_table.end() != it)
		{
			return it->second;
		}
		return 0;
	}

	void update_stack_size(size_t key, size_t ns)
	{
		_unique_lock<_shared_mutex> ul(_mutex);
		_table[key] = ns;
	}

	std::map<size_t, size_t> _table;
	_shared_mutex _mutex;
};

struct shared_initer 
{
	std::recursive_mutex* _traceMutex = NULL;
	std::atomic<my_actor::id>* _actorIDCount = NULL;
};
static shared_initer s_shared_initer;
static bool s_isSharedIniter = false;
static autoActorStackMng* s_autoActorStackMng = NULL;
shared_obj_pool<bool>* shared_bool::_sharedBoolPool = NULL;
std::recursive_mutex* TraceMutex_::_mutex = NULL;
std::atomic<my_actor::id>* my_actor::_actorIDCount = NULL;
msg_list_shared_alloc<actor_handle>::shared_node_alloc* my_actor::_childActorListAll = NULL;
msg_list_shared_alloc<std::function<void()> >::shared_node_alloc* my_actor::_quitExitCallbackAll = NULL;
msg_list_shared_alloc<my_actor::suspend_resume_option>::shared_node_alloc* my_actor::_suspendResumeQueueAll = NULL;
msg_map_shared_alloc<my_actor::msg_pool_status::id_key, std::shared_ptr<my_actor::msg_pool_status::pck_base> >::shared_node_alloc* my_actor::msg_pool_status::_msgTypeMapAll = NULL;

void my_actor::install()
{
	if (!s_inited)
	{
		s_inited = true;
		s_isSharedIniter = false;
		TraceMutex_::_mutex = new std::recursive_mutex;
		s_shared_initer._traceMutex = TraceMutex_::_mutex;
		DEBUG_OPERATION(s_installID = run_thread::this_thread_id());
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
		s_checkLostObjAlloc_ = new mem_alloc_mt<CheckLost_>(MEM_POOL_LENGTH);
		s_checkPumpLostObjAlloc_ = new mem_alloc_mt<CheckPumpLost_>(MEM_POOL_LENGTH);
		s_checkLostRefCountAlloc = make_ref_count_alloc<CheckLost_, mem_alloc_mt<void>>(MEM_POOL_LENGTH, [s_checkLostObjAlloc_](CheckLost_*){});
		s_checkPumpLostRefCountAlloc = make_ref_count_alloc<CheckPumpLost_, mem_alloc_mt<void>>(MEM_POOL_LENGTH, [s_checkPumpLostObjAlloc_](CheckPumpLost_*){});
#endif
		s_autoActorStackMng = new autoActorStackMng;
		shared_bool::_sharedBoolPool = create_shared_pool_mt<bool, std::mutex>(MEM_POOL_LENGTH);
		my_actor::_actorIDCount = new std::atomic<my_actor::id>(0);
		s_shared_initer._actorIDCount = my_actor::_actorIDCount;
		my_actor::_childActorListAll = new msg_list_shared_alloc<actor_handle>::shared_node_alloc(MEM_POOL_LENGTH);
		my_actor::_quitExitCallbackAll = new msg_list_shared_alloc<std::function<void()> >::shared_node_alloc(MEM_POOL_LENGTH);
		my_actor::_suspendResumeQueueAll = new msg_list_shared_alloc<my_actor::suspend_resume_option>::shared_node_alloc(MEM_POOL_LENGTH);
		my_actor::msg_pool_status::_msgTypeMapAll = new msg_map_shared_alloc<my_actor::msg_pool_status::id_key, std::shared_ptr<my_actor::msg_pool_status::pck_base> >::shared_node_alloc(MEM_POOL_LENGTH);
	}
}

void my_actor::install(const shared_initer* initer)
{
	if (!s_inited)
	{
		s_inited = true;
		s_isSharedIniter = true;
		TraceMutex_::_mutex = initer->_traceMutex;
		s_shared_initer._traceMutex = initer->_traceMutex;
		DEBUG_OPERATION(s_installID = run_thread::this_thread_id());
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
		s_checkLostObjAlloc_ = new mem_alloc_mt<CheckLost_>(MEM_POOL_LENGTH);
		s_checkPumpLostObjAlloc_ = new mem_alloc_mt<CheckPumpLost_>(MEM_POOL_LENGTH);
		s_checkLostRefCountAlloc = make_ref_count_alloc<CheckLost_, mem_alloc_mt<void>>(MEM_POOL_LENGTH, [s_checkLostObjAlloc_](CheckLost_*){});
		s_checkPumpLostRefCountAlloc = make_ref_count_alloc<CheckPumpLost_, mem_alloc_mt<void>>(MEM_POOL_LENGTH, [s_checkPumpLostObjAlloc_](CheckPumpLost_*){});
#endif
		s_autoActorStackMng = new autoActorStackMng;
		shared_bool::_sharedBoolPool = create_shared_pool_mt<bool, std::mutex>(MEM_POOL_LENGTH);
		my_actor::_actorIDCount = initer->_actorIDCount;
		s_shared_initer._actorIDCount = initer->_actorIDCount;
		my_actor::_childActorListAll = new msg_list_shared_alloc<actor_handle>::shared_node_alloc(MEM_POOL_LENGTH);
		my_actor::_quitExitCallbackAll = new msg_list_shared_alloc<std::function<void()> >::shared_node_alloc(MEM_POOL_LENGTH);
		my_actor::_suspendResumeQueueAll = new msg_list_shared_alloc<my_actor::suspend_resume_option>::shared_node_alloc(MEM_POOL_LENGTH);
		my_actor::msg_pool_status::_msgTypeMapAll = new msg_map_shared_alloc<my_actor::msg_pool_status::id_key, std::shared_ptr<my_actor::msg_pool_status::pck_base> >::shared_node_alloc(MEM_POOL_LENGTH);
	}
}

void my_actor::uninstall()
{
	if (s_inited)
	{
		s_inited = false;
		assert(run_thread::this_thread_id() == s_installID);
		delete shared_bool::_sharedBoolPool;
		shared_bool::_sharedBoolPool = NULL;
		if (!s_isSharedIniter)
			delete my_actor::_actorIDCount;
		s_shared_initer._actorIDCount = NULL;
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
		if (!s_isSharedIniter)
			delete TraceMutex_::_mutex;
		s_shared_initer._traceMutex = NULL;
		TraceMutex_::_mutex = NULL;
	}
}

const shared_initer* my_actor::get_initer()
{
	return &s_shared_initer;
}
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
CheckLost_::CheckLost_(const shared_strand& strand, msg_handle_base* msgHandle)
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

child_handle::child_handle(const child_handle&)
{
	assert(false);
}

child_handle::child_handle(child_handle&& s)
:_started(false), _quited(true)
{
	*this = std::move(s);
}

child_handle::child_handle(actor_handle&& actor)
:_actor(std::move(actor)), _started(false), _quited(false)
{
	my_actor* parent = _actor->_parentActor.get();
	parent->_childActorList.push_front(_actor);
	_actorIt = parent->_childActorList.begin();
	_actor->_quitCallback.push_front(parent->make_trig_notifer_to_self(_quiteAth));
	_athIt = _actor->_quitCallback.begin();
}

child_handle::child_handle()
:_started(false), _quited(true)
{
}

void child_handle::operator=(const child_handle&)
{
	assert(false);
}

void child_handle::operator =(child_handle&& s)
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

child_handle::~child_handle() __disable_noexcept
{
	if (!_quited)
	{
		my_actor* parent = _actor->_parentActor.get();
		assert(parent->_strand->running_in_this_thread());
		if (!ActorFunc_::is_quited(parent))
		{
			assert(parent->_inActor);
			assert(_started);
			parent->wait_trig(_quiteAth);
			peel();
		}
	}
}

const actor_handle& child_handle::get_actor() const
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

my_actor* child_handle::operator ->() const
{
	assert(_actor);
	my_actor* parent = _actor->_parentActor.get();
	assert(parent->_strand->running_in_this_thread());
	assert(parent->_inActor);
	return _actor.get();
}

void child_handle::peel()
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

child_handle::ptr child_handle::make_ptr()
{
	return ptr(new child_handle);
}

bool child_handle::empty() const
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
msg_handle_base::msg_handle_base()
:_waiting(false), _losted(false), _checkLost(false), _hostActor(NULL)
{

}

void msg_handle_base::set_actor(my_actor* hostActor)
{
	_hostActor = hostActor;
	DEBUG_OPERATION(_strand = hostActor->self_strand());
}

void msg_handle_base::check_lost(bool checkLost)
{
	assert(_strand->running_in_this_thread());
#ifndef ENABLE_CHECK_LOST
	assert(!checkLost);
#endif
	_checkLost = checkLost;
}

void msg_handle_base::lost_msg()
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

const shared_bool& msg_handle_base::dead_sign()
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

bool VoidBuffer_::empty()
{
	return 0 == _length;
}

size_t VoidBuffer_::length()
{
	return _length;
}

bool VoidBuffer_::back_is_lost_msg()
{
	return _buff.back() < 0;
}

bool VoidBuffer_::front_is_lost_msg()
{
	return _buff.front() < 0;
}

void VoidBuffer_::push_lost_front()
{
	_buff.push_front(-1);
}

void VoidBuffer_::push_lost_back()
{
	_buff.push_back(-1);
}

void VoidBuffer_::push_front()
{
	if (!empty() && !front_is_lost_msg())
	{
		_buff.front()++;
	}
	else
	{
		_buff.push_front(1);
	}
	_length++;
}

void VoidBuffer_::pop_front()
{
	if (front_is_lost_msg() || 0 == --_buff.front())
	{
		_buff.pop_front();
		_length--;
	}
}

void VoidBuffer_::push_back()
{
	if (!empty() && !back_is_lost_msg())
	{
		_buff.back()++;
	}
	else
	{
		_buff.push_back(1);
	}
	_length++;
}
//////////////////////////////////////////////////////////////////////////

MsgPoolVoid_::MsgPoolVoid_(const shared_strand& strand)
{
	_strand = strand;
	_waiting = false;
	_closed = false;
	_sendCount = 0;
}

MsgPoolVoid_::~MsgPoolVoid_()
{

}

MsgPoolVoid_::pump_handler MsgPoolVoid_::connect_pump(const std::shared_ptr<msg_pump_type>& msgPump)
{
	assert(msgPump);
	assert(_strand->running_in_this_thread());
	_msgPump = msgPump;
	pump_handler compHandler;
	compHandler._thisPool = _weakThis.lock();
	compHandler._msgPump = msgPump;
	_waiting = false;
	_sendCount = 0;
	return compHandler;
}

void MsgPoolVoid_::push_msg(const actor_handle& hostActor)
{
	if (_strand->running_in_this_thread())
	{
		post_msg(hostActor);
	}
	else
	{
		_strand->post(std::bind([](actor_handle& hostActor, const std::shared_ptr<MsgPoolVoid_>& sharedThis)
		{
			sharedThis->send_msg(hostActor);
		}, hostActor, _weakThis.lock()));
	}
}

void MsgPoolVoid_::_lost_msg(actor_handle& hostActor)
{
	if (_waiting)
	{
		_waiting = false;
		assert(_msgPump);
		assert(0 == _msgBuff.length());
		_sendCount++;
		_msgPump->lost_msg(std::move(hostActor));
	}
	else
	{
		_msgBuff.push_lost_back();
	}
}

void MsgPoolVoid_::lost_msg(const actor_handle& hostActor)
{
	if (!_closed)
	{
		_strand->try_tick(std::bind([](actor_handle& hostActor, const std::shared_ptr<MsgPoolVoid_>& sharedThis)
		{
			sharedThis->_lost_msg(hostActor);
		}, hostActor, _weakThis.lock()));
	}
}

void MsgPoolVoid_::lost_msg(actor_handle&& hostActor)
{
	if (!_closed)
	{
		_strand->try_tick(std::bind([](actor_handle& hostActor, const std::shared_ptr<MsgPoolVoid_>& sharedThis)
		{
			sharedThis->_lost_msg(hostActor);
		}, std::move(hostActor), _weakThis.lock()));
	}
}

void MsgPoolVoid_::send_msg(actor_handle& hostActor)
{
	if (_waiting)
	{
		_waiting = false;
		assert(_msgPump);
		assert(0 == _msgBuff.length());
		_sendCount++;
		_msgPump->receive_msg(std::move(hostActor));
	}
	else
	{
		_msgBuff.push_back();
	}
}

void MsgPoolVoid_::post_msg(const actor_handle& hostActor)
{
	if (_waiting)
	{
		_waiting = false;
		assert(_msgPump);
		assert(0 == _msgBuff.length());
		_sendCount++;
		_msgPump->receive_msg_tick(hostActor);
	}
	else
	{
		_msgBuff.push_back();
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
	if (suck.has())
	{
		_msgBuff.push_front();
	}
	else
	{
		_msgBuff.push_lost_front();
	}
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
			if (!_thisPool->_msgBuff.empty())
			{
				_thisPool->_sendCount++;
#ifdef ENABLE_CHECK_LOST
				if (_thisPool->_msgBuff.front_is_lost_msg())
				{
					_thisPool->_msgBuff.pop_front();
					_msgPump->lost_msg(std::move(hostActor));
				} 
				else
#endif
				{
					_thisPool->_msgBuff.pop_front();
					_msgPump->receive_msg(std::move(hostActor));
				}
			}
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
		assert(_thisPool->_msgBuff.empty());
		assert(pumpID == _thisPool->_sendCount);
	}
}

bool MsgPoolVoid_::pump_handler::try_pump(my_actor* host, unsigned char pumpID, bool& wait, bool& losted)
{
	assert(_thisPool);
	return host->send<bool>(_thisPool->_strand, std::bind([&wait, &losted, pumpID](pump_handler& pump)->bool
	{
		bool ok = false;
		auto& thisPool_ = pump._thisPool;
		if (pump._msgPump == thisPool_->_msgPump)
		{
			if (pumpID == thisPool_->_sendCount)
			{
				if (!thisPool_->_msgBuff.empty())
				{
#ifdef ENABLE_CHECK_LOST
					if (thisPool_->_msgBuff.front_is_lost_msg())
					{
						thisPool_->_msgBuff.pop_front();
						losted = true;
						ok = true;
					}
					else
#endif
					{
						thisPool_->_msgBuff.pop_front();
						ok = true;
					}
				}
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
	}, *this));
}

size_t MsgPoolVoid_::pump_handler::size(my_actor* host, unsigned char pumpID)
{
	assert(_thisPool);
	return host->send<size_t>(_thisPool->_strand, std::bind([pumpID](pump_handler& pump)->size_t
	{
		auto& thisPool_ = pump._thisPool;
		if (pump._msgPump == thisPool_->_msgPump)
		{
			if (pumpID == thisPool_->_sendCount)
			{
				return thisPool_->_msgBuff.length();
			}
			else
			{
				return thisPool_->_msgBuff.length() + 1;
			}
		}
		return 0;
	}, *this));
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
				return _thisPool->_msgBuff.length();
			}
			else
			{
				return _thisPool->_msgBuff.length() + 1;
			}
		}
		return 0;
	}
	return _thisPool->_msgBuff.length();
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

void MsgPumpVoid_::backflow(stack_obj<msg_type>& suck, bool& losted)
{
	if (_hasMsg)
	{
		_hasMsg = false;
		suck.create();
	}
	losted = _losted;
	_losted = false;
}

void MsgPumpVoid_::connect(const pump_handler& pumpHandler)
{
	assert(_strand->running_in_this_thread());
	assert(_hostActor);
	_pumpHandler = pumpHandler;
	_pumpCount = 0;
	if (_waiting)
	{
		_pumpHandler.start_pump(_pumpCount);
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
			_losted = false;
			_hasMsg = true;
		}
	}
}

void MsgPumpVoid_::_lost_msg()
{
	_losted = true;
	_pumpCount++;
	if (_waiting && _checkLost)
	{
		_waiting = false;
		_checkDis = false;
		_dstRec = NULL;
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
		bool losted = false;
		if (_pumpHandler.try_pump(_hostActor, _pumpCount, wait, losted))
		{
#ifdef ENABLE_CHECK_LOST
			if (wait)
			{
				_waiting = true;
				ActorFunc_::push_yield(_hostActor);
				assert(!_waiting);
				if (!_losted)
				{
					return true;
				}
				return false;
			}
			else if (!losted)
			{
				return true;
			}
			_losted = true;
#else
			if (wait)
			{
				_waiting = true;
				ActorFunc_::push_yield(_hostActor);
				assert(!_waiting);
			}
			return true;
#endif
		}
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

bool mutex_block_quit::go_run(bool& isRun)
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

bool mutex_block_sign::go_run(bool& isRun)
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
		assert(MEM_ALIGN(sizeof(my_actor), sizeof(void*))+sizeof(_Ty) <= _pull->_spaceSize);
		return (pointer)((char*)_pull->_space + MEM_ALIGN(sizeof(my_actor), sizeof(void*)));
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
	if (_locked && !ActorFunc_::is_quited(_self))
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

ActorGo_::ActorGo_(const shared_strand& strand, size_t stackSize)
: _strand(strand), _stackSize(stackSize) {}

ActorGo_::ActorGo_(shared_strand&& strand, size_t stackSize)
: _strand(std::move(strand)), _stackSize(stackSize) {}

ActorGo_::ActorGo_(io_engine& ios, size_t stackSize)
: _strand(boost_strand::create(ios)), _stackSize(stackSize) {}

ActorReadyGo_::ActorReadyGo_(const shared_strand& strand, size_t stackSize)
: _strand(strand), _stackSize(stackSize) {}

ActorReadyGo_::ActorReadyGo_(shared_strand&& strand, size_t stackSize)
: _strand(std::move(strand)), _stackSize(stackSize) {}

ActorReadyGo_::ActorReadyGo_(io_engine& ios, size_t stackSize)
: _strand(boost_strand::create(ios)), _stackSize(stackSize) {}

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
			_actor._yieldCount++;
			_actor._inActor = true;
			_actor.push_yield();
			assert(_actor.running_in_actor_strand());
			_actor._mainFunc(&_actor);
			assert(_actor._inActor);
			assert(!_actor._lockQuit);
			assert(!_actor._lockSuspend);
			assert(_actor._childActorList.empty());
			assert(_actor._beginQuitExec.empty());
		}
		catch (my_actor::force_quit_exception&)
		{//捕获Actor被强制退出异常
			assert(!_actor._inActor);
			_actor._inActor = true;
		}
		catch (my_actor::stack_exhaustion_exception&)
		{
			trace_line("\nerror: ", "stack_exhaustion_exception");
			assert(false);
			exit(10);
		}
		catch (pump_disconnected_exception&)
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
		assert(_actor._timerStateCompleted);
		_actor._quited = true;
		_actor._msgPoolStatus.clear(&_actor);//yield now
		_actor._inActor = false;
	}

	void exit_notify()
	{
		DEBUG_OPERATION(size_t yc = _actor.yield_count());
		_actor._exited = true;
		while (!_actor._suspendResumeQueue.empty())
		{
			if (_actor._suspendResumeQueue.front()._h)
			{
				CHECK_EXCEPTION(_actor._suspendResumeQueue.front()._h);
			}
			_actor._suspendResumeQueue.pop_front();
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
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(sb, &mbi, sizeof(mbi));
		assert(sb == mbi.AllocationBase);
		if (MEM_RESERVE == mbi.State)
		{
			assert(0 == mbi.Protect);
			return mbi.RegionSize + MEM_PAGE_SIZE;
		}
		else if ((PAGE_READWRITE | PAGE_GUARD) == mbi.Protect)
		{
			assert(MEM_COMMIT == mbi.State && MEM_PAGE_SIZE == mbi.RegionSize);
			return MEM_PAGE_SIZE;
		}
		return 0;
	}

	void check_stack()
	{
		context_yield::context_info* const info = _actor._actorPull->_coroInfo;
		const size_t cleanSize = clean_size(info);
		_actor._usingStackSize = std::max(_actor._usingStackSize, info->stackSize + info->reserveSize - cleanSize);
		//记录本次实际消耗的栈空间
		s_autoActorStackMng->update_stack_size(_actor._actorKey, _actor._usingStackSize);
		if (!_actor._afterExitCleanStack && cleanSize < info->reserveSize - MEM_PAGE_SIZE)
		{
			//释放物理内存，保留地址空间
			char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
			VirtualFree(sb + cleanSize - MEM_PAGE_SIZE, info->reserveSize - cleanSize, MEM_DECOMMIT);
			DWORD oldPro = 0;
			VirtualProtect(sb + info->reserveSize - MEM_PAGE_SIZE, MEM_PAGE_SIZE, PAGE_READWRITE | PAGE_GUARD, &oldPro);
		}
	}
#ifdef _MSC_VER
	void run(actor_push_type& actorPush)
	{
		__try
		{
			actor_handler(actorPush);
			if (_actor._checkStack)
			{//从实际栈底查看有多少个PAGE的PAGE_GUARD标记消失和用了多少栈预留空间
				_actor.run_in_thread_stack_after_quited([this]
				{
					check_stack();
					exit_notify();
				});
			}
			else
			{
				exit_notify();
			}
			if (_actor._afterExitCleanStack)
			{
				_actor._actorPull->_tick = 1;
			}
		}
		__except (seh_exception_handler(GetExceptionCode(), GetExceptionInformation()))
		{
			exit(100);
		}
	}
#elif __GNUG__
	void run(actor_push_type& actorPush)
	{
		__seh_try
		{
			actor_handler(actorPush);
			if (_actor._checkStack)
			{//从实际栈底查看有多少个PAGE的PAGE_GUARD标记消失和用了多少栈预留空间
				_actor.run_in_thread_stack_after_quited([this]
				{
					check_stack();
					exit_notify();
				});
			}
			else
			{
				exit_notify();
			}
			if (_actor._afterExitCleanStack)
			{
				_actor._actorPull->_tick = 1;
			}
		}
		__seh_except(seh_exception_handler(GetExceptionCode(), GetExceptionInformation()))
		{
			exit(100);
		}
		__seh_end_except
	}
#endif

	DWORD seh_exception_handler(DWORD ecd, _EXCEPTION_POINTERS* eInfo)
	{
		context_yield::context_info* const info = _actor._actorPull->_coroInfo;
		size_t const ts = info->stackSize + info->reserveSize;
		char* const sb = (char*)info->stackTop - ts;
#ifdef _WIN64
		char* const sp = (char*)((size_t)eInfo->ContextRecord->Rsp & (-MEM_PAGE_SIZE));
#else
		char* const sp = (char*)((size_t)eInfo->ContextRecord->Esp & (-MEM_PAGE_SIZE));
#endif
		void* const fault_address = (void*)eInfo->ExceptionRecord->ExceptionInformation[1];
		char* const violationAddr = (char*)((size_t)fault_address & (-MEM_PAGE_SIZE));
		if (violationAddr >= sb && violationAddr < info->stackTop)
		{
			if (STATUS_ACCESS_VIOLATION == ecd)
			{
				if (violationAddr - MEM_PAGE_SIZE >= sb)
				{
					VirtualAlloc(violationAddr - MEM_PAGE_SIZE, MEM_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD);
				}
				size_t l = 0;
				while (violationAddr + l < sp)
				{
					DWORD oldPro = 0;
					MEMORY_BASIC_INFORMATION mbi;
					VirtualQuery(violationAddr + l, &mbi, sizeof(mbi));
					if (MEM_RESERVE == mbi.State)
					{
						VirtualAlloc(violationAddr + l, MEM_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
					}
					else
					{
						VirtualProtect(violationAddr + l, MEM_PAGE_SIZE, PAGE_READWRITE, &oldPro);
						if ((PAGE_READWRITE | PAGE_GUARD) == oldPro)
						{
							break;
						}
					}
					l += MEM_PAGE_SIZE;
				}
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			else if (STATUS_GUARD_PAGE_VIOLATION == ecd)
			{
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			else if (STATUS_STACK_OVERFLOW == ecd)
			{
// 				if (violationAddr > sb + MEM_PAGE_SIZE)
// 				{
// 					DWORD oldPro = 0;
// 					VirtualProtect(violationAddr, MEM_PAGE_SIZE, PAGE_READWRITE, &oldPro);
// 					VirtualProtect(violationAddr - MEM_PAGE_SIZE, MEM_PAGE_SIZE, PAGE_READWRITE | PAGE_GUARD, &oldPro);
// 					return EXCEPTION_CONTINUE_EXECUTION;
// 				}
			}
		}
		struct local_ref 
		{
			size_t ts;
			void* sb;
			void* fault_address;
		} ref = { ts, sb, fault_address };
		TraceMutex_ mt;
		ContextPool_::coro_pull_interface* pull = ContextPool_::getContext(DEFAULT_STACKSIZE);
		pull->_param = &ref;
		if (STATUS_STACK_OVERFLOW == ecd)
		{
			pull->_currentHandler = [](ContextPool_::coro_push_interface& push, void* p)
			{
				local_ref* ref = (local_ref*)p;
				std::cout << "actor stack overflow";
				std::cout << ", stack base 0x" << (void*)ref->sb;
				std::cout << ", stack length " << ref->ts;
				std::cout << ", access address 0x" << ref->fault_address;
				std::cout << std::endl;
			};
		}
		else
		{
			pull->_currentHandler = [](ContextPool_::coro_push_interface& push, void* p)
			{
				local_ref* ref = (local_ref*)p;
				std::cout << "actor segmentation fault address 0x" << ref->fault_address << std::endl;
			};
		}
		pull->yield();
#ifdef ENABLE_DUMP_STACK
		pull->_param = eInfo;
		pull->_currentHandler = [](ContextPool_::coro_push_interface& push, void* p)
		{
			_EXCEPTION_POINTERS* eInfo = (_EXCEPTION_POINTERS*)p;
#ifdef _WIN64
			std::list<stack_line_info> stackList = get_stack_list((void*)eInfo->ContextRecord->Rbp, (void*)eInfo->ContextRecord->Rsp, (void*)eInfo->ContextRecord->Rip, 32, 0, true, true);
#else
			std::list<stack_line_info> stackList = get_stack_list((void*)eInfo->ContextRecord->Ebp, (void*)eInfo->ContextRecord->Esp, (void*)eInfo->ContextRecord->Eip, 32, 0, true, true);
#endif
			for (stack_line_info& ele : stackList)
			{
				std::wcout << ele << std::endl;
			}
			std::wcout << "exit" << std::endl << std::flush;
		};
		pull->yield();
#endif
		ContextPool_::recovery(pull);
		exit(102);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
#elif __linux__

	static size_t clean_size(context_yield::context_info* const info)
	{
		unsigned char mvec[256];
		const size_t totalStackSize = info->stackSize + info->reserveSize;
		char* const sb = (char*)info->stackTop - totalStackSize;
		bool ok = 0 == mincore(sb, totalStackSize, mvec);
		assert(ok);
		for (size_t i = 0; i < totalStackSize; i += MEM_PAGE_SIZE)
		{
			if (0 != mvec[i / MEM_PAGE_SIZE])
			{
				return i;
			}
		}
		return totalStackSize;
	}

	void check_stack()
	{
		context_yield::context_info* const info = _actor._actorPull->_coroInfo;
		const size_t cleanSize = clean_size(info);
		_actor._usingStackSize = std::max(_actor._usingStackSize, info->stackSize + info->reserveSize - cleanSize);
		//记录本次实际消耗的栈空间
		s_autoActorStackMng->update_stack_size(_actor._actorKey, _actor._usingStackSize);
		if (!_actor._afterExitCleanStack && cleanSize < info->reserveSize)
		{
			//释放物理内存，保留地址空间
			char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
			madvise(sb + cleanSize, info->reserveSize - cleanSize, MADV_DONTNEED);
		}
	}

	void run(actor_push_type& actorPush)
	{
		actor_handler(actorPush);
		if (_actor._checkStack)
		{
			_actor.run_in_thread_stack_after_quited([this]
			{
				check_stack();
				exit_notify();
			});
		}
		else
		{
			exit_notify();
		}
		if (_actor._afterExitCleanStack)
		{
			_actor._actorPull->_tick = 1;
		}
	}

#if (__linux__ && ENABLE_DUMP_STACK)
	static void dump_segmentation_fault(void* sp, size_t length)
	{
		stack_t sigaltStack;
		sigaltStack.ss_size = length;
		sigaltStack.ss_sp = sp;
		sigaltStack.ss_flags = 0;
		sigaltstack(&sigaltStack, NULL);

		struct sigaction sigAction;
		memset(&sigAction, 0, sizeof(sigAction));
		sigemptyset(&sigAction.sa_mask);
		sigAction.sa_flags = SA_SIGINFO | SA_ONSTACK;
		sigAction.sa_sigaction = [](int signum, siginfo_t* info, void* ptr)
		{
			TraceMutex_ mt;
			ucontext_t* const ucontext = (ucontext_t*)ptr;
#if (__i386__ || __x86_64__)
			void* const fault_address = (void*)ucontext->uc_sigmask.__val[3];
#elif (_ARM32 || _ARM64)
			void* const fault_address = (void*)ucontext->uc_mcontext.fault_address;
#endif
			my_actor* const self = my_actor::self_actor();
			if (self)
			{
				context_yield::context_info* const info = self->_actorPull->_coroInfo;
				char* const violationAddr = (char*)((size_t)fault_address & (0 - MEM_PAGE_SIZE));
				size_t const ts = info->stackSize + info->reserveSize;
				char* const sb = (char*)info->stackTop - ts;
				if (violationAddr >= sb && violationAddr < info->stackTop)
				{
					//触碰栈哨兵
					assert(violationAddr == sb);
					std::wcout << "actor stack overflow";
					std::wcout << ", stack base " << (void*)sb;
					std::wcout << ", stack length " << ts;
					std::wcout << ", access address " << fault_address;
					std::wcout << std::endl;
				}
				else
				{
					std::wcout << "actor segmentation fault address " << fault_address << std::endl;
				}
			}
			else
			{
				std::wcout << "segmentation fault address " << fault_address << std::endl;
			}
			std::wcout << "analyzing stack..." << std::endl;
#ifdef __x86_64__
			std::list<stack_line_info> stk = get_stack_list((void*)ucontext->uc_mcontext.gregs[REG_RBP], NULL, (void*)ucontext->uc_mcontext.gregs[REG_RIP], 32, 0, true, true);
#elif __i386__
			std::list<stack_line_info> stk = get_stack_list((void*)ucontext->uc_mcontext.gregs[REG_EBP], NULL, (void*)ucontext->uc_mcontext.gregs[REG_EIP], 32, 0, true, true);
#elif _ARM32
			std::list<stack_line_info> stk = get_stack_list(NULL, (void*)ucontext->uc_mcontext.arm_sp, (void*)ucontext->uc_mcontext.arm_lr, 32, 0, true, true);
#elif _ARM64
			std::list<stack_line_info> stk;
#endif
			for (stack_line_info& ele : stk)
			{
				std::wcout << ele << std::endl;
			}
			std::wcout << "exit" << std::endl << std::flush;
			exit(102);
		};
		sigaction(SIGSEGV, &sigAction, NULL);
	}

	static void undump_segmentation_fault()
	{
		stack_t sigaltStack;
		sigaltstack(NULL, &sigaltStack);
	}
#endif

#endif

private:
	my_actor& _actor;
};

#if (__linux__ && ENABLE_DUMP_STACK)
void my_actor::dump_segmentation_fault(void* sp, size_t length)
{
	actor_run::dump_segmentation_fault(sp, length);
}

void my_actor::undump_segmentation_fault()
{
	actor_run::undump_segmentation_fault();
}
#endif

//////////////////////////////////////////////////////////////////////////

my_actor::my_actor()
:_suspendResumeQueue(*_suspendResumeQueueAll),
_quitCallback(*_quitExitCallbackAll),
_beginQuitExec(*_quitExitCallbackAll),
_childActorList(*_childActorListAll)
{
	_actorPull = NULL;
	_actorPush = NULL;
	_timer = NULL;
	_timerStateCb = NULL;
	_alsVal = NULL;
	_timerStateSuspend = false;
	_timerStateCompleted = true;
	_inActor = false;
	_started = false;
	_quited = false;
	_exited = false;
	_suspended = false;
	_isForce = false;
	_holdPull = false;
	_holdQuited = false;
	_holdSuspended = false;
	_checkStack = false;
	_waitingQuit = false;
	_afterExitCleanStack = false;
#ifdef PRINT_ACTOR_STACK
	_checkStackFree = false;
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
	_timerStateStampEnd = 0;
}

my_actor::my_actor(const my_actor&)
{

}

my_actor::~my_actor()
{
	assert(_quited);
	assert(_exited);
	assert(!_lockSuspend);
	assert(!_holdPull);
	assert(!_holdQuited);
	assert(!_holdSuspended);
	assert(!_waitingQuit);
	assert(!_mainFunc);
	assert(!_childOverCount);
	assert(!_lockQuit);
	assert(!_childSuspendResumeCount);
	assert(_suspendResumeQueue.empty());
	assert(_beginQuitExec.empty());
	assert(_quitCallback.empty());
	assert(_childActorList.empty());

#ifdef PRINT_ACTOR_STACK
	context_yield::context_info* const info = _actorPull->_coroInfo;
	if (_checkStackFree || _checkStack || _usingStackSize > info->stackSize)
	{
		stack_overflow_format(_usingStackSize - info->stackSize, std::move(_createStack));
	}
#endif
}

my_actor& my_actor::operator =(const my_actor&)
{
	return *this;
}

actor_handle my_actor::create(shared_strand&& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	return my_actor::create(std::move(actorStrand), main_func(mainFunc), stackSize);
}

actor_handle my_actor::create(shared_strand&& actorStrand, main_func&& mainFunc, size_t stackSize)
{
	actor_pull_type* pull = ContextPool_::getContext(stackSize);
	if (!pull)
	{
		error_trace_line("stack memory exhaustion");
		throw stack_exhaustion_exception();
		return actor_handle();
	}
	actor_handle newActor(new(pull->_space)my_actor(), [](my_actor* p){p->~my_actor(); }, actor_ref_count_alloc<void>(pull));
	newActor->_weakThis = newActor;
	newActor->_timer = actorStrand->actor_timer();
	newActor->_strand = std::move(actorStrand);
	newActor->_mainFunc = std::move(mainFunc);
	newActor->_actorPull = pull;
#ifdef PRINT_ACTOR_STACK
	newActor->_createStack = get_stack_list(8, 1);
#endif

	pull->_param = newActor.get();
	pull->_currentHandler = [](actor_push_type& push, void* p)
	{
		(actor_run(*(my_actor*)p)).run(push);
	};
	pull->yield();
	return newActor;
}

actor_handle my_actor::create(shared_strand&& actorStrand, AutoStackActorFace_&& wrapActor)
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
		pull = ContextPool_::getContext(lasts ? lasts : MAX_STACKSIZE);
	}
	if (!pull)
	{
		error_trace_line("stack memory exhaustion");
		throw stack_exhaustion_exception();
		return actor_handle();
	}
	actor_handle newActor(new(pull->_space)my_actor(), [](my_actor* p){p->~my_actor(); }, actor_ref_count_alloc<void>(pull));
	newActor->_weakThis = newActor;
	newActor->_timer = actorStrand->actor_timer();
	newActor->_strand = std::move(actorStrand);
	newActor->_checkStack = checkStack;
	wrapActor.swap(newActor->_mainFunc);
	newActor->_actorKey = wrapActor.key();
	newActor->_actorPull = pull;
#ifdef PRINT_ACTOR_STACK
	newActor->_createStack = get_stack_list(8, 1);
#endif

	pull->_param = newActor.get();
	pull->_currentHandler = [](actor_push_type& push, void* p)
	{
		(actor_run(*(my_actor*)p)).run(push);
	};
	pull->_tick = 1;
	pull->yield();
	return newActor;
}

actor_handle my_actor::create(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	return create(shared_strand(actorStrand), main_func(mainFunc), stackSize);
}

actor_handle my_actor::create(const shared_strand& actorStrand, main_func&& mainFunc, size_t stackSize)
{
	return create(shared_strand(actorStrand), std::move(mainFunc), stackSize);
}

actor_handle my_actor::create(const shared_strand& actorStrand, AutoStackActorFace_&& wrapActor)
{
	return create(shared_strand(actorStrand), std::move(wrapActor));
}

child_handle my_actor::create_child(shared_strand&& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	assert_enter();
	actor_handle childActor = my_actor::create(std::move(actorStrand), mainFunc, stackSize);
	childActor->_parentActor = shared_from_this();
	return child_handle(std::move(childActor));
}

child_handle my_actor::create_child(shared_strand&& actorStrand, main_func&& mainFunc, size_t stackSize)
{
	assert_enter();
	actor_handle childActor = my_actor::create(std::move(actorStrand), std::move(mainFunc), stackSize);
	childActor->_parentActor = shared_from_this();
	return child_handle(std::move(childActor));
}

child_handle my_actor::create_child(const main_func& mainFunc, size_t stackSize)
{
	return create_child(_strand, mainFunc, stackSize);
}

child_handle my_actor::create_child(main_func&& mainFunc, size_t stackSize)
{
	return create_child(_strand, std::move(mainFunc), stackSize);
}

child_handle my_actor::create_child(shared_strand&& actorStrand, AutoStackActorFace_&& wrapActor)
{
	assert_enter();
	actor_handle childActor = my_actor::create(std::move(actorStrand), std::move(wrapActor));
	childActor->_parentActor = shared_from_this();
	return child_handle(std::move(childActor));
}

child_handle my_actor::create_child(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	return create_child(shared_strand(actorStrand), mainFunc, stackSize);
}

child_handle my_actor::create_child(const shared_strand& actorStrand, main_func&& mainFunc, size_t stackSize)
{
	return create_child(shared_strand(actorStrand), std::move(mainFunc), stackSize);
}

child_handle my_actor::create_child(const shared_strand& actorStrand, AutoStackActorFace_&& wrapActor)
{
	return create_child(shared_strand(actorStrand), std::move(wrapActor));
}

child_handle my_actor::create_child(AutoStackActorFace_&& wrapActor)
{
	return create_child(_strand, std::move(wrapActor));
}

void my_actor::child_run(child_handle& actorHandle)
{
	assert_enter();
	assert(!actorHandle._started);
	assert(!actorHandle._quited);
	assert(actorHandle.get_actor());
	assert(actorHandle->parent_actor()->self_id() == self_id());
	actorHandle._started = true;
	actorHandle->run();
}

void my_actor::children_run(std::list<child_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		child_run(*actorHandle);
	}
}

void my_actor::children_run(std::list<child_handle>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		child_run(actorHandle);
	}
}

void my_actor::child_force_quit(child_handle& actorHandle)
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle->parent_actor()->self_id() == self_id());
		actorHandle->force_quit();
		wait_trig(actorHandle._quiteAth);
		actorHandle.peel();
	}
}

void my_actor::children_force_quit(std::list<child_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		if (!actorHandle->_quited)
		{
			assert(actorHandle->get_actor());
			assert((*actorHandle)->parent_actor()->self_id() == self_id());
			(*actorHandle)->force_quit();
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

void my_actor::children_force_quit(std::list<child_handle>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		if (!actorHandle._quited)
		{
			assert(actorHandle.get_actor());
			assert(actorHandle->parent_actor()->self_id() == self_id());
			actorHandle->force_quit();
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

void my_actor::child_wait_quit(child_handle& actorHandle)
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle->parent_actor()->self_id() == self_id());
		assert(actorHandle._started);
		wait_trig(actorHandle._quiteAth);
		actorHandle.peel();
	}
}

void my_actor::children_wait_quit(std::list<child_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle->get_actor()->parent_actor()->self_id() == self_id());
		child_wait_quit(*actorHandle);
	}
}

void my_actor::children_wait_quit(std::list<child_handle>& actorHandles)
{
	assert_enter();
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle->parent_actor()->self_id() == self_id());
		child_wait_quit(actorHandle);
	}
}

bool my_actor::timed_child_wait_quit(int tm, child_handle& actorHandle)
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle->parent_actor()->self_id() == self_id());
		if (!timed_wait_trig(tm, actorHandle._quiteAth))
		{
			return false;
		}
		actorHandle.peel();
	}
	return true;
}

void my_actor::child_suspend(child_handle& actorHandle)
{
	assert_enter();
	lock_quit();
	assert(actorHandle.get_actor());
	assert(actorHandle->parent_actor()->self_id() == self_id());
	trig([&actorHandle](trig_once_notifer<>&& h){actorHandle->suspend(std::move(h)); });
	unlock_quit();
}

void my_actor::children_suspend(std::list<child_handle::ptr>& actorHandles)
{
	assert_enter();
	lock_quit();
	msg_handle<> amh;
	msg_notifer<> h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle->get_actor());
		assert((*actorHandle)->parent_actor()->self_id() == self_id());
		(*actorHandle)->suspend(wrap_ref_handler(h));
	}
	for (size_t i = actorHandles.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
	unlock_quit();
}

void my_actor::children_suspend(std::list<child_handle>& actorHandles)
{
	assert_enter();
	lock_quit();
	msg_handle<> amh;
	msg_notifer<> h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle->parent_actor()->self_id() == self_id());
		actorHandle->suspend(wrap_ref_handler(h));
	}
	for (size_t i = actorHandles.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
	unlock_quit();
}

void my_actor::child_resume(child_handle& actorHandle)
{
	assert_enter();
	lock_quit();
	assert(actorHandle.get_actor());
	assert(actorHandle->parent_actor()->self_id() == self_id());
	trig([&actorHandle](trig_once_notifer<>&& h){actorHandle->resume(std::move(h)); });
	unlock_quit();
}

void my_actor::children_resume(std::list<child_handle::ptr>& actorHandles)
{
	assert_enter();
	lock_quit();
	msg_handle<> amh;
	msg_notifer<> h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle->get_actor());
		assert((*actorHandle)->parent_actor()->self_id() == self_id());
		(*actorHandle)->resume(wrap_ref_handler(h));
	}
	for (size_t i = actorHandles.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
	unlock_quit();
}

void my_actor::children_resume(std::list<child_handle>& actorHandles)
{
	assert_enter();
	lock_quit();
	msg_handle<> amh;
	msg_notifer<> h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : actorHandles)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle->parent_actor()->self_id() == self_id());
		actorHandle->resume(wrap_ref_handler(h));
	}
	for (size_t i = actorHandles.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
	unlock_quit();
}

void my_actor::run_child_complete(shared_strand&& actorStrand, const main_func& h, size_t stackSize)
{
	assert_enter();
	child_handle actorHandle = create_child(std::move(actorStrand), h, stackSize);
	child_run(actorHandle);
	child_wait_quit(actorHandle);
}

void my_actor::run_child_complete(shared_strand&& actorStrand, main_func&& h, size_t stackSize)
{
	assert_enter();
	child_handle actorHandle = create_child(std::move(actorStrand), std::move(h), stackSize);
	child_run(actorHandle);
	child_wait_quit(actorHandle);
}

void my_actor::run_child_complete(const main_func& h, size_t stackSize)
{
	run_child_complete(self_strand(), h, stackSize);
}

void my_actor::run_child_complete(main_func&& h, size_t stackSize)
{
	run_child_complete(self_strand(), std::move(h), stackSize);
}

void my_actor::run_child_complete(const shared_strand& actorStrand, const main_func& h, size_t stackSize)
{
	run_child_complete(shared_strand(actorStrand), h, stackSize);
}

void my_actor::run_child_complete(const shared_strand& actorStrand, main_func&& h, size_t stackSize)
{
	run_child_complete(shared_strand(actorStrand), std::move(h), stackSize);
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
		timeout(ms, [this]{run_one(); });
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

void my_actor::tick_yield()
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

void my_actor::try_tick_yield()
{
	if (_lastYield == _yieldCount)
	{
		tick_yield();
	}
	_lastYield = _yieldCount;
}

const actor_handle& my_actor::parent_actor()
{
	return _parentActor;
}

const msg_list_shared_alloc<actor_handle>& my_actor::children()
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
	_beginQuitExec.push_front(std::move(quitHandler));//后注册的先执行
	return _beginQuitExec.begin();
}

void my_actor::cancel_quit_executor(const quit_iterator& qh)
{
	assert_enter();
	_beginQuitExec.erase(qh);
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

io_engine& my_actor::self_io_engine()
{
	return _strand->get_io_engine();
}

actor_handle my_actor::shared_from_this()
{
	return _weakThis.lock();
}

async_timer my_actor::make_timer()
{
	return _strand->make_timer();
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
	return _actorPull->_coroInfo->stackSize;
}

size_t my_actor::stack_total_size()
{
	context_yield::context_info* const info = _actorPull->_coroInfo;
	return info->stackSize + info->reserveSize;
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

void my_actor::run()
{
	_strand->try_tick(std::bind([](const actor_handle& shared_this)
	{
		my_actor* const self = shared_this.get();
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
	context_yield::context_info* const info = _actorPull->_coroInfo;
	assert((size_t)get_sp() >= (size_t)info->stackTop - info->stackSize - info->reserveSize + 1024);
	check_self();
#endif
}

void my_actor::force_quit()
{
	force_quit(std::function<void()>());
}

void my_actor::force_quit(const std::function<void()>& h)
{
	force_quit(std::function<void()>(h));
}

void my_actor::force_quit(std::function<void()>&& h)
{
	_strand->try_tick(std::bind([](actor_handle& shared_this, std::function<void()>& h)
	{
		my_actor* const self = shared_this.get();
		if (!self->_quited)
		{
			if (h)
			{
				self->_quitCallback.push_back(std::move(h));
			}
			if (!self->_lockQuit)
			{
				self->_isForce = true;
				self->_quited = true;
				self->_suspended = false;
				self->_holdSuspended = false;
				self->_holdPull = false;
				self->_lockSuspend = 0;
				self->cancel_timer();
				if (!self->_childActorList.empty())
				{
					self->_childOverCount = self->_childActorList.size();
					while (!self->_childActorList.empty())
					{
						actor_handle childActor = std::move(self->_childActorList.front());
						self->_childActorList.pop_front();
						childActor->force_quit(std::bind([](actor_handle& shared_this)
						{
							my_actor* const self = shared_this.get();
							self->_childOverCount--;
							if (0 == self->_childOverCount)
							{
								self->pull_yield();
							}
						}, std::move(shared_this)));
					}
				}
				else
				{
					self->pull_yield();
				}
			}
			else
			{
				self->_holdQuited = true;
				if (self->_waitingQuit)
				{
					self->_waitingQuit = false;
					self->pull_yield();
				}
			}
		}
		else if (h)
		{
			if (self->_exited)
			{
				CHECK_EXCEPTION(h);
			}
			else
			{
				self->_quitCallback.push_back(std::move(h));
			}
		}
	}, shared_from_this(), std::move(h)));
}

bool my_actor::is_started()
{
	assert(_strand->running_in_this_thread());
	return _started;
}

bool my_actor::is_exited()
{
	assert(_strand->running_in_this_thread());
	return _exited;
}

bool my_actor::is_force()
{
	assert(_exited);
	return _isForce;
}

bool my_actor::in_actor()
{
	assert(_strand->running_in_this_thread());
	return _inActor;
}

bool my_actor::running_in_actor_strand()
{
	return _strand->running_in_this_thread();
}

bool my_actor::quit_msg()
{
	assert_enter();
	return _holdQuited;
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
	if (0 == --_lockQuit && _holdQuited)
	{
		_holdQuited = false;
		force_quit();
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
	if (0 == --_lockSuspend && _holdSuspended)
	{
		_holdSuspended = false;
		assert(!_suspendResumeQueue.empty());
		suspend_resume_option opt = std::move(_suspendResumeQueue.front());
		_suspendResumeQueue.pop_front();
		if (opt._isSuspend)
		{
			suspend(std::move(opt._h));
		} 
		else
		{
			resume(std::move(opt._h));
		}
		tick_yield();
	}
}

bool my_actor::is_locked_quit()
{
	assert_enter();
	return 0 != _lockQuit;
}

bool my_actor::is_locked_suspend()
{
	assert_enter();
	return 0 != _lockSuspend;
}

void my_actor::after_exit_clean_stack()
{
	assert(_strand->running_in_this_thread());
	_afterExitCleanStack = true;
}

void my_actor::suspend()
{
	suspend(std::function<void()>());
}

void my_actor::suspend(const std::function<void()>& h)
{
	suspend(std::function<void()>(h));
}

void my_actor::suspend(std::function<void()>&& h)
{
	_strand->try_tick(std::bind([](actor_handle& shared_this, std::function<void()>& h)
	{
		my_actor* const self = shared_this.get();
		if (!self->_quited)
		{
			self->_suspendResumeQueue.push_back(suspend_resume_option(true, std::move(h)));
			if (!self->_lockSuspend)
			{
				if (1 == self->_suspendResumeQueue.size())
				{
					self->_suspended = true;
					self->suspend_timer();
					self->_suspend(std::bind([](actor_handle& shared_this)
					{
						my_actor* const self = shared_this.get();
						suspend_resume_option opt = std::move(self->_suspendResumeQueue.front());
						self->_suspendResumeQueue.pop_front();
						if (opt._h)
						{
							opt._h();
						}
						if (!self->_suspendResumeQueue.empty())
						{
							suspend_resume_option opt = std::move(self->_suspendResumeQueue.front());
							self->_suspendResumeQueue.pop_front();
							if (opt._isSuspend)
							{
								self->suspend(std::move(opt._h));
							}
							else
							{
								self->resume(std::move(opt._h));
							}
						}
					}, std::move(shared_this)));
				}
			}
			else
			{
				self->_holdSuspended = true;
			}
		}
		else if (h)
		{
			h();
		}
	}, shared_from_this(), std::move(h)));
}

void my_actor::_suspend(std::function<void()>&& h)
{
	if (!_childActorList.empty())
	{
		assert(0 == _childSuspendResumeCount);
		_childSuspendResumeCount = _childActorList.size();
		for (actor_handle& childActor : _childActorList)
		{
			childActor->suspend(std::bind([](actor_handle& shared_this, std::function<void()>& h)
			{
				my_actor* const self = shared_this.get();
				self->_childSuspendResumeCount--;
				if (0 == self->_childSuspendResumeCount)
				{
					h();
				}
			}, shared_from_this(), std::move(h)));
		}
	}
	else
	{
		h();
	}
}

void my_actor::resume()
{
	resume(std::function<void()>());
}

void my_actor::resume(const std::function<void()>& h)
{
	resume(std::function<void()>(h));
}

void my_actor::resume(std::function<void()>&& h)
{
	_strand->try_tick(std::bind([](const actor_handle& shared_this, std::function<void()>& h)
	{
		my_actor* const self = shared_this.get();
		if (!self->_quited)
		{
			self->_suspendResumeQueue.push_back(suspend_resume_option(false, std::move(h)));
			if (!self->_lockSuspend)
			{
				if (1 == self->_suspendResumeQueue.size())
				{
					self->_resume(std::bind([](actor_handle& shared_this)
					{
						my_actor* const self = shared_this.get();
						suspend_resume_option opt = std::move(self->_suspendResumeQueue.front());
						self->_suspendResumeQueue.pop_front();
						if (opt._h)
						{
							opt._h();
						}
						if (!self->_suspendResumeQueue.empty())
						{
							suspend_resume_option opt = std::move(self->_suspendResumeQueue.front());
							self->_suspendResumeQueue.pop_front();
							if (opt._isSuspend)
							{
								self->suspend(std::move(opt._h));
							}
							else
							{
								self->resume(std::move(opt._h));
							}
						}
						else
						{
							self->resume_timer();
							self->_suspended = false;
							if (self->_holdPull)
							{
								self->_holdPull = false;
								self->pull_yield();
							}
						}
					}, std::move(shared_this)));
				}
			}
			else
			{
				self->_holdSuspended = true;
			}
		}
		else if (h)
		{
			h();
		}
	}, shared_from_this(), std::move(h)));
}

void my_actor::_resume(std::function<void()>&& h)
{
	if (!_childActorList.empty())
	{
		assert(0 == _childSuspendResumeCount);
		_childSuspendResumeCount = _childActorList.size();
		for (actor_handle& childActor : _childActorList)
		{
			childActor->resume(std::bind([](actor_handle& shared_this, std::function<void()>& h)
			{
				my_actor* const self = shared_this.get();
				self->_childSuspendResumeCount--;
				if (0 == self->_childSuspendResumeCount)
				{
					h();
				}
			}, shared_from_this(), std::move(h)));
		}
	}
	else
	{
		h();
	}
}

void my_actor::switch_pause_play()
{
	switch_pause_play(std::function<void(bool isPaused)>());
}

void my_actor::switch_pause_play(const std::function<void(bool)>& h)
{
	switch_pause_play(std::function<void(bool)>(h));
}

void my_actor::switch_pause_play(std::function<void(bool)>&& h)
{
	_strand->try_tick(std::bind([](const actor_handle& shared_this, std::function<void(bool)>& h)
	{
		assert(shared_this->_strand->running_in_this_thread());
		if (!shared_this->_quited)
		{
			if (shared_this->_suspended)
			{
				if (h)
				{
					shared_this->resume(std::bind([](std::function<void(bool)>& h)
					{
						CHECK_EXCEPTION(h, false);
					}, std::move(h)));
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
					shared_this->suspend(std::bind([](std::function<void(bool)>& h)
					{
						CHECK_EXCEPTION(h, true);
					}, std::move(h)));
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
			CHECK_EXCEPTION(h, true);
			assert(shared_this->yield_count() == yc);
		}
	}, shared_from_this(), std::move(h)));
}

void my_actor::notify_trig_sign(int id)
{
	assert(id >= 0 && id < 8 * sizeof(void*));
	_strand->try_tick(std::bind([id](const actor_handle& shared_this)
	{
		my_actor* const self = shared_this.get();
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
	_strand->post([&]
	{
		assert(_strand->running_in_this_thread());
		if (_exited)
		{
			std::lock_guard<std::mutex> lg(mutex);
			conVar.notify_one();
		}
		else
		{
			_quitCallback.push_back([&]()
			{
				std::lock_guard<std::mutex> lg(mutex);
				conVar.notify_one();
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
		actorHandle->run();
	}
}

void my_actor::actor_force_quit(const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](trig_once_notifer<>&& h){anotherActor->force_quit(std::move(h)); });
}

void my_actor::actors_force_quit(const std::list<actor_handle>& anotherActors)
{
	assert_enter();
	lock_quit();
	msg_handle<> amh;
	msg_notifer<> h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->force_quit(wrap_ref_handler(h));
	}
	for (size_t i = anotherActors.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
	unlock_quit();
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
	trig_handle<> ath;
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
	trig([anotherActor](trig_once_notifer<>&& h){anotherActor->suspend(std::move(h)); });
}

void my_actor::actors_suspend(const std::list<actor_handle>& anotherActors)
{
	assert_enter();
	lock_quit();
	msg_handle<> amh;
	msg_notifer<> h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->suspend(wrap_ref_handler(h));
	}
	for (size_t i = anotherActors.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
	unlock_quit();
}

void my_actor::actor_resume(const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](trig_once_notifer<>&& h){anotherActor->resume(std::move(h)); });
}

void my_actor::actors_resume(const std::list<actor_handle>& anotherActors)
{
	assert_enter();
	lock_quit();
	msg_handle<> amh;
	msg_notifer<> h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->resume(wrap_ref_handler(h));
	}
	for (size_t i = anotherActors.size(); i > 0; i--)
	{
		wait_msg(amh);
	}
	close_msg_notifer(amh);
	unlock_quit();
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
	lock_quit();
	bool isPause = true;
	msg_handle<bool> amh;
	msg_notifer<bool> h = make_msg_notifer_to_self(amh);
	for (auto& actorHandle : anotherActors)
	{
		actorHandle->switch_pause_play(wrap_ref_handler(h));
	}
	for (size_t i = anotherActors.size(); i > 0; i--)
	{
		isPause &= wait_msg(amh);
	}
	close_msg_notifer(amh);
	unlock_quit();
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
#if ((__linux__ && (defined ENABLE_DUMP_STACK || (defined CHECK_SELF))) || (WIN32 && (_WIN32_WINNT < 0x0502) && (defined CHECK_SELF)))
	void*& tlsVal = io_engine::getTlsValueRef(ACTOR_TLS_INDEX);
	void* old = tlsVal;
	tlsVal = this;
	_actorPull->yield();
	tlsVal = old;
#else
	_actorPull->yield();
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
		assert(!_holdPull);
		_holdPull = true;
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
	_actorPush->yield();
	if (!_quited)
	{
		_inActor = true;
	}
	else
	{
		assert(!_lockQuit);
		assert(!_childOverCount);
		while (!_beginQuitExec.empty())
		{
			CHECK_EXCEPTION(_beginQuitExec.front());
			_beginQuitExec.pop_front();
		}
		throw force_quit_exception();
	}
}

void my_actor::push_yield_after_quited()
{
	check_stack();
	_inActor = false;
	_actorPush->yield();
	_inActor = true;
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
	if (!_timerStateCompleted)
	{
		assert(_timer);
		_timerStateCompleted = true;
		_timerStateCount++;
		_timer->cancel(_timerStateHandle);
		_timerStateCb->destroy();
		_reuMem.deallocate(_timerStateCb);
		_timerStateCb = NULL;
	}
}

void my_actor::suspend_timer()
{
	if (!_timerStateSuspend)
	{
		_timerStateSuspend = true;
		if (!_timerStateCompleted)
		{
			assert(_timer);
			_timer->cancel(_timerStateHandle);
			_timerStateStampEnd = get_tick_us();
			long long tt = _timerStateHandle._beginStamp + _timerStateTime;
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
		if (!_timerStateCompleted)
		{
			assert(_timer);
			assert(_timerStateTime >= _timerStateStampEnd - _timerStateHandle._beginStamp);
			_timerStateTime -= _timerStateStampEnd - _timerStateHandle._beginStamp;
			_timerStateHandle = _timer->timeout(_timerStateTime, shared_from_this());
		}
	}
}

void my_actor::check_stack()
{
#ifdef PRINT_ACTOR_STACK
	context_yield::context_info* const info = _actorPull->_coroInfo;
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
	return (my_actor*)::FlsGetValue(ContextPool_::coro_pull_interface::_actorFlsIndex);
#elif ((__linux__ && (defined ENABLE_DUMP_STACK || (defined CHECK_SELF))) || (WIN32 && (defined CHECK_SELF)))
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

void my_actor::als_set(void* val)
{
	_alsVal = val;
}

void* my_actor::als_get()
{
	return _alsVal;
}

void my_actor::close_msg_notifer(msg_handle_base& amh)
{
	assert_enter();
	amh.close();
}

bool my_actor::timed_wait_msg(int tm, msg_handle<>& amh)
{
	return timed_wait_msg(tm, [this]
	{
		pull_yield();
	}, amh);
}

bool my_actor::try_wait_msg(msg_handle<>& amh)
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
	BREAK_OF_SCOPE(
	{
		pump.get()->stop_waiting();
	});
	if (!pump.get()->try_read())
	{
		if (checkDis && pump.get()->isDisconnected())
		{
			throw msg_pump_handle<>::pump_disconnected(pump.get_id());
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

void my_actor::close_trig_notifer(msg_handle_base& ath)
{
	assert_enter();
	ath.close();
}

bool my_actor::timed_wait_trig(int tm, trig_handle<>& ath)
{
	return timed_wait_trig(tm, [this]
	{
		pull_yield();
	}, ath);
}

bool my_actor::try_wait_trig(trig_handle<>& ath)
{
	return timed_wait_trig(0, ath);
}

void my_actor::wait_msg(msg_handle<>& amh)
{
	timed_wait_msg(-1, amh);
}

void my_actor::wait_trig(trig_handle<>& ath)
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

void ActorFunc_::pull_yield_after_quited(my_actor* host)
{
	assert(host);
	host->pull_yield_after_quited();
}

void ActorFunc_::push_yield_after_quited(my_actor* host)
{
	assert(host);
	host->push_yield_after_quited();
}

bool ActorFunc_::is_quited(my_actor* host)
{
	assert(host);
	return host->_quited;
}

reusable_mem& ActorFunc_::reu_mem(my_actor* host)
{
	assert(host);
	return host->self_reusable();
}

void ActorFunc_::cancel_timer(my_actor* host)
{
	assert(host);
	host->cancel_timer();
}

#ifdef ENABLE_CHECK_LOST
std::shared_ptr<CheckLost_> ActorFunc_::new_check_lost(const shared_strand& strand, msg_handle_base* msgHandle)
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