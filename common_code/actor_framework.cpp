#define WIN32_LEAN_AND_MEAN

#include <boost/coroutine/all.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include "actor_framework.h"
#include "actor_stack.h"
#include "asm_ex.h"

typedef boost::coroutines::coroutine<void>::pull_type actor_pull_type;
typedef boost::coroutines::coroutine<void>::push_type actor_push_type;
typedef boost::asio::basic_waitable_timer<boost::chrono::high_resolution_clock> timer_type;

#ifdef _DEBUG

#define CHECK_EXCEPTION(__h) try { (__h)(); } catch (...) { assert(false); }
#define CHECK_EXCEPTION1(__h, __p0) try { (__h)(__p0); } catch (...) { assert(false); }
#define CHECK_EXCEPTION2(__h, __p0, __p1) try { (__h)(__p0, __p1); } catch (...) { assert(false); }
#define CHECK_EXCEPTION3(__h, __p0, __p1, __p2) try { (__h)(__p0, __p1, __p2); } catch (...) { assert(false); }

#else

#define CHECK_EXCEPTION(__h) (__h)()
#define CHECK_EXCEPTION1(__h, __p0) (__h)(__p0)
#define CHECK_EXCEPTION2(__h, __p0, __p1) (__h)(__p0, __p1)
#define CHECK_EXCEPTION3(__h, __p0, __p1, __p2) (__h)(__p0, __p1, __p2)

#endif //end _DEBUG

//堆栈底预留空间，防止爆栈
#define STACK_RESERVED_SPACE_SIZE		64
//内存边界对齐
#define MEM_ALIGN(__o, __a) (((__o) + ((__a)-1)) & (((__a)-1) ^ -1))

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

struct actor_free 
{
	actor_free() {}

	actor_free(stack_pck& stack)
	{
		_stack = stack;
	}

	void operator ()(my_actor* actor)
	{
		actor->~my_actor();
		actor_stack_pool::recovery(_stack);
	}

	stack_pck _stack;
};
//////////////////////////////////////////////////////////////////////////

bool param_list_base::closed()
{
	return (*_pIsClosed);
}

bool param_list_base::empty()
{
	return 0 == length();
}

void param_list_base::begin(long long actorID)
{
	close();
	_timeout = false;
	_hasTm = false;
	DEBUG_OPERATION(_actorID = actorID);
	_pIsClosed = boost::shared_ptr<bool>(new bool(false));
}

param_list_base& param_list_base::operator=( const param_list_base& )
{
	assert(false);
	return *this;
}

param_list_base::param_list_base( const param_list_base& )
{
	assert(false);
}

param_list_base::param_list_base()
{
	_waiting = false;
	_timeout = false;
	_hasTm = false;
	DEBUG_OPERATION(_actorID = 0);
}

param_list_base::~param_list_base()
{
	if (_pIsClosed)
	{
		(*_pIsClosed) = true;
	}
}
//////////////////////////////////////////////////////////////////////////

void null_param_list::close()
{
	count = 0;
	if (_pIsClosed)
	{
		assert(!(*_pIsClosed));
		(*_pIsClosed) = true;
		_waiting = false;
		_pIsClosed.reset();
	}
}

null_param_list::null_param_list()
{
	count = 0;
}

void null_param_list::clear()
{
	count = 0;
}

size_t null_param_list::length()
{
	return count;
}
//////////////////////////////////////////////////////////////////////////

async_trig_base& async_trig_base::operator=( const async_trig_base& )
{
	assert(false);
	return *this;
}

async_trig_base::async_trig_base( const async_trig_base& )
{
	assert(false);
}

async_trig_base::async_trig_base()
{
	_waiting = false;
	_notify = true;
	_timeout = false;
	_hasTm = false;
	_dstRefPt = NULL;
	DEBUG_OPERATION(_actorID = 0);
}

async_trig_base::~async_trig_base()
{
	close();
}

bool async_trig_base::has_trig()
{
	return _notify;
}

void async_trig_base::begin(long long actorID)
{
	close();
	_pIsClosed = boost::shared_ptr<bool>(new bool(false));
	_notify = false;
	_timeout = false;
	_hasTm = false;
	_dstRefPt = NULL;
	DEBUG_OPERATION(_actorID = actorID);
}

void async_trig_base::close()
{
	if (_pIsClosed)
	{
		assert(!(*_pIsClosed));
		(*_pIsClosed) = true;
		_waiting = false;
		_pIsClosed.reset();
	}
}
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
	_norQuit = false;
}

