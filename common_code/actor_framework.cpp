#define WIN32_LEAN_AND_MEAN

#include <boost/coroutine/all.hpp>
#include "actor_framework.h"
#include "actor_stack.h"
#include "async_buffer.h"
#include "sync_msg.h"

typedef boost::coroutines::coroutine<void>::pull_type actor_pull_type;
typedef boost::coroutines::coroutine<void>::push_type actor_push_type;

#ifdef CHECK_SELF
msg_map<void*, my_actor*> s_stackLine(100000);
boost::mutex s_stackLineMutex;
struct initStackLine
{
	initStackLine()
	{
		s_stackLine.insert(make_pair((void*)NULL, (my_actor*)NULL));
		s_stackLine.insert(make_pair((void*)-1, (my_actor*)NULL));
	}
} s_initStackLine;
#endif

#define CORO_CONTEXT_STATE_SPACE	(4 kB)

boost::atomic<my_actor::id> s_actorIDCount(0);//ID计数
std::shared_ptr<shared_obj_pool<bool>> s_sharedBoolPool(create_shared_pool<bool>(100000, [](void*){}));
msg_list_shared_alloc<actor_handle>::shared_node_alloc my_actor::_childActorListAll(100000);
msg_list_shared_alloc<std::function<void()> >::shared_node_alloc my_actor::_quitExitCallbackAll(100000);
msg_list_shared_alloc<my_actor::suspend_resume_option>::shared_node_alloc my_actor::_suspendResumeQueueAll(100000);
msg_map_shared_alloc<my_actor::msg_pool_status::id_key, std::shared_ptr<my_actor::msg_pool_status::pck_base> >::shared_node_alloc my_actor::msg_pool_status::_msgTypeMapAll(100000);

/*!
@brief Actor栈分配器
*/
struct actor_stack_pool_allocate
{
	actor_stack_pool_allocate() {}

	actor_stack_pool_allocate(void* sp, size_t size)
	{
		_stack.sp = sp;
		_stack.size = size;
	}

	void allocate( boost::coroutines::stack_context & stackCon, size_t size)
	{
		stackCon = _stack;
	}

	void deallocate( boost::coroutines::stack_context & stackCon)
	{

	}

	boost::coroutines::stack_context _stack;
};

struct actor_stack_allocate 
{
	actor_stack_allocate() {}

	actor_stack_allocate(void** sp, size_t* size)
	{
		_sp = sp;
		_size = size;
	}

	void allocate( boost::coroutines::stack_context & stackCon, size_t size)
	{
		boost::coroutines::stack_allocator all;
		all.allocate(stackCon, size);
		*_sp = stackCon.sp;
		*_size = stackCon.size;
	}

	void deallocate( boost::coroutines::stack_context & stackCon)
	{
		boost::coroutines::stack_allocator all;
		all.deallocate(stackCon);
	}

	void** _sp;
	size_t* _size;
};
//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
child_actor_handle::child_actor_param& child_actor_handle::child_actor_param::operator=( child_actor_handle::child_actor_param& s )
{
	_actor = s._actor;
	_actorIt = s._actorIt;
	s._isCopy = true;
	_isCopy = false;
	return *this;
}

child_actor_handle::child_actor_param::~child_actor_param()
{
	assert(_isCopy);//检测创建后有没有接收子Actor句柄
}

child_actor_handle::child_actor_param::child_actor_param( child_actor_handle::child_actor_param& s )
{
	*this = s;
}

child_actor_handle::child_actor_param::child_actor_param()
{
	_isCopy = true;
}
#endif

child_actor_handle::child_actor_handle( child_actor_handle& )
{
	assert(false);
}

child_actor_handle::child_actor_handle()
{
	_quited = true;
}

child_actor_handle::child_actor_handle( child_actor_param& s )
{
	_quited = true;
	*this = s;
	s._actor->_exitCallback.push_back(s._actor->_parentActor->make_trig_notifer(_quiteAth));
}

