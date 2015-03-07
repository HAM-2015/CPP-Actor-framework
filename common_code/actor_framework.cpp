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

	void operator ()(boost_actor* actor)
	{
		actor->~boost_actor();
		actor_stack_pool::recovery(_stack);
	}

	stack_pck _stack;
};

void check_force_quit::reset()
{
	_force_quit = false;
}

check_force_quit::~check_force_quit()
{
	assert(!_force_quit);
}

check_force_quit::check_force_quit()
{
	_force_quit = true;
}
//////////////////////////////////////////////////////////////////////////

bool param_list_base::closed()
{
	return (*_pIsClosed);
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

size_t null_param_list::get_size()
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
	DEBUG_OPERATION(_qh = s._actor->parent_actor()->regist_quit_handler(boost::bind(&child_actor_handle::quited_set, this)));//在父Actor退出时置_quited=true
	return *this;
}

child_actor_handle::~child_actor_handle()
{
	assert(_quited);
}

void child_actor_handle::quited_set()
{
	assert(!_quited);
	_quited = true;
}

void child_actor_handle::cancel_quit_it()
{
	DEBUG_OPERATION(_param._actor->parent_actor()->cancel_quit_handler(_qh));
}

actor_handle child_actor_handle::get_actor()
{
	return _param._actor;
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

struct boost_actor::timer_pck
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

class boost_actor::boost_actor_run
{
public:
	boost_actor_run(boost_actor& actor)
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
		catch (boost_actor::actor_force_quit&)
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
	boost_actor& _actor;
};

boost_actor::boost_actor()
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

boost_actor::boost_actor(const boost_actor&)
{

}

boost_actor::~boost_actor()
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

boost_actor& boost_actor::operator =(const boost_actor&)
{
	return *this;
}

actor_handle boost_actor::create( shared_strand actorStrand, const main_func& mainFunc, size_t stackSize )
{
	return create(actorStrand, mainFunc, boost::function<void (bool)>(), stackSize);
}

actor_handle boost_actor::create( shared_strand actorStrand, const main_func& mainFunc,
	const boost::function<void (bool)>& cb, size_t stackSize )
{
	assert(stackSize && stackSize <= 1024 kB && 0 == stackSize % (4 kB));
	actor_handle newActor;
	if (actor_stack_pool::isEnable())
	{
		size_t actorSize = MEM_ALIGN(sizeof(boost_actor), sizeof(void*));
		size_t timerSize = MEM_ALIGN(sizeof(timer_pck), sizeof(void*));
		stack_pck stackMem = actor_stack_pool::getStack(MEM_ALIGN(actorSize+timerSize+stackSize, 4 kB));
		size_t totalSize = stackMem._stack.size;
		BYTE* stackTop = (BYTE*)stackMem._stack.sp;
		newActor = actor_handle(new(stackTop-actorSize) boost_actor, actor_free(stackMem));
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
		newActor = actor_handle(new boost_actor);
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

void boost_actor::async_create( shared_strand actorStrand, const main_func& mainFunc,
	const boost::function<void (actor_handle)>& ch, size_t stackSize )
{
	boost_strand::asyncInvoke<actor_handle>(actorStrand, boost::bind(&create, actorStrand, mainFunc, stackSize), ch);
}

void boost_actor::async_create( shared_strand actorStrand, const main_func& mainFunc,
	const boost::function<void (actor_handle)>& ch, const boost::function<void (bool)>& cb, size_t stackSize )
{
	boost_strand::asyncInvoke<actor_handle>(actorStrand, boost::bind(&create, actorStrand, mainFunc, cb, stackSize), ch);
}

child_actor_handle::child_actor_param boost_actor::create_child_actor( shared_strand actorStrand, const main_func& mainFunc, size_t stackSize )
{
	assert_enter();

	child_actor_handle::child_actor_param actorHandle;
	if (actorStrand != _strand)
	{//不依赖于同一个strand，采用异步创建
		boost_actor::async_create(actorStrand, mainFunc, _strand->wrap_post(
			boost::bind(&boost_actor::create_actor_handler, shared_from_this(), _1,
			boost::ref(actorHandle._actor), boost::ref(actorHandle._actorIt))
			), stackSize);
		push_yield();//等待创建完成后通知
	}
	else
	{
		actorHandle._actor = boost_actor::create(actorStrand, mainFunc, stackSize);
		_childActorList.push_front(actorHandle._actor);
		actorHandle._actorIt = _childActorList.begin();
		actorHandle._actor->_parentActor = shared_from_this();
	}
	DEBUG_OPERATION(actorHandle._isCopy = false);
	return actorHandle;
}

child_actor_handle::child_actor_param boost_actor::create_child_actor(const main_func& mainFunc, size_t stackSize)
{
	return create_child_actor(this_strand(), mainFunc, stackSize);
}

void boost_actor::child_actor_run( child_actor_handle& actorHandle )
{
	assert_enter();
	assert(!actorHandle._quited);
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor().get() == this);

	actorHandle._param._actor->notify_start_run();
}

void boost_actor::child_actor_run(const list<boost::shared_ptr<child_actor_handle> >& actorHandles)
{
	assert_enter();
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		child_actor_run(**it);
	}
}

bool boost_actor::child_actor_quit( child_actor_handle& actorHandle )
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor().get() == this);

		if (actorHandle._param._actor->_strand == _strand)
		{
			trig<bool>(boost::bind(&boost_actor::force_quit, actorHandle._param._actor, _1), actorHandle._norQuit);
		} 
		else
		{
			trig<bool>(boost::bind(&boost_actor::notify_force_quit, actorHandle._param._actor, _1), actorHandle._norQuit);
		}
		actorHandle.quited_set();
		actorHandle.cancel_quit_it();
		actorHandle._param._actor.reset();
		_childActorList.erase(actorHandle._param._actorIt);
	}
	return actorHandle._norQuit;
}