child_actor_handle::child_actor_handle( child_actor_param& s )
{
	_quited = true;
	_norQuit = false;
	*this = s;
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
	_norQuit = false;
	_param = s;
	DEBUG_OPERATION(_param._isCopy = true);
	DEBUG_OPERATION(_qh = s._actor->parent_actor()->regist_quit_handler([this](){_quited = true;}));//在父Actor退出时置_quited=true
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

actor_handle child_actor_handle::peel()
{
	actor_handle r = _param._actor;
	if (_param._actor)
	{
		assert(_param._actor->parent_actor()->_strand->running_in_this_thread());
		assert(!_param._actor->parent_actor()->_quited);
		_quited = true;
		DEBUG_OPERATION(_param._actor->parent_actor()->_quitHandlerList.erase(_qh));
		_param._actor->parent_actor()->_childActorList.erase(_param._actorIt);
		_param._actor->_parentActor.reset();
		_param._actor.reset();
	}
	return r;
}

child_actor_handle::ptr child_actor_handle::make_ptr()
{
	return ptr(new child_actor_handle);
}

void* child_actor_handle::operator new(size_t s)
{
	void* p = malloc(s);
	assert(p);
	return p;
}

void child_actor_handle::operator delete(void* p)
{
	if (p)
	{
		free(p);
	}
}

//////////////////////////////////////////////////////////////////////////

struct my_actor::timer_pck
{
	timer_pck(boost::asio::io_service& ios)
		:_timer(ios), _timerTime(0), _timerSuspend(false), _timerCompleted(true), _timerCount(0) {}

	timer_type _timer;
	bool _timerSuspend;
	bool _timerCompleted;
	size_t _timerCount;
	boost::posix_time::microsec _timerTime;
	boost::posix_time::ptime _timerStampBegin;
	boost::posix_time::ptime _timerStampEnd;
	boost::function<void ()> _h;
};

boost::atomic<long long> _actorIDCount(0);//ID计数
bool _autoMakeTimer = true;

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
			DEBUG_OPERATION(_actor._inActor = true);
			_actor.push_yield();
			assert(_actor._strand->running_in_this_thread());
			_actor._mainFunc(&_actor);
			DEBUG_OPERATION(_actor._inActor = false);
			_actor._quited = true;
			_actor._mainFunc.clear();
			assert(_actor._childActorList.empty());
			assert(_actor._quitHandlerList.empty());
			assert(_actor._suspendResumeQueue.empty());
		}
		catch (my_actor::force_quit_exception&)
		{//捕获Actor被强制退出异常
			assert(!_actor._inActor);
			_actor._mainFunc.clear();
		}
#ifdef _DEBUG
		catch (...)
		{
			assert(false);
		}
#endif
		while (!_actor._exitCallback.empty())
		{
			assert(_actor._exitCallback.front());
			CHECK_EXCEPTION1(_actor._exitCallback.front(), !_actor._isForce);
			_actor._exitCallback.pop_front();
		}
		if (_actor._timerSleep)
		{
			_actor.cancel_timer();
			if (actor_stack_pool::isEnable())
			{
				_actor._timerSleep->~timer_pck();
			}
			else
			{
				delete _actor._timerSleep;
			}
			_actor._timerSleep = NULL;
		}
	}
private:
	my_actor& _actor;
};

my_actor::my_actor()
{
	_actorPull = NULL;
	_actorPush = NULL;
	_timerSleep = NULL;
	_quited = false;
	_started = false;
	DEBUG_OPERATION(_inActor = false);
	_suspended = false;
	_hasNotify = false;
	_isForce = false;
	_stackTop = NULL;
	_stackSize = 0;
	_yieldCount = 0;
	_childOverCount = 0;
	_childSuspendResumeCount = 0;
	_actorID = ++_actorIDCount;
}

my_actor::my_actor(const my_actor&)
{

}

my_actor::~my_actor()
{
	assert(_quited);
	assert(!_mainFunc);
	assert(!_timerSleep);
	assert(!_childOverCount);
	assert(!_childSuspendResumeCount);
	assert(_suspendResumeQueue.empty());
	assert(_quitHandlerList.empty());
	assert(_suspendResumeQueue.empty());
	assert(_exitCallback.empty());
	assert(_childActorList.empty());
#if (CHECK_ACTOR_STACK) || (_DEBUG)
	if (*(long long*)((BYTE*)_stackTop-_stackSize+STACK_RESERVED_SPACE_SIZE-sizeof(long long)) != 0xFEFEFEFEFEFEFEFE)
	{
		assert(false);
		char buf[48];
		sprintf_s(buf, "%llu Actor堆栈溢出", _actorID);
		throw boost::shared_ptr<string>(new string(buf));
	}
#endif
	delete (actor_pull_type*)_actorPull;
}

my_actor& my_actor::operator =(const my_actor&)
{
	return *this;
}