child_actor_handle& child_actor_handle::operator=( child_actor_handle& )
{
	assert(false);
	return *this;
}

child_actor_handle& child_actor_handle::operator=( child_actor_param& s )
{
	assert(_quited);
	_quited = false;
	_param = s;
	s._actor->_exitCallback.push_back(s._actor->_parentActor->make_trig_notifer(_quiteAth));
	DEBUG_OPERATION(_param._isCopy = true);
	DEBUG_OPERATION(_qh = s._actor->parent_actor()->regist_quit_handler([this]{_quited = true;}));//在父Actor退出时置_quited=true
	return *this;
}

child_actor_handle::~child_actor_handle()
{
	assert(_quited);
}

actor_handle child_actor_handle::get_actor()
{
	return _param._actor;
}

my_actor* child_actor_handle::operator ->()
{
	return _param._actor.get();
}

void child_actor_handle::peel()
{
	if (_param._actor)
	{
		assert(_param._actor->parent_actor()->_strand->running_in_this_thread());
		assert(!_param._actor->parent_actor()->_quited);
		_quited = true;
		DEBUG_OPERATION(_param._actor->parent_actor()->_quitHandlerList.erase(_qh));
		_param._actor->parent_actor()->_childActorList.erase(_param._actorIt);
		_param._actor.reset();
	}
}

child_actor_handle::ptr child_actor_handle::make_ptr()
{
	return ptr(new child_actor_handle);
}

bool child_actor_handle::empty()
{
	return !_param._actor;
}

//////////////////////////////////////////////////////////////////////////

void actor_msg_handle_base::run_one()
{
	assert(!_hostActor->is_quited());
	_hostActor->pull_yield();
}

actor_msg_handle_base::actor_msg_handle_base()
:_waiting(false)
{

}

void actor_msg_handle_base::set_actor(const actor_handle& hostActor)
{
	_hostActor = hostActor.get();
	DEBUG_OPERATION(_strand = hostActor->self_strand());
}

bool actor_msg_handle_base::is_quited()
{
	return _hostActor->is_quited();
}

std::shared_ptr<bool> actor_msg_handle_base::new_bool()
{
	assert(s_sharedBoolPool);
	std::shared_ptr<bool> r = s_sharedBoolPool->new_();
	*r = false;
	return r;
}

//////////////////////////////////////////////////////////////////////////

void MsgPumpBase_::run_one()
{
	assert(!_hostActor->is_quited());
	_hostActor->pull_yield();
}

void MsgPumpBase_::push_yield()
{
	assert(!_hostActor->is_quited());
	_hostActor->push_yield();
}

bool MsgPumpBase_::is_quited()
{
	return !_hostActor || _hostActor->is_quited();
}

//////////////////////////////////////////////////////////////////////////

MsgPoolVoid_::MsgPoolVoid_(const shared_strand& strand)
{
	_strand = strand;
	_waiting = false;
	_sendCount = 0;
	_msgBuff = 0;
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
		auto shared_this = _weakThis.lock();
		_strand->post([=]
		{
			shared_this->send_msg(hostActor);
		});
	}
}