bool boost_actor::child_actor_quit(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	bool res = true;
	actor_msg_handle<child_actor_handle::ptr, bool> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		assert((*it)->get_actor());
		assert((*it)->get_actor()->parent_actor().get() == this);
		if ((*it)->get_actor()->_strand == _strand)
		{
			(*it)->get_actor()->force_quit(boost::bind(h, *it, _1));
		} 
		else
		{
			(*it)->get_actor()->notify_force_quit(boost::bind(h, *it, _1));
		}
	}
	for (int i = (int)actorHandles.size(); i > 0; i--)
	{
		bool ok = true;
		child_actor_handle::ptr cp;
		pump_msg(cmh, cp, ok);
		res &= ok;
		cp->quited_set();
		cp->cancel_quit_it();
		cp->_param._actor.reset();
		_childActorList.erase(cp->_param._actorIt);
	}
	close_msg_notify(cmh);
	return res;
}

bool boost_actor::child_actor_wait_quit( child_actor_handle& actorHandle )
{
	assert_enter();
	if (!actorHandle._quited)
	{
		assert(actorHandle.get_actor());
		assert(actorHandle.get_actor()->parent_actor().get() == this);

		actorHandle._norQuit = another_actor_wait_quit(actorHandle._param._actor);
		actorHandle.quited_set();
		actorHandle.cancel_quit_it();
		actorHandle._param._actor.reset();
		_childActorList.erase(actorHandle._param._actorIt);
	}
	return actorHandle._norQuit;
}

bool boost_actor::child_actors_wait_quit(const list<child_actor_handle::ptr>& actorHandles)
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

void boost_actor::child_actor_suspend(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor().get() == this);
	if (actorHandle.get_actor()->_strand == _strand)
	{
		trig(boost::bind(&boost_actor::suspend, actorHandle.get_actor(), _1));
	} 
	else
	{
		trig(boost::bind(&boost_actor::notify_suspend, actorHandle.get_actor(), _1));
	}
}