actor_handle my_actor::create( shared_strand actorStrand, const main_func& mainFunc, size_t stackSize )
{
	return create(actorStrand, mainFunc, boost::function<void (bool)>(), stackSize);
}

actor_handle my_actor::create( shared_strand actorStrand, const main_func& mainFunc,
	const boost::function<void (bool)>& cb, size_t stackSize )
{
	assert(stackSize && stackSize <= 1024 kB && 0 == stackSize % (4 kB));
	actor_handle newActor;
	if (actor_stack_pool::isEnable())
	{
		const size_t actorSize = MEM_ALIGN(sizeof(my_actor), sizeof(void*));
		const size_t timerSize = MEM_ALIGN(sizeof(timer_pck), sizeof(void*));
		assert(actorSize+timerSize < stackSize - 2 kB);
		stack_pck stackMem = actor_stack_pool::getStack(stackSize);
		const size_t totalSize = stackMem._stack.size;
		BYTE* stackTop = (BYTE*)stackMem._stack.sp;
		newActor = actor_handle(new(stackTop-actorSize) my_actor, actor_free(stackMem));
		if (_autoMakeTimer)
		{
			newActor->_timerSleep = new(stackTop-actorSize-timerSize) timer_pck(actorStrand->get_io_service());
		}
		newActor->_strand = actorStrand;
		newActor->_mainFunc = mainFunc;
		newActor->_stackTop = stackTop-actorSize-timerSize;
		newActor->_stackSize = totalSize-actorSize-timerSize;
		if (cb) newActor->_exitCallback.push_back(cb);
		newActor->_actorPull = new actor_pull_type(boost_actor_run(*newActor),
			boost::coroutines::attributes(newActor->_stackSize), actor_stack_pool_allocate(newActor->_stackTop, newActor->_stackSize));
	} 
	else
	{
		newActor = actor_handle(new my_actor);
		if (_autoMakeTimer)
		{
			newActor->_timerSleep = new timer_pck(actorStrand->get_io_service());
		}
		newActor->_strand = actorStrand;
		newActor->_mainFunc = mainFunc;
		if (cb) newActor->_exitCallback.push_back(cb);
		newActor->_actorPull = new actor_pull_type(boost_actor_run(*newActor),
			boost::coroutines::attributes(stackSize), actor_stack_allocate(&newActor->_stackTop, &newActor->_stackSize));
	}
	newActor->_weakThis = newActor;
#if (CHECK_ACTOR_STACK) || (_DEBUG)
	*(long long*)((BYTE*)newActor->_stackTop-newActor->_stackSize+STACK_RESERVED_SPACE_SIZE-sizeof(long long)) = 0xFEFEFEFEFEFEFEFE;
#endif
	return newActor;
}

void my_actor::async_create( shared_strand actorStrand, const main_func& mainFunc,
	const boost::function<void (actor_handle)>& ch, size_t stackSize )
{
	boost_strand::asyncInvoke<actor_handle>(actorStrand, [actorStrand, mainFunc, stackSize]()->actor_handle
	{
		return my_actor::create(actorStrand, mainFunc, stackSize);
	}, ch);
}

void my_actor::async_create( shared_strand actorStrand, const main_func& mainFunc,
	const boost::function<void (actor_handle)>& ch, const boost::function<void (bool)>& cb, size_t stackSize )
{
	boost_strand::asyncInvoke<actor_handle>(actorStrand, [actorStrand, mainFunc, cb, stackSize]()->actor_handle
	{
		return my_actor::create(actorStrand, mainFunc, cb, stackSize);
	}, ch);
}

child_actor_handle::child_actor_param my_actor::create_child_actor( shared_strand actorStrand, const main_func& mainFunc, size_t stackSize )
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
	return create_child_actor(this_strand(), mainFunc, stackSize);
}

void my_actor::child_actor_run( child_actor_handle& actorHandle )
{
	assert_enter();
	assert(!actorHandle._quited);
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor().get() == this);
	actorHandle._param._actor->notify_run();
}

void my_actor::child_actor_run(const list<boost::shared_ptr<child_actor_handle> >& actorHandles)
{
	assert_enter();
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		child_actor_run(**it);
	}
}

bool my_actor::child_actor_force_quit( child_actor_handle& actorHandle )
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor().get() == this);

		actor_handle actor = actorHandle.peel();
		if (actor->this_strand() == _strand)
		{
			actorHandle._norQuit = trig<bool>([actor](const boost::function<void(bool)>& h){actor->force_quit(h); });
		} 
		else
		{
			actorHandle._norQuit = trig<bool>([actor](const boost::function<void(bool)>& h){actor->notify_quit(h); });
		}
	}
	return actorHandle._norQuit;
}