void MsgPoolVoid_::send_msg(const actor_handle& hostActor)
{
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
	my_actor::quit_guard qg(host);
	return host->send<bool>(_thisPool->_strand, [&, refThis_]()->bool
	{
		auto lockThis = refThis_;
		bool ok = false;
		if (_msgPump == _thisPool->_msgPump)
		{
			if (pumpID == _thisPool->_sendCount)
			{
				if (_thisPool->_msgBuff)
				{
					_thisPool->_msgBuff--;
					ok = true;
				}
			}
			else
			{//上次消息没取到，重新取，但实际中间已经post出去了
				assert(!_thisPool->_waiting);
				assert(pumpID + 1 == _thisPool->_sendCount);
				ok = true;
			}
		}
		return ok;
	});
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

void MsgPoolVoid_::pump_handler::operator()(unsigned char pumpID)
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

MsgPumpVoid_::MsgPumpVoid_(const actor_handle& hostActor)
{
	_waiting = false;
	_hasMsg = false;
	_checkDis = false;
	_dstRec = NULL;
	_pumpCount = 0;
	_hostActor = hostActor.get();
	_strand = hostActor->self_strand();
}

MsgPumpVoid_::~MsgPumpVoid_()
{

}

void MsgPumpVoid_::clear()
{
	assert(_strand->running_in_this_thread());
	_pumpHandler.clear();
	if (!is_quited() && _checkDis)
	{
		assert(_waiting);
		_waiting = false;
		_dstRec = NULL;
		run_one();
	}
}

void MsgPumpVoid_::close()
{
	_hasMsg = false;
	_waiting = false;
	_checkDis = false;
	_dstRec = NULL;
	_pumpCount = 0;
	_pumpHandler.clear();
	_hostActor = NULL;
}

void MsgPumpVoid_::connect(const pump_handler& pumpHandler)
{
	assert(_strand->running_in_this_thread());
	assert(_hostActor);
	_pumpHandler = pumpHandler;
	_pumpCount = 0;
	if (_waiting)
	{
		_pumpHandler.post_pump(_pumpCount);
	}
}

void MsgPumpVoid_::receiver()
{
	if (!is_quited())
	{
		_pumpCount++;
		assert(!_hasMsg);
		if (_waiting)
		{
			_waiting = false;
			_checkDis = false;
			if (_dstRec)
			{
				*_dstRec = true;
				_dstRec = NULL;
			}
			run_one();
		}
		else
		{
			_hasMsg = true;
		}
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
		_pumpHandler(_pumpCount);
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
		_pumpHandler(_pumpCount);
		_waiting = !_hasMsg;
		_hasMsg = false;
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
	if (!_pumpHandler.empty())
	{
		bool wait = false;
		if (_pumpHandler.try_pump(_hostActor, _pumpCount, wait))
		{
			if (wait)
			{
				if (!_hasMsg)
				{
					_waiting = true;
					push_yield();
					assert(_hasMsg);
				}
				_hasMsg = false;
			}
			return true;
		}
	}
	return false;
}

void MsgPumpVoid_::stop_waiting()
{
	_waiting = false;
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

void TrigOnceBase_::tick_handler() const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->tick_handler();
	reset();
}

void TrigOnceBase_::dispatch_handler() const
{
	assert(!_pIsTrig->exchange(true));
	assert(_hostActor);
	_hostActor->dispatch_handler();
	reset();
}

void TrigOnceBase_::push_yield() const
{
	_hostActor->push_yield();
}

void TrigOnceBase_::operator =(const TrigOnceBase_&)
{
	assert(false);
}
//////////////////////////////////////////////////////////////////////////

template <typename T>
struct actor_ref_count_alloc
{
	template<class Other>
	struct rebind
	{
		typedef actor_ref_count_alloc<Other> other;
	};

	actor_ref_count_alloc(StackPck_& stackMem, void** ptr)
		:_stackMem(stackMem), _ptr(ptr)
	{
	}

	actor_ref_count_alloc(const actor_ref_count_alloc<T>& s)
		:_stackMem(s._stackMem), _ptr(s._ptr)
	{
	}

	template<class Other>
	actor_ref_count_alloc(const actor_ref_count_alloc<Other>& s)
		: _stackMem(s._stackMem), _ptr(s._ptr)
	{
	}

	T* allocate(size_t count)
	{
		assert(1 == count);
		*_ptr = (unsigned char*)*_ptr - MEM_ALIGN(sizeof(T), sizeof(void*));
		return (T*)*_ptr;
	}

	void deallocate(T* ptr, size_t count)
	{
		assert(1 == count);
		ActorStackPool_::recovery(_stackMem);
	}

	void destroy(T* ptr)
	{
		ptr->~T();
	}

	StackPck_ _stackMem;
	void** _ptr;
};
//////////////////////////////////////////////////////////////////////////

class my_actor::boost_actor_run
{
public:
	boost_actor_run(my_actor& actor)
		: _actor(actor)
	{

	}

	void operator ()( actor_push_type& actorPush )
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
				if (_actor._quited)
				{
					throw force_quit_exception();
				}
				_actor._inActor = true;
			}
			assert(_actor._strand->running_in_this_thread());
			_actor._mainFunc(&_actor);
			_actor._quited = true;
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
#ifdef _DEBUG
		catch (my_actor::pump_disconnected_exception&)
		{
			assert(false);
		}
		catch (async_buffer_close_exception&)
		{
			assert(false);
		}
		catch (sync_csp_exception&)
		{
			assert(false);
		}
		catch (...)
		{
			assert(false);
		}
#endif
		assert(_actor._quited);
		clear_function(_actor._mainFunc);
		clear_function(_actor._timerState._timerCb);
		if (_actor._timer && !_actor._timerState._timerCompleted)
		{
			_actor._timerState._timerCompleted = true;
			_actor._timer->cancel(_actor._timerState._timerHandle);
		}
		_actor._msgPoolStatus.clear(&_actor);
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
	_childOverCount = 0;
	_childSuspendResumeCount = 0;
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
	s_stackLineMutex.lock();
	s_stackLine.erase(_btIt);
	s_stackLine.erase(_topIt);
	s_stackLineMutex.unlock();
#endif
#ifdef CHECK_ACTOR_STACK
	unsigned char* bt = (unsigned char*)_stackTop - _stackSize - STACK_RESERVED_SPACE_SIZE;
	size_t i = 0;
	for (; i < _stackSize + STACK_RESERVED_SPACE_SIZE - CORO_CONTEXT_STATE_SPACE && bt[i] == 0xFD; i++) {}
	if (_checkStackFree || STACK_RESERVED_SPACE_SIZE >= i)
	{
		stack_overflow_format(STACK_RESERVED_SPACE_SIZE - (int)i, _createStack);
	}
#endif
	delete (actor_pull_type*)_actorPull;
}

my_actor& my_actor::operator =(const my_actor&)
{
	return *this;
}

actor_handle my_actor::create(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	assert(stackSize && stackSize <= 1024 kB && 0 == stackSize % (4 kB));
	actor_handle newActor;
	assert(ActorStackPool_::isEnable());
	/*内存结构(L:H):|------Actor Stack------|---shared_ptr_ref_count---|--Actor Obj--|*/
	const size_t actorSize = MEM_ALIGN(sizeof(my_actor), sizeof(void*));
	StackPck_ stackMem = ActorStackPool_::getStack(stackSize + STACK_RESERVED_SPACE_SIZE);
	unsigned char* refTop = (unsigned char*)stackMem._stack.sp - actorSize;
	newActor = actor_handle(new(refTop)my_actor, [](my_actor* p){p->~my_actor(); },
		actor_ref_count_alloc<void>(stackMem, (void**)&refTop));
	newActor->_stackTop = refTop;
	newActor->_stackSize = stackMem._stack.size - ((size_t)stackMem._stack.sp - (size_t)newActor->_stackTop);
	newActor->_strand = actorStrand;
	newActor->_mainFunc = mainFunc;
	newActor->_timer = actorStrand->get_timer();
	newActor->_actorPull = new actor_pull_type(boost_actor_run(*newActor),
		boost::coroutines::attributes(newActor->_stackSize), actor_stack_pool_allocate(newActor->_stackTop, newActor->_stackSize));
	newActor->_stackSize -= STACK_RESERVED_SPACE_SIZE;
	newActor->_weakThis = newActor;
#ifdef CHECK_SELF
	s_stackLineMutex.lock();
	newActor->_btIt = s_stackLine.insert(make_pair((unsigned char*)newActor->_stackTop-newActor->_stackSize, newActor.get())).first;
	newActor->_topIt = s_stackLine.insert(make_pair((unsigned char*)newActor->_stackTop - 1, (my_actor*)NULL)).first;
	s_stackLineMutex.unlock();
#endif

#ifdef CHECK_ACTOR_STACK
	newActor->_checkStackFree = false;
	memset((unsigned char*)newActor->_stackTop - newActor->_stackSize - STACK_RESERVED_SPACE_SIZE, 0xFD, newActor->_stackSize + STACK_RESERVED_SPACE_SIZE - CORO_CONTEXT_STATE_SPACE);
	newActor->_createStack = std::shared_ptr<list<stack_line_info>>(new list<stack_line_info>(get_stack_list(8, 1)));
#endif
	return newActor;
}

child_actor_handle::child_actor_param my_actor::create_child_actor(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize)
{
	assert_enter();
	child_actor_handle::child_actor_param actorHandle;
	actorHandle._actor = my_actor::create(actorStrand, mainFunc, stackSize);
	actorHandle._actor->_parentActor = shared_from_this();
	_childActorList.push_front(actorHandle._actor);
	actorHandle._actorIt = _childActorList.begin();
	DEBUG_OPERATION(actorHandle._isCopy = false);
	return actorHandle;
}

child_actor_handle::child_actor_param my_actor::create_child_actor(const main_func& mainFunc, size_t stackSize)
{
	return create_child_actor(self_strand(), mainFunc, stackSize);
}

void my_actor::child_actor_run( child_actor_handle& actorHandle )
{
	assert_enter();
	assert(!actorHandle._quited);
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
	actorHandle._param._actor->notify_run();
}

void my_actor::child_actor_run(const list<std::shared_ptr<child_actor_handle> >& actorHandles)
{
	assert_enter();
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		child_actor_run(**it);
	}
}