void boost_actor::child_actors_suspend(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	actor_msg_handle<> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		assert((*it)->get_actor());
		assert((*it)->get_actor()->parent_actor().get() == this);
		if ((*it)->get_actor()->_strand == _strand)
		{
			(*it)->get_actor()->suspend(h);
		} 
		else
		{
			(*it)->get_actor()->notify_suspend(h);
		}
	}
	for (int i = (int)actorHandles.size(); i > 0; i--)
	{
		pump_msg(cmh);
	}
	close_msg_notify(cmh);
}

void boost_actor::child_actor_resume(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor().get() == this);
	if (actorHandle.get_actor()->_strand == _strand)
	{
		trig(boost::bind(&boost_actor::resume, actorHandle.get_actor(), _1));
	}
	else
	{
		trig(boost::bind(&boost_actor::notify_resume, actorHandle.get_actor(), _1));
	}
}

void boost_actor::child_actors_resume(const list<child_actor_handle::ptr>& actorHandles)
{
	assert_enter();
	actor_msg_handle<> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = actorHandles.begin(); it != actorHandles.end(); it++)
	{
		assert((*it)->get_actor());
		assert((*it)->get_actor()->parent_actor().get() == this);
		if ((*it)->get_actor()->_strand == _strand)
		{
			(*it)->get_actor()->resume(h);
		}
		else
		{
			(*it)->get_actor()->notify_resume(h);
		}
	}
	for (int i = (int)actorHandles.size(); i > 0; i--)
	{
		pump_msg(cmh);
	}
	close_msg_notify(cmh);
}

actor_handle boost_actor::child_actor_peel(child_actor_handle& actorHandle)
{
	assert_enter();
	assert(!actorHandle._quited);
	assert(actorHandle.get_actor());
	assert(actorHandle.get_actor()->parent_actor().get() == this);

	actorHandle.quited_set();
	actorHandle.cancel_quit_it();
	actorHandle._norQuit = false;
	_childActorList.erase(actorHandle._param._actorIt);
	actor_handle r = actorHandle._param._actor;
	actorHandle._param._actor.reset();
	r->_parentActor.reset();
	return r;
}

bool boost_actor::run_child_actor_complete( shared_strand actorStrand, const main_func& h, size_t stackSize )
{
	assert_enter();
	child_actor_handle actorHandle = create_child_actor(actorStrand, h, stackSize);
	child_actor_run(actorHandle);
	return child_actor_wait_quit(actorHandle);
}

bool boost_actor::run_child_actor_complete(const main_func& h, size_t stackSize)
{
	return run_child_actor_complete(this_strand(), h, stackSize);
}

void boost_actor::open_timer()
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

void boost_actor::sleep( int ms )
{
	assert_enter();
	assert(ms >= 0);
	if (ms)
	{
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::run_one, shared_from_this()));
	}
	else
	{
		_strand->post(boost::bind(&boost_actor::run_one, shared_from_this()));
	}
	push_yield();
}

actor_handle boost_actor::parent_actor()
{
	return _parentActor.lock();
}

const list<actor_handle>& boost_actor::child_actors()
{
	return _childActorList;
}

boost_actor::quit_handle boost_actor::regist_quit_handler( const boost::function<void ()>& quitHandler )
{
	assert_enter();
	_quitHandlerList.push_front(quitHandler);//后注册的先执行
	return _quitHandlerList.begin();
}

void boost_actor::cancel_quit_handler( quit_handle rh )
{
	assert_enter();
	_quitHandlerList.erase(rh);
}

boost::function<void ()> boost_actor::begin_trig(async_trig_handle<>& th)
{
	assert_enter();
	th.begin(_actorID);
	auto pIsClosed = th._pIsClosed;
	actor_handle shared_this = shared_from_this();
	return [this, shared_this, pIsClosed, &th]()
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th._notify)
			{
				async_trig_post_yield(th, NULL);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler, shared_this, pIsClosed, boost::ref(th)));
		}
	};
}