bool my_actor::child_actors_force_quit(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	bool res = true;
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		res &= child_actor_force_quit(**it);
	}
	return res;
}

bool my_actor::child_actor_wait_quit( child_actor_handle& actorHandle )
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor().get() == this);
		actorHandle._norQuit = actor_wait_quit(actorHandle.peel());
	}
	return actorHandle._norQuit;
}

bool my_actor::child_actors_wait_quit(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	bool res = true;
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		assert((*it)->get_actor()->parent_actor().get() == this);
		res &= child_actor_wait_quit(**it);
	}
	return res;
}

void my_actor::child_actor_suspend(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor().get() == this);
	actor_handle actor = actorHandle.get_actor();
	if (actorHandle.get_actor()->this_strand() == _strand)
	{
		trig([actor](const boost::function<void()>& h){actor->suspend(h); });
	} 
	else
	{
		trig([actor](const boost::function<void()>& h){actor->notify_suspend(h); });
	}
}

void my_actor::child_actors_suspend(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		child_actor_suspend(**it);
	}
}

void my_actor::child_actor_resume(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor().get() == this);
	actor_handle actor = actorHandle.get_actor();
	if (actorHandle.get_actor()->this_strand() == _strand)
	{
		trig([actor](const boost::function<void()>& h){actor->resume(h); });
	}
	else
	{
		trig([actor](const boost::function<void()>& h){actor->notify_resume(h); });
	}
}

void my_actor::child_actors_resume(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		child_actor_resume(**it);
	}
}

actor_handle my_actor::child_actor_peel(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(!actorHandle._quited);
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor().get() == this);
	return actorHandle.peel();
}

bool my_actor::run_child_actor_complete( shared_strand actorStrand, const main_func& h, size_t stackSize )
{
	assert_enter();
	child_actor_handle actorHandle = create_child_actor(actorStrand, h, stackSize);
	child_actor_run(actorHandle);
	return child_actor_wait_quit(actorHandle);
}

bool my_actor::run_child_actor_complete(const main_func& h, size_t stackSize)
{
	return run_child_actor_complete(this_strand(), h, stackSize);
}

void my_actor::open_timer()
{
	assert_enter();
	if (!_timerSleep)
	{
		if (actor_stack_pool::isEnable())
		{
			size_t timerSize = (sizeof(timer_pck)+(sizeof(void*)-1)) & ((sizeof(void*)-1) ^ -1);
			_timerSleep = new((BYTE*)this-timerSize) timer_pck(_strand->get_io_service());
		}
		else
		{
			_timerSleep = new timer_pck(_strand->get_io_service());
		}
	}
}

void my_actor::sleep( int ms )
{
	assert_enter();
	assert(ms >= 0);
	actor_handle shared_this = shared_from_this();
	if (ms)
	{
		assert(_timerSleep);
		time_out(ms, [shared_this](){shared_this->run_one(); });
	}
	else
	{
		_strand->post([shared_this](){shared_this->run_one(); });
	}
	push_yield();
}

actor_handle my_actor::parent_actor()
{
	return _parentActor.lock();
}

const list<actor_handle>& my_actor::child_actors()
{
	return _childActorList;
}

my_actor::quit_iterator my_actor::regist_quit_handler( const boost::function<void ()>& quitHandler )
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

boost::function<void ()> my_actor::begin_trig(async_trig_handle<>& th)
{
	assert_enter();
	th.begin(_actorID);
	boost::shared_ptr<bool>& pIsClosed = th._pIsClosed;
	actor_handle shared_this = shared_from_this();
	return [=, &th]()
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th._notify)
			{
				shared_this->async_trig_post_yield(th, NULL);
			}
		}
		else
		{
			auto& pIsClosed_ = pIsClosed;
			auto& shared_this_ = shared_this;
			auto& th_ = th;
			_strand->post((boost::function<void(void)>)[=, &th_]()
			{
				shared_this_->_async_trig_handler(pIsClosed_, th_);
			});
		}
	};
}

boost::function<void ()> my_actor::begin_trig(boost::shared_ptr<async_trig_handle<> >& th)
{
	assert_enter();
	th->begin(_actorID);
	boost::shared_ptr<bool>& pIsClosed = th->_pIsClosed;
	actor_handle shared_this = shared_from_this();
	return [=]()
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th->_notify)
			{
				shared_this->async_trig_post_yield(*th, NULL);
			}
		}
		else
		{
			auto& pIsClosed_ = pIsClosed;
			auto& shared_this_ = shared_this;
			auto& th_ = th;
			_strand->post((boost::function<void(void)>)[=]()
			{
				shared_this_->_async_trig_handler(pIsClosed_, *th_);
			});
		}
	};
}