void my_actor::child_actor_force_quit( child_actor_handle& actorHandle )
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());

		my_actor* actor = actorHandle._param._actor.get();
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

void my_actor::child_actors_force_quit(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		const child_actor_handle::ptr& actorHandle = *it;
		if (!actorHandle->_quited)
		{
			assert(actorHandle->get_actor());
			assert(actorHandle->get_actor()->parent_actor()->self_id() == self_id());
			my_actor* actor = actorHandle->_param._actor.get();
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
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		const child_actor_handle::ptr& actorHandle = *it;
		if (!actorHandle->_quited)
		{
			wait_trig(actorHandle->_quiteAth);
			actorHandle->peel();
		}
	}
}

void my_actor::child_actor_wait_quit( child_actor_handle& actorHandle )
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor()->self_id() == self_id());
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

void my_actor::child_actors_wait_quit(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		assert((*it)->get_actor()->parent_actor()->self_id() == self_id());
		child_actor_wait_quit(**it);
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

void my_actor::child_actors_suspend(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer(amh);
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		child_actor_handle::ptr actorHandle = *it;
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

void my_actor::child_actors_resume(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	actor_msg_handle<> amh;
	auto h = make_msg_notifer(amh);
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		child_actor_handle::ptr actorHandle = *it;
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

void my_actor::sleep( int ms )
{
	assert_enter();
	if (ms > 0)
	{
		assert(_timer);
		timeout(ms, [this]{run_one(); });
		push_yield();
	}
	else
	{
		actor_handle shared_this = shared_from_this();
		if (0 == ms)
		{
			_strand->post([shared_this]{shared_this->run_one(); });
		}
		push_yield();
	}
}

void my_actor::sleep_guard(int ms)
{
	assert_enter();
	quit_guard qg(this);
	sleep(ms);
}

void my_actor::yield()
{
	assert_enter();
	actor_handle shared_this = shared_from_this();
	_strand->post([shared_this]{shared_this->run_one(); });
	push_yield();
}

void my_actor::yield_guard()
{
	assert_enter();
	quit_guard qg(this);
	_strand->next_tick([this]{run_one(); });
	push_yield();
}

actor_handle my_actor::parent_actor()
{
	return _parentActor;
}

const msg_list_shared_alloc<actor_handle>& my_actor::child_actors()
{
	return _childActorList;
}

my_actor::quit_iterator my_actor::regist_quit_handler( const std::function<void ()>& quitHandler )
{
	assert_enter();
	_quitHandlerList.push_front(quitHandler);//后注册的先执行
	return _quitHandlerList.begin();
}

void my_actor::cancel_quit_handler( quit_iterator qh )
{
	assert_enter();
	_quitHandlerList.erase(qh);
}

void my_actor::cancel_delay_trig()
{
	assert_enter();
	cancel_timer();
}

void my_actor::post_handler()
{
	actor_handle shared_this = shared_from_this();
	_strand->post([shared_this]{shared_this->run_one(); });
}

void my_actor::dispatch_handler()
{
	actor_handle shared_this = shared_from_this();
	_strand->dispatch([shared_this]{shared_this->run_one(); });
}

void my_actor::tick_handler()
{
	actor_handle shared_this = shared_from_this();
	_strand->try_tick([shared_this]{shared_this->run_one(); });
}

void my_actor::next_tick_handler()
{
	actor_handle shared_this = shared_from_this();
	_strand->next_tick([shared_this]{shared_this->run_one(); });
}

shared_strand my_actor::self_strand()
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
	assert((size_t)get_sp() >= (size_t)_stackTop-_stackSize+1024);
	check_self();
}

void my_actor::notify_quit()
{
	actor_handle shared_this = shared_from_this();
	_strand->try_tick([shared_this]{shared_this->force_quit(std::function<void()>()); });
}

void my_actor::notify_quit(const std::function<void ()>& h)
{
	actor_handle shared_this = shared_from_this();
	_strand->try_tick([=]{shared_this->force_quit(h); });
}

void my_actor::force_quit( const std::function<void ()>& h )
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);
	if (!_quited)
	{
		if (!_lockQuit)
		{
			_quited = true;
			_isForce = true;
			if (h) _exitCallback.push_back(h);
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
			_exitCallback.push_back(h);
		}
		_notifyQuited = true;
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
			_exitCallback.push_back(h);
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

void my_actor::notify_suspend()
{
	notify_suspend(std::function<void ()>());
}

void my_actor::notify_suspend(const std::function<void ()>& h)
{
	actor_handle shared_this = shared_from_this();
	_strand->try_tick([=]{shared_this->suspend(h); });
}

void my_actor::suspend(const std::function<void ()>& h)
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);

	_suspendResumeQueue.push_back(suspend_resume_option());
	suspend_resume_option& tmp = _suspendResumeQueue.back();
	tmp._isSuspend = true;
	tmp._h = h;
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
				for (auto it = _childActorList.begin(); it != _childActorList.end(); it++)
				{
					actor_handle shared_this = shared_from_this();
					if ((*it)->_strand == _strand)
					{
						(*it)->suspend([shared_this]{shared_this->child_suspend_cb_handler(); });
					}
					else
					{
						(*it)->notify_suspend(_strand->wrap_post([shared_this]{shared_this->child_suspend_cb_handler(); }));
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
	notify_resume(std::function<void ()>());
}

void my_actor::notify_resume(const std::function<void ()>& h)
{
	actor_handle shared_this = shared_from_this();
	_strand->try_tick([=]{shared_this->resume(h); });
}

void my_actor::resume(const std::function<void ()>& h)
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);

	_suspendResumeQueue.push_back(suspend_resume_option());
	suspend_resume_option& tmp = _suspendResumeQueue.back();
	tmp._isSuspend = false;
	tmp._h = h;
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
				for (auto it = _childActorList.begin(); it != _childActorList.end(); it++)
				{
					actor_handle shared_this = shared_from_this();
					if ((*it)->_strand == _strand)
					{
						(*it)->resume(_strand->wrap_post([shared_this]{shared_this->child_resume_cb_handler(); }));
					} 
					else
					{
						(*it)->notify_resume(_strand->wrap_post([shared_this]{shared_this->child_resume_cb_handler(); }));
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
	switch_pause_play(std::function<void (bool isPaused)>());
}

void my_actor::switch_pause_play(const std::function<void (bool)>& h)
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
	boost::mutex mutex;
	boost::condition_variable conVar;
	boost::unique_lock<boost::mutex> ul(mutex);
	LAMBDA_THIS_REF2(ref2, conVar, mutex);
	_strand->post([&ref2]
	{
		assert(ref2->_strand->running_in_this_thread());
		if (ref2->_quited && !ref2->_childOverCount)
		{
			boost::lock_guard<boost::mutex> lg(ref2.mutex);
			ref2.conVar.notify_one();
		}
		else
		{
			auto& ref2_ = ref2;
			ref2->_exitCallback.push_back([&ref2_]()
			{
				boost::lock_guard<boost::mutex> lg(ref2_.mutex);
				ref2_.conVar.notify_one();
			});
		}
	});
	conVar.wait(ul);
}

void my_actor::append_quit_callback(const std::function<void ()>& h)
{
	if (_strand->running_in_this_thread())
	{
		if (_quited && !_childOverCount)
		{
			DEBUG_OPERATION(size_t yc = yield_count());
			CHECK_EXCEPTION(h);
			assert(yield_count() == yc);
		} 
		else
		{
			_exitCallback.push_back(h);
		}
	} 
	else
	{
		actor_handle shared_this = shared_from_this();
		_strand->post([=]{shared_this->append_quit_callback(h); });
	}
}

void my_actor::actors_start_run(const list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->notify_run();
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
	auto h = make_msg_notifer(amh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->notify_quit(h);
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
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		actor_wait_quit(*it);
	}
}

bool my_actor::timed_actor_wait_quit(int tm, const actor_handle& anotherActor)
{
	assert_enter();
	assert(anotherActor);
	actor_trig_handle<> ath;
	anotherActor->append_quit_callback(make_trig_notifer(ath));
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
	auto h = make_msg_notifer(amh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->notify_suspend(h);
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
	auto h = make_msg_notifer(amh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->notify_resume(h);
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
	auto h = make_msg_notifer(amh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->switch_pause_play(h);
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

void my_actor::pull_yield()
{
	assert(!_quited);
	if (!_suspended)
	{
		(*(actor_pull_type*)_actorPull)();
	}
	else
	{
		assert(!_hasNotify);
		_hasNotify = true;
	}
}

void my_actor::pull_yield_as_mutex()
{
	(*(actor_pull_type*)_actorPull)();
}

void my_actor::push_yield()
{
	assert(!_quited);
	assert(_inActor);
	check_stack();
	_yieldCount++;
	_inActor = false;
	(*(actor_push_type*)_actorPush)();
	if (!_quited)
	{
		_inActor = true;
		return;
	}
	throw force_quit_exception();
}

void my_actor::push_yield_as_mutex()
{
	check_stack();
	(*(actor_push_type*)_actorPush)();
}

void my_actor::force_quit_cb_handler()
{
	assert(_quited);
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
	assert(_quited);
	assert(!_childOverCount);
	DEBUG_OPERATION(size_t yc = yield_count());
	while (!_quitHandlerList.empty())
	{
		CHECK_EXCEPTION(_quitHandlerList.front());
		_quitHandlerList.pop_front();
	}
	assert(yield_count() == yc);
	(*(actor_pull_type*)_actorPull)();
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
		_timer->cancel(_timerState._timerHandle);
		clear_function(_timerState._timerCb);
	}
}

void my_actor::suspend_timer()
{
	if (!_timerState._timerSuspend)
	{
		_timerState._timerSuspend = true;
		if (!_timerState._timerCompleted)
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
		if (!_timerState._timerCompleted)
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
	if ((size_t)get_sp() < (size_t)_stackTop-_stackSize)
	{
		stack_overflow_format((int)((size_t)get_sp() - (size_t)_stackTop - _stackSize), _createStack);
	}
#endif
	check_self();
}

my_actor* my_actor::self_actor()
{
#ifdef CHECK_SELF
	boost::lock_guard<boost::mutex> lg(s_stackLineMutex);
	auto eit = s_stackLine.insert(make_pair(get_sp(), (my_actor*)NULL));
	if (eit.second)
	{
		s_stackLine.erase(eit.first--);
		assert(eit.first != s_stackLine.end());
		return eit.first->second;
	}
#endif
	return NULL;
}

void my_actor::check_self()
{
#ifdef CHECK_SELF
	assert(this == self_actor());
#endif
}

size_t my_actor::stack_free_space()
{
	int s = (int)((size_t)get_sp() - ((size_t)_stackTop-_stackSize));
	if (s < 0)
	{
		return 0;
	}
	return (size_t)s;
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
		if (0 != tm)
		{
			bool timed = false;
			if (tm > 0)
			{
				delay_trig(tm, [this, &timed]
				{
					if (!_quited)
					{
						timed = true;
						pull_yield();
					}
				});
			}
			push_yield();
			if (!timed)
			{
				if (tm > 0)
				{
					cancel_delay_trig();
				}
				return true;
			}
		}
		amh.stop_waiting();
		return false;
	}
	return true;
}

bool my_actor::try_wait_msg(actor_msg_handle<>& amh)
{
	return timed_wait_msg(0, amh);
}

bool my_actor::timed_pump_msg(int tm, const msg_pump_handle<>& pump, bool checkDis)
{
	assert_enter();
	assert(pump->_hostActor && pump->_hostActor->self_id() == self_id());
	if (!pump->read_msg())
	{
		if (checkDis && pump->isDisconnected())
		{
			pump->stop_waiting();
			throw pump_disconnected_exception();
		}
		if (0 != tm)
		{
			pump->_checkDis = checkDis;
			bool timed = false;
			if (tm > 0)
			{
				delay_trig(tm, [this, &timed]
				{
					if (!_quited)
					{
						timed = true;
						pull_yield();
					}
				});
			}
			push_yield();
			if (!timed)
			{
				if (tm > 0)
				{
					cancel_delay_trig();
				}
				if (pump->_checkDis)
				{
					assert(checkDis);
					pump->_checkDis = false;
					throw pump_disconnected_exception();
				}
				return true;
			}
		}
		pump->_checkDis = false;
		pump->stop_waiting();
		return false;
	}
	return true;
}

bool my_actor::try_pump_msg(const msg_pump_handle<>& pump, bool checkDis)
{
	assert_enter();
	assert(pump->_hostActor && pump->_hostActor->self_id() == self_id());
	if (!pump->try_read())
	{
		if (checkDis && pump->isDisconnected())
		{
			pump->_waiting = false;
			throw pump_disconnected_exception();
		}
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
		if (0 != tm)
		{
			bool timed = false;
			if (tm > 0)
			{
				delay_trig(tm, [this, &timed]
				{
					if (!_quited)
					{
						timed = true;
						pull_yield();
					}
				});
			}
			push_yield();
			if (!timed)
			{
				if (tm > 0)
				{
					cancel_delay_trig();
				}
				return true;
			}
		}
		ath.stop_waiting();
		return false;
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

void my_actor::pump_msg(const msg_pump_handle<>& pump, bool checkDis /*= false*/)
{
	timed_pump_msg(-1, pump, checkDis);
}