boost::function<void ()> boost_actor::begin_trig(boost::shared_ptr<async_trig_handle<> > th)
{
	assert_enter();
	th->begin(_actorID);
	auto pIsClosed = th->_pIsClosed;
	actor_handle shared_this = shared_from_this();
	return [this, shared_this, pIsClosed, th]()
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th->_notify)
			{
				async_trig_post_yield(*th, NULL);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler_ptr, shared_this, pIsClosed, th));
		}
	};
}

bool boost_actor::timed_wait_trig(async_trig_handle<>& th, int tm)
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

void boost_actor::wait_trig(async_trig_handle<>& th)
{
	timed_wait_trig(th, -1);
}

void boost_actor::close_trig(async_trig_base& th)
{
	DEBUG_OPERATION(if (th._actorID) {assert(th._actorID == _actorID); th._actorID = 0;})
	th.close();
}

void boost_actor::delay_trig(int ms, const boost::function<void ()>& h)
{
	assert_enter();
	time_out(ms, h);
}

void boost_actor::delay_trig(int ms, async_trig_handle<>& th)
{
	assert_enter();
	assert(th._pIsClosed);
	time_out(ms, boost::bind(&boost_actor::_async_trig_handler, shared_from_this(), 
		th._pIsClosed, boost::ref(th)));
}

void boost_actor::delay_trig(int ms, boost::shared_ptr<async_trig_handle<> > th)
{
	assert_enter();
	assert(th->_pIsClosed);
	time_out(ms, boost::bind(&boost_actor::_async_trig_handler_ptr, shared_from_this(), 
		th->_pIsClosed, th));
}

void boost_actor::cancel_delay_trig()
{
	assert_enter();
	cancel_timer();
}

void boost_actor::trig( const boost::function<void (boost::function<void ()>)>& h )
{
	assert_enter();
#ifdef _DEBUG
	h(wrapped_trig_handler<boost::function<void ()> >(boost::bind(&boost_actor::trig_handler, shared_from_this())));
#else
	h(boost::bind(&boost_actor::trig_handler, shared_from_this()));
#endif
	push_yield();
}

void boost_actor::send( shared_strand exeStrand, const boost::function<void ()>& h )
{
	assert_enter();
	if (exeStrand != _strand)
	{
		trig(boost::bind(&boost_strand::asyncInvokeVoid, exeStrand, h, _1));
	} 
	else
	{
		h();
	}
}

boost::function<void()> boost_actor::make_msg_notify(actor_msg_handle<>& cmh)
{
	cmh.begin(_actorID);
	return _strand->wrap_post(boost::bind(&boost_actor::check_run1, shared_from_this(), cmh._pIsClosed, boost::ref(cmh)));
}

boost::function<void()> boost_actor::make_msg_notify(boost::shared_ptr<actor_msg_handle<> > cmh)
{
	cmh->begin(_actorID);
	return _strand->wrap_post(boost::bind(&boost_actor::check_run1_ptr, shared_from_this(), cmh->_pIsClosed, cmh));
}

void boost_actor::close_msg_notify( param_list_base& cmh )
{
	DEBUG_OPERATION(if (cmh._actorID) {assert(cmh._actorID == _actorID); cmh._actorID = 0;})
	cmh.close();
}

bool boost_actor::timed_pump_msg(actor_msg_handle<>& cmh, int tm)
{
	assert(cmh._actorID == _actorID);
	assert_enter();
	assert(cmh._pIsClosed);
	if (cmh.count)
	{
		cmh.count--;
		return true;
	}
	cmh._waiting = true;
	return pump_msg_push(cmh, tm);
}

void boost_actor::pump_msg(actor_msg_handle<>& cmh)
{
	timed_pump_msg(cmh, -1);
}

bool boost_actor::pump_msg_push(param_list_base& pm, int tm)
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

void boost_actor::trig_handler()
{
	_strand->post(boost::bind(&boost_actor::run_one, shared_from_this()));
}

bool boost_actor::async_trig_push(async_trig_base& th, int tm, void* pref)
{
	assert(th._pIsClosed);
	if (!th._notify)
	{
		th._waiting = true;
		th._dstRefPt = pref;
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
		th.get_param(pref);
	}
	return true;
}