bool my_actor::timed_wait_trig(async_trig_handle<>& th, int tm)
{
	assert(th._actorID == _actorID);
	assert_enter();
	if (async_trig_push(th, tm, NULL))
	{
		close_trig(th);
		return true;
	}
	return false;
}

void my_actor::wait_trig(async_trig_handle<>& th)
{
	timed_wait_trig(th, -1);
}

void my_actor::close_trig(async_trig_base& th)
{
	DEBUG_OPERATION(if (th._actorID) {assert(th._actorID == _actorID); th._actorID = 0;})
	th.close();
}

void my_actor::delay_trig(int ms, const boost::function<void ()>& h)
{
	assert_enter();
	time_out(ms, h);
}

void my_actor::delay_trig(int ms, async_trig_handle<>& th)
{
	assert_enter();
	assert(th._pIsClosed);
	actor_handle shared_this = shared_from_this();
	auto& pIsClosed = th._pIsClosed;
	time_out(ms, [=, &th](){shared_this->_async_trig_handler(pIsClosed, th); });
}

void my_actor::delay_trig(int ms, boost::shared_ptr<async_trig_handle<> >& th)
{
	assert_enter();
	assert(th->_pIsClosed);
	actor_handle shared_this = shared_from_this();
	auto& pIsClosed = th->_pIsClosed;
	time_out(ms, [=](){shared_this->_async_trig_handler(pIsClosed, *th); });
}

void my_actor::cancel_delay_trig()
{
	assert_enter();
	cancel_timer();
}

void my_actor::trig( const boost::function<void (boost::function<void ()>)>& h )
{
	assert_enter();
	actor_handle shared_this = shared_from_this();
#ifdef _DEBUG
	h(wrapped_trig_handler<boost::function<void()> >([shared_this](){shared_this->trig_handler(); }));
#else
	h([shared_this](){shared_this->trig_handler(); });
#endif
	push_yield();
}

void my_actor::send( shared_strand exeStrand, const boost::function<void ()>& h )
{
	assert_enter();
	if (exeStrand != _strand)
	{
		trig([=](const boost::function<void()>& cb){boost_strand::asyncInvokeVoid(exeStrand, h, cb); });
	} 
	else
	{
		h();
	}
}

boost::function<void()> my_actor::make_msg_notify(actor_msg_handle<>& amh)
{
	amh.begin(_actorID);
	actor_handle shared_this = shared_from_this();
	return _strand->wrap_post([shared_this, &amh](){shared_this->check_run1(amh._pIsClosed, amh); });
}

boost::function<void()> my_actor::make_msg_notify(boost::shared_ptr<actor_msg_handle<> >& amh)
{
	amh->begin(_actorID);
	actor_handle shared_this = shared_from_this();
	return _strand->wrap_post([shared_this, amh](){shared_this->check_run1(amh->_pIsClosed, *amh); });
}

void my_actor::close_msg_notify( param_list_base& amh )
{
	DEBUG_OPERATION(if (amh._actorID) {assert(amh._actorID == _actorID); amh._actorID = 0;})
	amh.close();
}

bool my_actor::timed_pump_msg(actor_msg_handle<>& amh, int tm)
{
	assert(amh._actorID == _actorID);
	assert_enter();
	assert(amh._pIsClosed);
	if (amh.count)
	{
		amh.count--;
		return true;
	}
	amh._waiting = true;
	return pump_msg_push(amh, tm);
}

void my_actor::pump_msg(actor_msg_handle<>& amh)
{
	timed_pump_msg(amh, -1);
}

bool my_actor::pump_msg_push(param_list_base& pm, int tm)
{
	if (tm >= 0)
	{
		pm._hasTm = true;
		actor_handle shared_this = shared_from_this();
		delay_trig(tm, [this, shared_this, &pm]()
		{
			if (!_quited)
			{
				assert(pm._waiting);
				pm._waiting = false;
				pm._timeout = true;
				pull_yield();
			}
		});
	}
	push_yield();
	assert(!_quited);
	if (pm._timeout)
	{
		pm._timeout = false;
		pm._hasTm = false;
		return false;
	} 
	return true;
}

void my_actor::trig_handler()
{
	actor_handle shared_this = shared_from_this();
	_strand->post([shared_this](){shared_this->run_one(); });
}

bool my_actor::async_trig_push(async_trig_base& th, int tm, void* dst)
{
	assert(th._pIsClosed);
	if (!th._notify)
	{
		th._waiting = true;
		th._dstRefPt = dst;
		if (tm >= 0)
		{
			th._hasTm = true;
			actor_handle shared_this = shared_from_this();
			delay_trig(tm, [this, shared_this, &th]()
			{
				if (!_quited)
				{
					assert(th._waiting);
					th._waiting = false;
					th._timeout = true;
					pull_yield();
				}
			});
		}
		push_yield();
		if (th._timeout)
		{
			th._timeout = false;
			th._hasTm = false;
			return false;
		}
	}
	else
	{
		th.move_out(dst);
	}
	return true;
}

void my_actor::async_trig_post_yield(async_trig_base& th, void* src)
{
	th._notify = true;
	if (th._waiting)
	{
		th._waiting = false;
		if (th._hasTm)
		{
			th._hasTm = false;
			cancel_timer();
		}
		th.save_to_dst(src);
		actor_handle shared_this = shared_from_this();
		_strand->post([shared_this](){shared_this->run_one(); });
	} 
	else
	{
		th.save_to_temp(src);
	}
}

void my_actor::async_trig_pull_yield(async_trig_base& th, void* src)
{
	th._notify = true;
	if (th._waiting)
	{
		th._waiting = false;
		if (th._hasTm)
		{
			th._hasTm = false;
			cancel_timer();
		}
		th.move_to_dst(src);
		pull_yield();
	} 
	else
	{
		th.move_to_temp(src);
	}
}

void my_actor::_async_trig_handler(const boost::shared_ptr<bool>& pIsClosed, async_trig_handle<>& th)
{
	assert(_strand->running_in_this_thread());
	if (!_quited && !(*pIsClosed) && !th._notify)
	{
		async_trig_pull_yield(th, NULL);
	}
}

void my_actor::check_run1( boost::shared_ptr<bool>& pIsClosed, actor_msg_handle<>& amh )
{
	if (_quited || (*pIsClosed)) return;

	if (amh._waiting)
	{
		amh._waiting = false;
		if (amh._hasTm)
		{
			amh._hasTm = false;
			cancel_timer();
		}
		assert(0 == amh.count);
		pull_yield();
	} 
	else
	{
		amh.count++;
	}
}

shared_strand my_actor::this_strand()
{
	return _strand;
}

actor_handle my_actor::shared_from_this()
{
	return _weakThis.lock();
}

long long my_actor::this_id()
{
	return _actorID;
}

size_t my_actor::yield_count()
{
	assert_enter();
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
	_strand->post([shared_this](){shared_this->start_run(); });
}

void my_actor::assert_enter()
{
	assert(_strand->running_in_this_thread());
	assert(!_quited);
	assert(_inActor);
	assert((size_t)get_sp() >= (size_t)_stackTop-_stackSize+1024);
}

void my_actor::start_run()
{
	if (_strand->running_in_this_thread())
	{
		assert(!_inActor);
		assert(!_started);
		if (!_quited && !_started)
		{
			_started = true;
			pull_yield();
		}
	} 
	else
	{
		notify_run();
	}
}

void my_actor::notify_quit()
{
	actor_handle shared_this = shared_from_this();
	_strand->post([shared_this](){shared_this->force_quit(boost::function<void(bool)>()); });
}

void my_actor::notify_quit(const boost::function<void (bool)>& h)
{
	actor_handle shared_this = shared_from_this();
	_strand->post([=](){shared_this->force_quit(h); });
}

void my_actor::force_quit( const boost::function<void (bool)>& h )
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);
	if (!_quited)
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
					cc->force_quit([shared_this](bool){shared_this->force_quit_cb_handler(); });
				} 
				else
				{
					cc->notify_quit(_strand->wrap_post((boost::function<void(bool)>)[shared_this](bool){shared_this->force_quit_cb_handler(); }));
				}
			}
		} 
		else
		{
			exit_callback();
		}
	}
	else if (h)
	{
		if (!_childOverCount)
		{
			CHECK_EXCEPTION1(h, !_isForce);
		} 
		else
		{
			_exitCallback.push_back(h);
		}
	}
}

void my_actor::notify_suspend()
{
	notify_suspend(boost::function<void ()>());
}

void my_actor::notify_suspend(const boost::function<void ()>& h)
{
	actor_handle shared_this = shared_from_this();
	_strand->post([=](){shared_this->suspend(h); });
}

void my_actor::suspend(const boost::function<void ()>& h)
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
			if (_timerSleep)
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
						(*it)->suspend([shared_this](){shared_this->child_suspend_cb_handler(); });
					}
					else
					{
						(*it)->notify_suspend(_strand->wrap_post([shared_this](){shared_this->child_suspend_cb_handler(); }));
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
	notify_resume(boost::function<void ()>());
}