void boost_actor::async_trig_post_yield(async_trig_base& th, void* cref)
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
		th.set_ref(cref);
		_strand->post(boost::bind(&boost_actor::run_one, shared_from_this()));
	} 
	else
	{
		th.set_temp(cref);
	}
}

void boost_actor::async_trig_pull_yield(async_trig_base& th, void* cref)
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
		th.set_ref(cref);
		pull_yield();
	} 
	else
	{
		th.set_temp(cref);
	}
}

void boost_actor::_async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<>& th)
{
	assert(_strand->running_in_this_thread());
	if (!_quited && !(*pIsClosed) && !th._notify)
	{
		async_trig_pull_yield(th, NULL);
	}
}

void boost_actor::_async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<> >& th)
{
	_async_trig_handler(pIsClosed, *th);
}

void boost_actor::create_actor_handler( actor_handle actor, actor_handle& retActor, list<actor_handle>::iterator& ch )
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);
	if (_quited)
	{//父Actor已经退出，那么子Actor刚创建也必须退出
		actor->notify_force_quit();
		return;
	}

	retActor = actor;
	_childActorList.push_front(actor);
	ch = _childActorList.begin();
	actor->_parentActor = shared_from_this();
	pull_yield();
}

void boost_actor::check_run1( boost::shared_ptr<bool>& pIsClosed, actor_msg_handle<>& cmh )
{
	if (_quited || (*pIsClosed)) return;

	if (cmh._waiting)
	{
		cmh._waiting = false;
		if (cmh._hasTm)
		{
			cmh._hasTm = false;
			cancel_timer();
		}
		assert(0 == cmh.count);
		pull_yield();
	} 
	else
	{
		cmh.count++;
	}
}

void boost_actor::check_run1_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<actor_msg_handle<> >& cmh)
{
	check_run1(pIsClosed, *cmh);
}

shared_strand boost_actor::this_strand()
{
	return _strand;
}

actor_handle boost_actor::shared_from_this()
{
	return _weakThis.lock();
}

long long boost_actor::this_id()
{
	return _actorID;
}

size_t boost_actor::yield_count()
{
	assert_enter();
	return _yieldCount;
}

void boost_actor::reset_yield()
{
	assert_enter();
	_yieldCount = 0;
}

void boost_actor::notify_start_run()
{
	_strand->post(boost::bind(&boost_actor::start_run, shared_from_this()));
}

void boost_actor::assert_enter()
{
	assert(_strand->running_in_this_thread());
	assert(!_quited);
	assert(_inActor);
	assert((size_t)get_sp() >= (size_t)_stackTop-_stackSize+1024);
}

void boost_actor::start_run()
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
		_strand->post(boost::bind(&boost_actor::start_run, shared_from_this()));
	}
}

void boost_actor::notify_force_quit()
{
	_strand->post(boost::bind(&boost_actor::force_quit, shared_from_this(), boost::function<void (bool)>()));
}

void boost_actor::notify_force_quit(const boost::function<void (bool)>& h)
{
	_strand->post(boost::bind(&boost_actor::force_quit, shared_from_this(), h));
}

void boost_actor::force_quit( const boost::function<void (bool)>& h )
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
				if (cc->_strand == _strand)
				{
					cc->force_quit(boost::bind(&boost_actor::force_quit_cb_handler, shared_from_this()));
				} 
				else
				{
					cc->notify_force_quit(_strand->wrap_post(boost::bind(&boost_actor::force_quit_cb_handler, shared_from_this())));
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

void boost_actor::notify_suspend()
{
	notify_suspend(boost::function<void ()>());
}

void boost_actor::notify_suspend(const boost::function<void ()>& h)
{
	_strand->post(boost::bind(&boost_actor::suspend, shared_from_this(), h));
}

void boost_actor::suspend(const boost::function<void ()>& h)
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

void boost_actor::suspend()
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
					if ((*it)->_strand == _strand)
					{
						(*it)->suspend(boost::bind(&boost_actor::child_suspend_cb_handler, shared_from_this()));
					}
					else
					{
						(*it)->notify_suspend(_strand->wrap_post(boost::bind(&boost_actor::child_suspend_cb_handler, shared_from_this())));
					}
				}
				return;
			} 
		}
	}
	_childSuspendResumeCount = 1;
	child_suspend_cb_handler();
}