void my_actor::notify_resume(const boost::function<void ()>& h)
{
	actor_handle shared_this = shared_from_this();
	_strand->post([=](){shared_this->resume(h); });
}

void my_actor::resume(const boost::function<void ()>& h)
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
			if (_timerSleep)
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
						(*it)->resume(_strand->wrap_post([shared_this](){shared_this->child_resume_cb_handler(); }));
					} 
					else
					{
						(*it)->notify_resume(_strand->wrap_post([shared_this](){shared_this->child_resume_cb_handler(); }));
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
	switch_pause_play(boost::function<void (bool isPaused)>());
}

void my_actor::switch_pause_play(const boost::function<void (bool isPaused)>& h)
{
	actor_handle shared_this = shared_from_this();
	_strand->post([this, shared_this, h]()
	{
		assert(_strand->running_in_this_thread());
		if (!_quited)
		{
			if (_suspended)
			{
				if (h)
				{
					auto& h_ = h;
					resume([h_](){h_(false); });
				} 
				else
				{
					resume(boost::function<void ()>());
				}
			} 
			else
			{
				if (h)
				{
					auto& h_ = h;
					suspend([h_](){h_(true); });
				} 
				else
				{
					suspend(boost::function<void ()>());
				}
			}
		}
		else if (h)
		{
			CHECK_EXCEPTION1(h, true);
		}
	});
}

bool my_actor::outside_wait_quit()
{
	assert(!_strand->in_this_ios());
	bool rt = false;
	boost::condition_variable conVar;
	boost::mutex mutex;
	boost::unique_lock<boost::mutex> ul(mutex);
	_strand->post([&]()
	{
		assert(_strand->running_in_this_thread());
		if (_quited && !_childOverCount)
		{
			rt = !_isForce;
			boost::lock_guard<boost::mutex> lg(mutex);
			conVar.notify_one();
		}
		else
		{
			bool& rt_ = rt;
			auto& mutex_ = mutex;
			auto& conVar_ = conVar;
			_exitCallback.push_back([&](bool ok)
			{
				rt_ = ok;
				boost::lock_guard<boost::mutex> lg(mutex_);
				conVar_.notify_one();
			});
		}
	});
	conVar.wait(ul);
	return rt;
}

void my_actor::append_quit_callback(const boost::function<void (bool)>& h)
{
	if (_strand->running_in_this_thread())
	{
		if (_quited && !_childOverCount)
		{
			CHECK_EXCEPTION1(h, !_isForce);
		} 
		else
		{
			_exitCallback.push_back(h);
		}
	} 
	else
	{
		actor_handle shared_this = shared_from_this();
		_strand->post([=](){shared_this->append_quit_callback(h); });
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

bool my_actor::actor_force_quit( actor_handle anotherActor )
{
	assert_enter();
	assert(anotherActor);
	return trig<bool>([anotherActor](const boost::function<void (bool)>& h){anotherActor->notify_quit(h); });
}

void my_actor::actors_force_quit(const list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		actor_force_quit(*it);
	}
}

bool my_actor::actor_wait_quit( actor_handle anotherActor )
{
	assert_enter();
	assert(anotherActor);
	return trig<bool>([anotherActor](const boost::function<void(bool)>& h){anotherActor->append_quit_callback(h); });
}

void my_actor::actors_wait_quit(const list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		actor_wait_quit(*it);
	}
}

void my_actor::actor_suspend(actor_handle anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](const boost::function<void()>& h){anotherActor->notify_suspend(h); });
}

void my_actor::actors_suspend(const list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		actor_suspend(*it);
	}
}

void my_actor::actor_resume(actor_handle anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig([anotherActor](const boost::function<void()>& h){anotherActor->notify_resume(h); });
}

void my_actor::actors_resume(const list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		actor_resume(*it);
	}
}

bool my_actor::actor_switch(actor_handle anotherActor)
{
	assert_enter();
	assert(anotherActor);
	return trig<bool>([anotherActor](const boost::function<void(bool)>& h){anotherActor->switch_pause_play(h); });
}

bool my_actor::actors_switch(const list<actor_handle>& anotherActors)
{
	assert_enter();
	bool isPause = true;
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		isPause &= actor_switch(*it);
	}
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

void my_actor::push_yield()
{
	assert(!_quited);
	assert(_inActor);
	_yieldCount++;
	DEBUG_OPERATION(_inActor = false);
	(*(actor_push_type*)_actorPush)();
	if (!_quited)
	{
		DEBUG_OPERATION(_inActor = true);
		return;
	}
	throw force_quit_exception();
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
					CHECK_EXCEPTION(tmp._h);
				}
				_suspendResumeQueue.pop_front();
			} 
			else
			{
				actor_handle shared_this = shared_from_this();
				_strand->post([shared_this](){shared_this->resume(); });
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
					CHECK_EXCEPTION(tmp._h);
				}
				_suspendResumeQueue.pop_front();
			} 
			else
			{
				actor_handle shared_this = shared_from_this();
				_strand->post([shared_this](){shared_this->suspend(); });
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
	while (!_quitHandlerList.empty())
	{
		CHECK_EXCEPTION(_quitHandlerList.front());
		_quitHandlerList.pop_front();
	}
	(*(actor_pull_type*)_actorPull)();
}

void my_actor::enable_stack_pool()
{
	assert(0 == _actorIDCount);
	actor_stack_pool::enable();
}

void my_actor::expires_timer()
{
	size_t tid = ++_timerSleep->_timerCount;
	actor_handle shared_this = shared_from_this();
	boost::system::error_code ec;
	_timerSleep->_timer.expires_from_now(boost::chrono::microseconds(_timerSleep->_timerTime.total_microseconds()), ec);
	_timerSleep->_timer.async_wait(_strand->wrap_post<boost::function<void (const boost::system::error_code&)> >
		([this, shared_this, tid](const boost::system::error_code& err)
	{
		if (!err && _timerSleep && tid == _timerSleep->_timerCount)
		{
			if (!_timerSleep->_timerSuspend && !_timerSleep->_timerCompleted)
			{
				_timerSleep->_timerCompleted = true;
				auto h = _timerSleep->_h;
				_timerSleep->_h.clear();
				h();
			}
		}
	}));
}

void my_actor::time_out(int ms, const boost::function<void ()>& h)
{
	assert_enter();
	assert(_timerSleep);
	assert(_timerSleep->_timerCompleted);
	_timerSleep->_timerCompleted = false;
	unsigned long long tms;
	if (ms >= 0)
	{
		tms = (unsigned int)ms;
	} 
	else
	{
		tms = 0xFFFFFFFFFFFF;
	}
	_timerSleep->_h = h;
	_timerSleep->_timerTime = boost::posix_time::microsec(tms*1000);
	_timerSleep->_timerStampBegin = boost::posix_time::microsec_clock::universal_time();
	if (!_timerSleep->_timerSuspend)
	{
		expires_timer();
	}
	else
	{
		_timerSleep->_timerStampEnd = _timerSleep->_timerStampBegin;
	}
}

void my_actor::cancel_timer()
{
	assert(_timerSleep);
	if (!_timerSleep->_timerCompleted)
	{
		_timerSleep->_timerCompleted = true;
		_timerSleep->_h.clear();
		boost::system::error_code ec;
		_timerSleep->_timer.cancel(ec);
	}
}

void my_actor::suspend_timer()
{
	assert(_timerSleep);
	if (!_timerSleep->_timerSuspend)
	{
		_timerSleep->_timerSuspend = true;
		if (!_timerSleep->_timerCompleted)
		{
			boost::system::error_code ec;
			_timerSleep->_timer.cancel(ec);
			_timerSleep->_timerStampEnd = boost::posix_time::microsec_clock::universal_time();
			auto tt = _timerSleep->_timerStampBegin+_timerSleep->_timerTime;
			if (_timerSleep->_timerStampEnd > tt)
			{
				_timerSleep->_timerStampEnd = tt;
			}
		}
	}
}

void my_actor::resume_timer()
{
	assert(_timerSleep);
	if (_timerSleep->_timerSuspend)
	{
		_timerSleep->_timerSuspend = false;
		if (!_timerSleep->_timerCompleted)
		{
			assert(_timerSleep->_timerTime >= _timerSleep->_timerStampEnd-_timerSleep->_timerStampBegin);
			_timerSleep->_timerTime -= _timerSleep->_timerStampEnd-_timerSleep->_timerStampBegin;
			_timerSleep->_timerStampBegin = boost::posix_time::microsec_clock::universal_time();
			expires_timer();
		}
	}
}

void my_actor::disable_auto_make_timer()
{
	assert(0 == _actorIDCount);
	_autoMakeTimer = false;
}

void my_actor::check_stack()
{
#if (CHECK_ACTOR_STACK) || (_DEBUG)
	if ((size_t)get_sp() < (size_t)_stackTop-_stackSize+STACK_RESERVED_SPACE_SIZE)
	{
		assert(false);
		throw boost::shared_ptr<string>(new string("Actor堆栈异常"));
	}
#endif
}

size_t my_actor::stack_free_space()
{
	int s = (int)((size_t)get_sp() - ((size_t)_stackTop-_stackSize+STACK_RESERVED_SPACE_SIZE));
	if (s < 0)
	{
		return 0;
	}
	return (size_t)s;
}