void boost_actor::notify_resume()
{
	notify_resume(boost::function<void ()>());
}

void boost_actor::notify_resume(const boost::function<void ()>& h)
{
	_strand->post(boost::bind(&boost_actor::resume, shared_from_this(), h));
}

void boost_actor::resume(const boost::function<void ()>& h)
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

void boost_actor::resume()
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
					if ((*it)->_strand == _strand)
					{
						(*it)->resume(_strand->wrap_post(boost::bind(&boost_actor::child_resume_cb_handler, shared_from_this())));
					} 
					else
					{
						(*it)->notify_resume(_strand->wrap_post(boost::bind(&boost_actor::child_resume_cb_handler, shared_from_this())));
					}
				}
				return;
			} 
		}
	}
	_childSuspendResumeCount = 1;
	child_resume_cb_handler();
}

void boost_actor::switch_pause_play()
{
	switch_pause_play(boost::function<void (bool isPaused)>());
}

void boost_actor::switch_pause_play(const boost::function<void (bool isPaused)>& h)
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
					resume(boost::bind(h, false));
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
					suspend(boost::bind(h, true));
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

bool boost_actor::outside_wait_quit()
{
	assert(!_strand->in_this_ios());
	bool rt = false;
	boost::condition_variable conVar;
	boost::mutex mutex;
	boost::unique_lock<boost::mutex> ul(mutex);
	_strand->post(boost::bind(&boost_actor::outside_wait_quit_proxy, shared_from_this(), &conVar, &mutex, &rt));
	conVar.wait(ul);
	return rt;
}

void boost_actor::append_quit_callback(const boost::function<void (bool)>& h)
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
		_strand->post(boost::bind(&boost_actor::append_quit_callback, shared_from_this(), h));
	}
}

void boost_actor::another_actors_start_run(const list<actor_handle>& anotherActors)
{
	assert_enter();
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->notify_start_run();
	}
}

bool boost_actor::another_actor_force_quit( actor_handle anotherActor )
{
	assert_enter();
	assert(anotherActor);
	bool ok = false;
	trig<bool>(boost::bind(&boost_actor::notify_force_quit, anotherActor, _1), ok);
	return ok;
}

void boost_actor::another_actors_force_quit(const list<actor_handle>& anotherActors)
{
	assert_enter();
	actor_msg_handle<bool> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->notify_force_quit(h);
	}
	for (int i = (int)anotherActors.size(); i > 0; i--)
	{
		bool ok;
		pump_msg(cmh, ok);
	}
	close_msg_notify(cmh);
}

bool boost_actor::another_actor_wait_quit( actor_handle anotherActor )
{
	assert_enter();
	assert(anotherActor);
	bool ok = false;
	trig<bool>(boost::bind(&boost_actor::append_quit_callback, anotherActor, _1), ok);
	return ok;
}

void boost_actor::another_actors_wait_quit(const list<actor_handle>& anotherActors)
{
	assert_enter();
	actor_msg_handle<bool> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->append_quit_callback(h);
	}
	for (int i = (int)anotherActors.size(); i > 0; i--)
	{
		bool ok;
		pump_msg(cmh, ok);
	}
	close_msg_notify(cmh);
}

void boost_actor::another_actor_suspend(actor_handle anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig(boost::bind(&boost_actor::notify_suspend, anotherActor, _1));
}

void boost_actor::another_actors_suspend(const list<actor_handle>& anotherActors)
{
	assert_enter();
	actor_msg_handle<> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->notify_suspend(h);
	}
	for (int i = (int)anotherActors.size(); i > 0; i--)
	{
		pump_msg(cmh);
	}
	close_msg_notify(cmh);
}

void boost_actor::another_actor_resume(actor_handle anotherActor)
{
	assert_enter();
	assert(anotherActor);
	trig(boost::bind(&boost_actor::notify_resume, anotherActor, _1));
}

void boost_actor::another_actors_resume(const list<actor_handle>& anotherActors)
{
	assert_enter();
	actor_msg_handle<> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->notify_resume(h);
	}
	for (int i = (int)anotherActors.size(); i > 0; i--)
	{
		pump_msg(cmh);
	}
	close_msg_notify(cmh);
}

bool boost_actor::another_actor_switch(actor_handle anotherActor)
{
	assert_enter();
	assert(anotherActor);
	bool isPause = true;
	trig<bool>(boost::bind(&boost_actor::switch_pause_play, anotherActor, _1), isPause);
	return isPause;
}

bool boost_actor::another_actors_switch(const list<actor_handle>& anotherActors)
{
	assert_enter();
	bool isPause = true;
	actor_msg_handle<bool> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherActors.begin(); it != anotherActors.end(); it++)
	{
		(*it)->switch_pause_play(h);
	}
	for (int i = (int)anotherActors.size(); i > 0; i--)
	{
		isPause &= pump_msg(cmh);
	}
	close_msg_notify(cmh);
	return isPause;
}

void boost_actor::run_one()
{
	assert(_strand->running_in_this_thread());
	assert(!_inActor);
	if (!_quited)
	{
		pull_yield();
	}
}

void boost_actor::pull_yield()
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

void boost_actor::push_yield()
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
	throw actor_force_quit();
}

void boost_actor::force_quit_cb_handler()
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

void boost_actor::child_suspend_cb_handler()
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
				_strand->post(boost::bind(&boost_actor::resume, shared_from_this()));
				return;
			}
		}
	}
}

void boost_actor::child_resume_cb_handler()
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
				_strand->post(boost::bind(&boost_actor::suspend, shared_from_this()));
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

void boost_actor::exit_callback()
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

void boost_actor::outside_wait_quit_proxy( boost::condition_variable* conVar, boost::mutex* mutex, bool* rt )
{
	assert(_strand->running_in_this_thread());
	if (_quited && !_childOverCount)
	{
		*rt = !_isForce;
		boost::lock_guard<boost::mutex> lg(*mutex);
		conVar->notify_one();
	} 
	else
	{
		_exitCallback.push_back(boost::bind(&boost_actor::outside_wait_quit_handler, shared_from_this(), conVar, mutex, rt, _1));
	}
}

void boost_actor::outside_wait_quit_handler( boost::condition_variable* conVar, boost::mutex* mutex, bool* rt, bool ok )
{
	*rt = ok;
	boost::lock_guard<boost::mutex> lg(*mutex);
	conVar->notify_one();
}

void boost_actor::enable_stack_pool()
{
	assert(0 == _actorIDCount);
	actor_stack_pool::enable();
}

void boost_actor::expires_timer()
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

void boost_actor::time_out(int ms, const boost::function<void ()>& h)
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

void boost_actor::cancel_timer()
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

void boost_actor::suspend_timer()
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

void boost_actor::resume_timer()
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

void boost_actor::disable_auto_make_timer()
{
	assert(0 == _actorIDCount);
	_autoMakeTimer = false;
}

void boost_actor::check_stack()
{
#if (CHECK_ACTOR_STACK) || (_DEBUG)
	if ((size_t)get_sp() < (size_t)_stackTop-_stackSize+STACK_RESERVED_SPACE_SIZE)
	{
		assert(false);
		throw boost::shared_ptr<string>(new string("Actor堆栈异常"));
	}
#endif
}

size_t boost_actor::stack_free_space()
{
	int s = (int)((size_t)get_sp() - ((size_t)_stackTop-_stackSize+STACK_RESERVED_SPACE_SIZE));
	if (s < 0)
	{
		return 0;
	}
	return (size_t)s;
}