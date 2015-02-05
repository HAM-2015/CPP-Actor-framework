#define WIN32_LEAN_AND_MEAN

#include <boost/coroutine/all.hpp>
#include "boost_coroutine.h"
#include "coro_stack.h"
#include "asm_ex.h"

typedef boost::coroutines::coroutine<void>::pull_type coro_pull_type;
typedef boost::coroutines::coroutine<void>::push_type coro_push_type;

#ifdef _DEBUG

#define CHECK_EXCEPTION(h) try { (h)(); } catch (...) { assert(false); }
#define CHECK_EXCEPTION1(h, p0) try { (h)(p0); } catch (...) { assert(false); }
#define CHECK_EXCEPTION2(h, p0, p1) try { (h)(p0, p1); } catch (...) { assert(false); }
#define CHECK_EXCEPTION3(h, p0, p1, p2) try { (h)(p0, p1, p2); } catch (...) { assert(false); }

#else

#define CHECK_EXCEPTION(h) (h)()
#define CHECK_EXCEPTION1(h, p0) (h)(p0)
#define CHECK_EXCEPTION2(h, p0, p1) (h)(p0, p1)
#define CHECK_EXCEPTION3(h, p0, p1, p2) (h)(p0, p1, p2)

#endif //end _DEBUG


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
	return (*_ptrClosed);
}

void param_list_base::begin(long long coroID)
{
	close();
	_timeout = false;
	_hasTm = false;
	DEBUG_OPERATION(_coroID = coroID);
	_ptrClosed = boost::shared_ptr<bool>(new bool(false));
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
	DEBUG_OPERATION(_coroID = 0);
}

param_list_base::~param_list_base()
{
	if (_ptrClosed)
	{
		(*_ptrClosed) = true;
	}
}
//////////////////////////////////////////////////////////////////////////

void null_param_list::close()
{
	count = 0;
	if (_ptrClosed)
	{
		assert(!(*_ptrClosed));
		(*_ptrClosed) = true;
		_waiting = false;
		_ptrClosed.reset();
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
	DEBUG_OPERATION(_coroID = 0);
}

async_trig_base::~async_trig_base()
{
	close();
}

void async_trig_base::begin(long long coroID)
{
	close();
	_ptrClosed = boost::shared_ptr<bool>(new bool(false));
	_notify = false;
	_timeout = false;
	_hasTm = false;
	DEBUG_OPERATION(_coroID = coroID);
}

void async_trig_base::close()
{
	if (_ptrClosed)
	{
		assert(!(*_ptrClosed));
		(*_ptrClosed) = true;
		_waiting = false;
		_ptrClosed.reset();
	}
}
//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
child_coro_handle::child_coro_param& child_coro_handle::child_coro_param::operator=( child_coro_handle::child_coro_param& s )
{
	_coro = s._coro;
	_coroIt = s._coroIt;
	s._isCopy = true;
	_isCopy = false;
	return *this;
}

child_coro_handle::child_coro_param::~child_coro_param()
{
	assert(_isCopy);//检测创建后有没有接收子协程句柄
}

child_coro_handle::child_coro_param::child_coro_param( child_coro_handle::child_coro_param& s )
{
	*this = s;
}

child_coro_handle::child_coro_param::child_coro_param()
{
	_isCopy = true;
}
#endif

child_coro_handle::child_coro_handle( child_coro_handle& )
{
	assert(false);
}

child_coro_handle::child_coro_handle()
{
	_quited = true;
	_norQuit = false;
}

child_coro_handle::child_coro_handle( child_coro_param& s )
{
	_quited = true;
	_norQuit = false;
	*this = s;
}

child_coro_handle& child_coro_handle::operator=( child_coro_handle& )
{
	assert(false);
	return *this;
}

child_coro_handle& child_coro_handle::operator=( child_coro_param& s )
{
	assert(_quited);
	_quited = false;
	_norQuit = false;
	_param = s;
	DEBUG_OPERATION(_param._isCopy = true);
	DEBUG_OPERATION(_qh = s._coro->parent_coro()->regist_quit_handler(boost::bind(&child_coro_handle::quited_set, this)));//在父协程退出时置_quited=true
	return *this;
}

child_coro_handle::~child_coro_handle()
{
	assert(_quited);
}

void child_coro_handle::quited_set()
{
	assert(!_quited);
	_quited = true;
}

void child_coro_handle::cancel_quit_it()
{
	DEBUG_OPERATION(_param._coro->parent_coro()->cancel_quit_handler(_qh));
}

coro_handle child_coro_handle::get_coro()
{
	return _param._coro;
}

child_coro_handle::ptr child_coro_handle::make_ptr()
{
	return ptr(new child_coro_handle);
}

void* child_coro_handle::operator new(size_t s)
{
	void* p = malloc(s);
	assert(p);
	return p;
}

void child_coro_handle::operator delete(void* p)
{
	if (p)
	{
		free(p);
	}
}

//////////////////////////////////////////////////////////////////////////

boost::atomic<long long> _coroIDCount(0);//ID计数
bool _autoMakeTimer = true;

class boost_coro::boost_coro_run
{
public:
	boost_coro_run(boost_coro& coro)
		: _coro(coro)
	{

	}

	void operator ()( coro_push_type& coroPush )
	{
		assert(_coro._strand->running_in_this_thread());
		assert(!_coro._quited);
		assert(_coro._mainFunc);
		_coro._coroPush = &coroPush;
		try
		{
			_coro._inCoro = true;
			_coro.push_yield();
			_coro._mainFunc(&_coro);
			_coro._inCoro = false;
			_coro._quited = true;
			_coro._mainFunc.clear();
			assert(_coro._childCoroList.empty());
			assert(_coro._quitHandlerList.empty());
			assert(_coro._suspendResumeQueue.empty());
		}
		catch (boost_coro::coro_force_quit&)
		{//捕获协程被强制退出异常
			assert(!_coro._inCoro);
			_coro._mainFunc.clear();
		}
#ifdef _DEBUG
		catch (...)
		{
			assert(false);
		}
#endif
		while (!_coro._exitCallback.empty())
		{
			assert(_coro._exitCallback.front());
			CHECK_EXCEPTION1(_coro._exitCallback.front(), !_coro._isForce);
			_coro._exitCallback.pop_front();
		}
		if (_coro._timerSleep)
		{
			_coro._timerSleep->cancel();
			_coro._timerSleep.reset();
		}
	}
private:
	boost_coro& _coro;
};

boost_coro::boost_coro()
{
	_coroPull = NULL;
	_coroPush = NULL;
	_quited = false;
	_started = false;
	_inCoro = false;
	_suspended = false;
	_hasNotify = false;
	_isForce = false;
	_stackTop = NULL;
	_stackSize = 0;
	_yieldCount = 0;
	_childOverCount = 0;
	_childSuspendResumeCount = 0;
	_coroID = ++_coroIDCount;
}

boost_coro::boost_coro(const boost_coro&)
{

}

boost_coro::~boost_coro()
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
	assert(_childCoroList.empty());
	delete (coro_pull_type*)_coroPull;
}

boost_coro& boost_coro::operator =(const boost_coro&)
{
	return *this;
}

coro_handle boost_coro::outside_create( shared_strand coroStrand, const main_func& mainFunc, size_t stackSize )
{
	assert(!coroStrand->in_this_ios());
	return boost_strand::syncInvoke<coro_handle>(coroStrand, boost::bind(&boost_coro::local_create, coroStrand, mainFunc, stackSize));
}

coro_handle boost_coro::local_create( shared_strand coroStrand, const main_func& mainFunc, size_t stackSize )
{
	return local_create(coroStrand, mainFunc, boost::function<void (bool)>(), stackSize);
}

coro_handle boost_coro::create(shared_strand coroStrand, const main_func& mainFunc, size_t stackSize)
{
	if (!coroStrand->in_this_ios())
	{
		return outside_create(coroStrand, mainFunc, stackSize);
	} 
	else
	{
		return local_create(coroStrand, mainFunc, stackSize);
	}
}

coro_handle boost_coro::local_create( shared_strand coroStrand, const main_func& mainFunc,
	const boost::function<void (bool)>& cb, size_t stackSize )
{
	assert(coroStrand->running_in_this_thread());
	assert(stackSize && stackSize <= 1024 kB && 0 == stackSize % (4 kB));
	coro_handle newCoro;
	if (coro_stack_pool::isEnable())
	{
		size_t objSize = sizeof(boost_coro)+timeout_trig::object_size(timeout_trig::high_resolution_timer);
		void* objPtr = buffer_alloc<>::obj_allocate_size(objSize);
		auto sharedPoolMem = boost::shared_ptr<void>(objPtr, boost::bind(&buffer_alloc<>::obj_deallocate_size, objSize, _1));
		newCoro = coro_handle(new(objPtr) boost_coro, boost::bind(&buffer_alloc<boost_coro>::shared_obj_destroy, sharedPoolMem, _1));
		if (_autoMakeTimer)
		{
			newCoro->_timerSleep = timeout_trig::create_in_pool(sharedPoolMem, (BYTE*)objPtr+sizeof(boost_coro),
				coroStrand, timeout_trig::high_resolution_timer);
		}
		newCoro->_sharedPoolMem = sharedPoolMem;
		newCoro->_strand = coroStrand;
		newCoro->_mainFunc = mainFunc;
		if (cb) newCoro->_exitCallback.push_back(cb);
#if 105500 == BOOST_VERSION
		newCoro->_coroPull = new coro_pull_type(boost_coro_run(*newCoro),
			boost::coroutines::attributes(stackSize), coro_stack(&newCoro->_stackTop, &newCoro->_stackSize), buffer_alloc<>());
#elif 105600 <= BOOST_VERSION
		newCoro->_coroPull = new coro_pull_type(boost_coro_run(*newCoro),
			boost::coroutines::attributes(stackSize), coro_stack(&newCoro->_stackTop, &newCoro->_stackSize));
#else
		assert(false);
#endif
	} 
	else
	{
		newCoro = coro_handle(new boost_coro);
		if (_autoMakeTimer)
		{
			newCoro->_timerSleep = timeout_trig::create_high_resolution(coroStrand);
		}
		newCoro->_strand = coroStrand;
		newCoro->_mainFunc = mainFunc;
		if (cb) newCoro->_exitCallback.push_back(cb);
		newCoro->_coroPull = new coro_pull_type(boost_coro_run(*newCoro),
			boost::coroutines::attributes(stackSize), coro_stack(&newCoro->_stackTop, &newCoro->_stackSize));
	}
	newCoro->_weakThis = newCoro;
	return newCoro;
}

void boost_coro::async_create( shared_strand coroStrand, const main_func& mainFunc,
	const boost::function<void (coro_handle)>& ch, size_t stackSize )
{
	boost_strand::asyncInvoke<coro_handle>(coroStrand, boost::bind(&local_create, coroStrand, mainFunc, stackSize), ch);
}

void boost_coro::async_create( shared_strand coroStrand, const main_func& mainFunc,
	const boost::function<void (coro_handle)>& ch, const boost::function<void (bool)>& cb, size_t stackSize )
{
	boost_strand::asyncInvoke<coro_handle>(coroStrand, boost::bind(&local_create, coroStrand, mainFunc, cb, stackSize), ch);
}

child_coro_handle::child_coro_param boost_coro::create_child_coro( shared_strand coroStrand, const main_func& mainFunc, size_t stackSize )
{
	assert_enter();

	child_coro_handle::child_coro_param coroHandle;
	if (coroStrand != _strand)
	{//不依赖于同一个strand，采用异步创建
		boost_coro::async_create(coroStrand, mainFunc, _strand->wrap_post(
			boost::bind(&boost_coro::create_coro_handler, shared_from_this(), _1,
			boost::ref(coroHandle._coro), boost::ref(coroHandle._coroIt))
			), stackSize);
		push_yield();//等待创建完成后通知
	}
	else
	{
		coroHandle._coro = boost_coro::local_create(coroStrand, mainFunc, stackSize);
		_childCoroList.push_front(coroHandle._coro);
		coroHandle._coroIt = _childCoroList.begin();
		coroHandle._coro->_parentCoro = shared_from_this();
	}
	DEBUG_OPERATION(coroHandle._isCopy = false);
	return coroHandle;
}

child_coro_handle::child_coro_param boost_coro::create_child_coro(const main_func& mainFunc, size_t stackSize)
{
	return create_child_coro(this_strand(), mainFunc, stackSize);
}

void boost_coro::child_coro_run( child_coro_handle& coroHandle )
{
	assert_enter();
	assert(!coroHandle._quited);
	assert(coroHandle.get_coro());
	assert(coroHandle.get_coro()->parent_coro().get() == this);

	coroHandle._param._coro->notify_start_run();
}

void boost_coro::child_coros_run(const list<boost::shared_ptr<child_coro_handle> >& coroHandles)
{
	assert_enter();
	for (auto it = coroHandles.begin(); it != coroHandles.end(); it++)
	{
		child_coro_run(**it);
	}
}

bool boost_coro::child_coro_quit( child_coro_handle& coroHandle )
{
	assert_enter();
	if (!coroHandle._quited)
	{
		assert(coroHandle.get_coro());
		assert(coroHandle.get_coro()->parent_coro().get() == this);

		if (coroHandle._param._coro->_strand == _strand)
		{
			trig<bool>(boost::bind(&boost_coro::force_quit, coroHandle._param._coro, _1), coroHandle._norQuit);
		} 
		else
		{
			trig<bool>(boost::bind(&boost_coro::notify_force_quit, coroHandle._param._coro, _1), coroHandle._norQuit);
		}
		coroHandle.quited_set();
		coroHandle.cancel_quit_it();
		coroHandle._param._coro.reset();
		_childCoroList.erase(coroHandle._param._coroIt);
	}
	return coroHandle._norQuit;
}

bool boost_coro::child_coros_quit(const list<child_coro_handle::ptr>& coroHandles)
{
	assert_enter();
	bool res = true;
	coro_msg_handle<child_coro_handle::ptr, bool> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = coroHandles.begin(); it != coroHandles.end(); it++)
	{
		assert((*it)->get_coro());
		assert((*it)->get_coro()->parent_coro().get() == this);
		if ((*it)->get_coro()->_strand == _strand)
		{
			(*it)->get_coro()->force_quit(boost::bind(h, *it, _1));
		} 
		else
		{
			(*it)->get_coro()->notify_force_quit(boost::bind(h, *it, _1));
		}
	}
	for (int i = (int)coroHandles.size(); i > 0; i--)
	{
		bool ok = true;
		child_coro_handle::ptr cp;
		pump_msg(cmh, cp, ok);
		res &= ok;
		cp->quited_set();
		cp->cancel_quit_it();
		cp->_param._coro.reset();
		_childCoroList.erase(cp->_param._coroIt);
	}
	close_msg_notify(cmh);
	return res;
}

bool boost_coro::child_coro_wait_quit( child_coro_handle& coroHandle )
{
	assert_enter();
	if (!coroHandle._quited)
	{
		assert(coroHandle.get_coro());
		assert(coroHandle.get_coro()->parent_coro().get() == this);

		coroHandle._norQuit = another_coro_wait_quit(coroHandle._param._coro);
		coroHandle.quited_set();
		coroHandle.cancel_quit_it();
		coroHandle._param._coro.reset();
		_childCoroList.erase(coroHandle._param._coroIt);
	}
	return coroHandle._norQuit;
}

bool boost_coro::child_coros_wait_quit(const list<child_coro_handle::ptr>& coroHandles)
{
	assert_enter();
	bool res = true;
	for (auto it = coroHandles.begin(); it != coroHandles.end(); it++)
	{
		assert((*it)->get_coro()->parent_coro().get() == this);
		res &= child_coro_wait_quit(**it);
	}
	return res;
}

void boost_coro::child_coro_suspend(child_coro_handle& coroHandle)
{
	assert_enter();
	assert(coroHandle.get_coro());
	assert(coroHandle.get_coro()->parent_coro().get() == this);
	if (coroHandle.get_coro()->_strand == _strand)
	{
		trig(boost::bind(&boost_coro::suspend, coroHandle.get_coro(), _1));
	} 
	else
	{
		trig(boost::bind(&boost_coro::notify_suspend, coroHandle.get_coro(), _1));
	}
}

void boost_coro::child_coros_suspend(const list<child_coro_handle::ptr>& coroHandles)
{
	assert_enter();
	coro_msg_handle<> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = coroHandles.begin(); it != coroHandles.end(); it++)
	{
		assert((*it)->get_coro());
		assert((*it)->get_coro()->parent_coro().get() == this);
		if ((*it)->get_coro()->_strand == _strand)
		{
			(*it)->get_coro()->suspend(h);
		} 
		else
		{
			(*it)->get_coro()->notify_suspend(h);
		}
	}
	for (int i = (int)coroHandles.size(); i > 0; i--)
	{
		pump_msg(cmh);
	}
	close_msg_notify(cmh);
}

void boost_coro::child_coro_resume(child_coro_handle& coroHandle)
{
	assert_enter();
	assert(coroHandle.get_coro());
	assert(coroHandle.get_coro()->parent_coro().get() == this);
	if (coroHandle.get_coro()->_strand == _strand)
	{
		trig(boost::bind(&boost_coro::resume, coroHandle.get_coro(), _1));
	}
	else
	{
		trig(boost::bind(&boost_coro::notify_resume, coroHandle.get_coro(), _1));
	}
}

void boost_coro::child_coros_resume(const list<child_coro_handle::ptr>& coroHandles)
{
	assert_enter();
	coro_msg_handle<> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = coroHandles.begin(); it != coroHandles.end(); it++)
	{
		assert((*it)->get_coro());
		assert((*it)->get_coro()->parent_coro().get() == this);
		if ((*it)->get_coro()->_strand == _strand)
		{
			(*it)->get_coro()->resume(h);
		}
		else
		{
			(*it)->get_coro()->notify_resume(h);
		}
	}
	for (int i = (int)coroHandles.size(); i > 0; i--)
	{
		pump_msg(cmh);
	}
	close_msg_notify(cmh);
}

coro_handle boost_coro::child_coro_peel(child_coro_handle& coroHandle)
{
	assert_enter();
	assert(!coroHandle._quited);
	assert(coroHandle.get_coro());
	assert(coroHandle.get_coro()->parent_coro().get() == this);

	coroHandle.quited_set();
	coroHandle.cancel_quit_it();
	coroHandle._norQuit = false;
	_childCoroList.erase(coroHandle._param._coroIt);
	coro_handle r = coroHandle._param._coro;
	coroHandle._param._coro.reset();
	r->_parentCoro.reset();
	return r;
}

bool boost_coro::run_child_coro_complete( shared_strand coroStrand, const main_func& h, size_t stackSize )
{
	assert_enter();
	child_coro_handle coroHandle = create_child_coro(coroStrand, h, stackSize);
	child_coro_run(coroHandle);
	return child_coro_wait_quit(coroHandle);
}

bool boost_coro::run_child_coro_complete(const main_func& h, size_t stackSize)
{
	return run_child_coro_complete(this_strand(), h, stackSize);
}

void boost_coro::open_timer()
{
	assert_enter();
	if (!_timerSleep)
	{
		if (_sharedPoolMem)
		{
			_timerSleep = timeout_trig::create_in_pool(_sharedPoolMem, (BYTE*)_sharedPoolMem.get()+sizeof(boost_coro),
				_strand, timeout_trig::high_resolution_timer);
		}
		else
		{
			_timerSleep = timeout_trig::create_high_resolution(_strand);
		}
	}
}

void boost_coro::sleep( int ms )
{
	assert_enter();
	assert(ms >= 0);
	if (ms)
	{
		assert(_timerSleep);
		_timerSleep->timeOut(ms, boost::bind(&boost_coro::run_one, shared_from_this()));
	}
	else
	{
		_strand->post(boost::bind(&boost_coro::run_one, shared_from_this()));
	}
	push_yield();
}

coro_handle boost_coro::parent_coro()
{
	return _parentCoro.lock();
}

const list<coro_handle>& boost_coro::child_coros()
{
	return _childCoroList;
}

boost_coro::quit_handle boost_coro::regist_quit_handler( const boost::function<void ()>& quitHandler )
{
	assert_enter();
	_quitHandlerList.push_front(quitHandler);//后注册的先执行
	return _quitHandlerList.begin();
}

void boost_coro::cancel_quit_handler( quit_handle rh )
{
	assert_enter();
	_quitHandlerList.erase(rh);
}

boost::function<void ()> boost_coro::begin_trig(async_trig_handle<>& th)
{
	assert_enter();
	th.begin(_coroID);
	auto isClosed = th._ptrClosed;
	return [&, isClosed]()
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th._notify)
			{
				async_trig_post_yield(th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler, shared_from_this(), isClosed, boost::ref(th)));
		}
	};
}

boost::function<void ()> boost_coro::begin_trig(boost::shared_ptr<async_trig_handle<> > th)
{
	assert_enter();
	th->begin(_coroID);
	auto isClosed = th->_ptrClosed;
	return [&, isClosed, th]()
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th->_notify)
			{
				async_trig_post_yield(*th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler_ptr, shared_from_this(), isClosed, th));
		}
	};
}

bool boost_coro::wait_trig(async_trig_handle<>& th, int tm /* = -1 */)
{
	assert(th._coroID == _coroID);
	assert_enter();
	if (!async_trig_push(th, tm))
	{
		return false;
	}
	close_trig(th);
	return true;
}

void boost_coro::close_trig(async_trig_base& th)
{
	DEBUG_OPERATION(if (th._coroID) {assert(th._coroID == _coroID); th._coroID = 0;})
	th.close();
}

void boost_coro::delay_trig(int ms, const boost::function<void ()>& h)
{
	assert_enter();
	assert(_timerSleep);
	_timerSleep->timeOutAgain(ms, h);
}

void boost_coro::delay_trig(int ms, async_trig_handle<>& th)
{
	assert_enter();
	assert(th._ptrClosed);
	assert(_timerSleep);
	_timerSleep->timeOut(ms, boost::bind(&boost_coro::_async_trig_handler, shared_from_this(), 
		th._ptrClosed, boost::ref(th)));
}

void boost_coro::delay_trig(int ms, boost::shared_ptr<async_trig_handle<> > th)
{
	assert_enter();
	assert(th->_ptrClosed);
	assert(_timerSleep);
	_timerSleep->timeOut(ms, boost::bind(&boost_coro::_async_trig_handler_ptr, shared_from_this(), 
		th->_ptrClosed, th));
}

void boost_coro::cancel_delay_trig()
{
	assert_enter();
	assert(_timerSleep);
	_timerSleep->cancel();
}

void boost_coro::trig( const boost::function<void (boost::function<void ()>)>& h )
{
	assert_enter();
#ifdef _DEBUG
	h(wrapped_trig_handler<boost::function<void ()> >(boost::bind(&boost_coro::trig_handler, shared_from_this())));
#else
	h(boost::bind(&boost_coro::trig_handler, shared_from_this()));
#endif
	push_yield();
}

void boost_coro::trig_ret( shared_strand extStrand, const boost::function<void ()>& h )
{
	trig(boost::bind(&boost_strand::asyncInvokeVoid, extStrand, h, _1));
}

boost::function<void()> boost_coro::make_msg_notify(coro_msg_handle<>& cmh)
{
	cmh.begin(_coroID);
	return _strand->wrap_post(boost::bind(&boost_coro::check_run1, shared_from_this(), cmh._ptrClosed, boost::ref(cmh)));
}

boost::function<void()> boost_coro::make_msg_notify(boost::shared_ptr<coro_msg_handle<> > cmh)
{
	cmh->begin(_coroID);
	return _strand->wrap_post(boost::bind(&boost_coro::check_run1_ptr, shared_from_this(), cmh->_ptrClosed, cmh));
}

void boost_coro::close_msg_notify( param_list_base& cmh )
{
	DEBUG_OPERATION(if (cmh._coroID) {assert(cmh._coroID == _coroID); cmh._coroID = 0;})
	cmh.close();
}

bool boost_coro::pump_msg(coro_msg_handle<>& cmh, int tm /* = -1 */)
{
	assert(cmh._coroID == _coroID);
	assert_enter();
	assert(cmh._ptrClosed);
	if (0 == cmh.count)
	{
		cmh._waiting = true;
		if (!pump_msg_push(cmh, tm))
		{
			return false;
		}
	}
	else
	{
		cmh.count--;
	}
	return true;
}

bool boost_coro::pump_msg_push(param_list_base& pm, int tm)
{
	if (tm >= 0)
	{
		pm._hasTm = true;
		delay_trig(tm, boost::bind(&boost_coro::pump_msg_timeout, shared_from_this(), boost::ref(pm)));
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

void boost_coro::pump_msg_timeout(param_list_base& pm)
{
	if (!_quited)
	{
		assert(pm._waiting);
		pm._waiting = false;
		pm._timeout = true;
		pull_yield();
	}
}

void boost_coro::trig_handler()
{
	_strand->post(boost::bind(&boost_coro::run_one, shared_from_this()));
}

bool boost_coro::async_trig_push(async_trig_base& th, int tm)
{
	assert(th._ptrClosed);
	if (!th._notify)
	{
		th._waiting = true;
		if (tm >= 0)
		{
			th._hasTm = true;
			delay_trig(tm, boost::bind(&boost_coro::async_trig_timeout, shared_from_this(), boost::ref(th)));
		}
		push_yield();
		if (th._timeout)
		{
			th._timeout = false;
			th._hasTm = false;
			return false;
		}
	}
	assert(th._notify);
	return true;
}

void boost_coro::async_trig_timeout(async_trig_base& th)
{
	if (!_quited)
	{
		assert(th._waiting);
		th._waiting = false;
		th._timeout = true;
		pull_yield();
	}
}

void boost_coro::async_trig_post_yield(async_trig_base& th)
{
	th._notify = true;
	if (th._waiting)
	{
		th._waiting = false;
		if (th._hasTm)
		{
			th._hasTm = false;
			_timerSleep->cancel();
		}
		_strand->post(boost::bind(&boost_coro::run_one, shared_from_this()));
	} 
}

void boost_coro::async_trig_pull_yield(async_trig_base& th)
{
	th._notify = true;
	if (th._waiting)
	{
		th._waiting = false;
		if (th._hasTm)
		{
			th._hasTm = false;
			_timerSleep->cancel();
		}
		pull_yield();
	} 
}

void boost_coro::_async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<>& th)
{
	assert(_strand->running_in_this_thread());
	if (!_quited && !(*isClosed) && !th._notify)
	{
		async_trig_pull_yield(th);
	}
}

void boost_coro::_async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<> >& th)
{
	_async_trig_handler(isClosed, *th);
}

void boost_coro::create_coro_handler( coro_handle coro, coro_handle& retCoro, list<coro_handle>::iterator& ch )
{
	assert(_strand->running_in_this_thread());
	assert(!_inCoro);
	if (_quited)
	{//父协程已经退出，那么子协程刚创建也必须退出
		coro->notify_force_quit();
		return;
	}

	retCoro = coro;
	_childCoroList.push_front(coro);
	ch = _childCoroList.begin();
	coro->_parentCoro = shared_from_this();
	pull_yield();
}

void boost_coro::check_run1( boost::shared_ptr<bool> isClosed, coro_msg_handle<>& cmh )
{
	if (_quited || (*isClosed)) return;

	if (cmh._waiting)
	{
		cmh._waiting = false;
		if (cmh._hasTm)
		{
			cmh._hasTm = false;
			_timerSleep->cancel();
		}
		assert(0 == cmh.count);
		pull_yield();
	} 
	else
	{
		cmh.count++;
	}
}

void boost_coro::check_run1_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<coro_msg_handle<> >& cmh)
{
	check_run1(isClosed, *cmh);
}

shared_strand boost_coro::this_strand()
{
	return _strand;
}

coro_handle boost_coro::shared_from_this()
{
	return _weakThis.lock();
}

long long boost_coro::this_id()
{
	return _coroID;
}

boost::shared_ptr<timeout_trig> boost_coro::this_timer()
{
	return _timerSleep;
}

size_t boost_coro::yield_count()
{
	assert_enter();
	return _yieldCount;
}

void boost_coro::reset_yield()
{
	assert_enter();
	_yieldCount = 0;
}

void boost_coro::notify_start_run()
{
	_strand->post(boost::bind(&boost_coro::start_run, shared_from_this()));
}

void boost_coro::assert_enter()
{
	assert(_strand->running_in_this_thread());
	assert(!_quited);
	assert(_inCoro);
	assert((size_t)get_sp() >= (size_t)_stackTop-_stackSize+1024);
}

void boost_coro::start_run()
{
	if (_strand->running_in_this_thread())
	{
		assert(!_inCoro);
		assert(!_started);
		if (!_quited && !_started)
		{
			_started = true;
			pull_yield();
		}
	} 
	else
	{
		_strand->post(boost::bind(&boost_coro::start_run, shared_from_this()));
	}
}

void boost_coro::notify_force_quit()
{
	_strand->post(boost::bind(&boost_coro::force_quit, shared_from_this(), boost::function<void (bool)>()));
}

void boost_coro::notify_force_quit(const boost::function<void (bool)>& h)
{
	_strand->post(boost::bind(&boost_coro::force_quit, shared_from_this(), h));
}

void boost_coro::force_quit( const boost::function<void (bool)>& h )
{
	assert(_strand->running_in_this_thread());
	assert(!_inCoro);
	if (!_quited)
	{
		_quited = true;
		_isForce = true;
		if (h) _exitCallback.push_back(h);
		if (!_childCoroList.empty())
		{
			_childOverCount = _childCoroList.size();
			while (!_childCoroList.empty())
			{
				auto cc = _childCoroList.front();
				_childCoroList.pop_front();
				if (cc->_strand == _strand)
				{
					cc->force_quit(boost::bind(&boost_coro::force_quit_cb_handler, shared_from_this()));
				} 
				else
				{
					cc->notify_force_quit(_strand->wrap_post(boost::bind(&boost_coro::force_quit_cb_handler, shared_from_this())));
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

void boost_coro::notify_suspend()
{
	notify_suspend(boost::function<void ()>());
}

void boost_coro::notify_suspend(const boost::function<void ()>& h)
{
	_strand->post(boost::bind(&boost_coro::suspend, shared_from_this(), h));
}

void boost_coro::suspend(const boost::function<void ()>& h)
{
	assert(_strand->running_in_this_thread());
	assert(!_inCoro);

	_suspendResumeQueue.push_back(suspend_resume_option());
	suspend_resume_option& tmp = _suspendResumeQueue.back();
	tmp._isSuspend = true;
	tmp._h = h;
	if (_suspendResumeQueue.size() == 1)
	{
		suspend();
	}
}

void boost_coro::suspend()
{
	assert(_strand->running_in_this_thread());
	assert(!_inCoro);

	assert(!_childSuspendResumeCount);
	if (!_quited)
	{
		if (!_suspended)
		{
			_suspended = true;
			if (_timerSleep)
			{
				_timerSleep->suspend();
			}
			if (!_childCoroList.empty())
			{
				_childSuspendResumeCount = _childCoroList.size();
				for (auto it = _childCoroList.begin(); it != _childCoroList.end(); it++)
				{
					if ((*it)->_strand == _strand)
					{
						(*it)->suspend(boost::bind(&boost_coro::child_suspend_cb_handler, shared_from_this()));
					}
					else
					{
						(*it)->notify_suspend(_strand->wrap_post(boost::bind(&boost_coro::child_suspend_cb_handler, shared_from_this())));
					}
				}
				return;
			} 
		}
	}
	_childSuspendResumeCount = 1;
	child_suspend_cb_handler();
}

void boost_coro::notify_resume()
{
	notify_resume(boost::function<void ()>());
}

void boost_coro::notify_resume(const boost::function<void ()>& h)
{
	_strand->post(boost::bind(&boost_coro::resume, shared_from_this(), h));
}

void boost_coro::resume(const boost::function<void ()>& h)
{
	assert(_strand->running_in_this_thread());
	assert(!_inCoro);

	_suspendResumeQueue.push_back(suspend_resume_option());
	suspend_resume_option& tmp = _suspendResumeQueue.back();
	tmp._isSuspend = false;
	tmp._h = h;
	if (_suspendResumeQueue.size() == 1)
	{
		resume();
	}
}

void boost_coro::resume()
{
	assert(_strand->running_in_this_thread());
	assert(!_inCoro);

	assert(!_childSuspendResumeCount);
	if (!_quited)
	{
		if (_suspended)
		{
			_suspended = false;
			if (_timerSleep)
			{
				_timerSleep->resume();
			}
			if (!_childCoroList.empty())
			{
				_childSuspendResumeCount = _childCoroList.size();
				for (auto it = _childCoroList.begin(); it != _childCoroList.end(); it++)
				{
					if ((*it)->_strand == _strand)
					{
						(*it)->resume(boost::bind(&boost_coro::child_resume_cb_handler, shared_from_this()));
					} 
					else
					{
						(*it)->notify_resume(_strand->wrap_post(boost::bind(&boost_coro::child_resume_cb_handler, shared_from_this())));
					}
				}
				return;
			} 
		}
	}
	_childSuspendResumeCount = 1;
	child_resume_cb_handler();
}

void boost_coro::switch_pause_play()
{
	switch_pause_play(boost::function<void (bool isPaused)>());
}

void boost_coro::switch_pause_play(const boost::function<void (bool isPaused)>& h)
{
	coro_handle _this = shared_from_this();
	_strand->post(([_this, h]()
	{
		assert(_this->_strand->running_in_this_thread());
		if (!_this->_quited)
		{
			if (_this->_suspended)
			{
				if (h)
				{
					_this->resume(boost::bind(h, false));
				} 
				else
				{
					_this->resume(boost::function<void ()>());
				}
			} 
			else
			{
				if (h)
				{
					_this->suspend(boost::bind(h, true));
				} 
				else
				{
					_this->suspend(boost::function<void ()>());
				}
			}
		}
		else if (h)
		{
			CHECK_EXCEPTION1(h, true);
		}
	}));
}

bool boost_coro::outside_wait_quit()
{
	assert(!_strand->in_this_ios());
	bool rt = false;
	boost::condition_variable conVar;
	boost::mutex mutex;
	boost::unique_lock<boost::mutex> ul(mutex);
	_strand->post(boost::bind(&boost_coro::outside_wait_quit_proxy, shared_from_this(), &conVar, &mutex, &rt));
	conVar.wait(ul);
	return rt;
}

void boost_coro::append_quit_callback(const boost::function<void (bool)>& h)
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
		_strand->post(boost::bind(&boost_coro::append_quit_callback, shared_from_this(), h));
	}
}

void boost_coro::another_coros_start_run(const list<coro_handle>& anotherCoros)
{
	assert_enter();
	for (auto it = anotherCoros.begin(); it != anotherCoros.end(); it++)
	{
		(*it)->notify_start_run();
	}
}

bool boost_coro::another_coro_force_quit( coro_handle anotherCoro )
{
	assert_enter();
	assert(anotherCoro);
	bool ok = false;
	trig<bool>(boost::bind(&boost_coro::notify_force_quit, anotherCoro, _1), ok);
	return ok;
}

void boost_coro::another_coros_force_quit(const list<coro_handle>& anotherCoros)
{
	assert_enter();
	coro_msg_handle<bool> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherCoros.begin(); it != anotherCoros.end(); it++)
	{
		(*it)->notify_force_quit(h);
	}
	for (int i = (int)anotherCoros.size(); i > 0; i--)
	{
		bool ok;
		pump_msg(cmh, ok);
	}
	close_msg_notify(cmh);
}

bool boost_coro::another_coro_wait_quit( coro_handle anotherCoro )
{
	assert_enter();
	assert(anotherCoro);
	bool ok = false;
	trig<bool>(boost::bind(&boost_coro::append_quit_callback, anotherCoro, _1), ok);
	return ok;
}

void boost_coro::another_coros_wait_quit(const list<coro_handle>& anotherCoros)
{
	assert_enter();
	coro_msg_handle<bool> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherCoros.begin(); it != anotherCoros.end(); it++)
	{
		(*it)->append_quit_callback(h);
	}
	for (int i = (int)anotherCoros.size(); i > 0; i--)
	{
		bool ok;
		pump_msg(cmh, ok);
	}
	close_msg_notify(cmh);
}

void boost_coro::another_coro_suspend(coro_handle anotherCoro)
{
	assert_enter();
	assert(anotherCoro);
	trig(boost::bind(&boost_coro::notify_suspend, anotherCoro, _1));
}

void boost_coro::another_coros_suspend(const list<coro_handle>& anotherCoros)
{
	assert_enter();
	coro_msg_handle<> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherCoros.begin(); it != anotherCoros.end(); it++)
	{
		(*it)->notify_suspend(h);
	}
	for (int i = (int)anotherCoros.size(); i > 0; i--)
	{
		pump_msg(cmh);
	}
	close_msg_notify(cmh);
}

void boost_coro::another_coro_resume(coro_handle anotherCoro)
{
	assert_enter();
	assert(anotherCoro);
	trig(boost::bind(&boost_coro::notify_resume, anotherCoro, _1));
}

void boost_coro::another_coros_resume(const list<coro_handle>& anotherCoros)
{
	assert_enter();
	coro_msg_handle<> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherCoros.begin(); it != anotherCoros.end(); it++)
	{
		(*it)->notify_resume(h);
	}
	for (int i = (int)anotherCoros.size(); i > 0; i--)
	{
		pump_msg(cmh);
	}
	close_msg_notify(cmh);
}

bool boost_coro::another_coro_switch(coro_handle anotherCoro)
{
	assert_enter();
	assert(anotherCoro);
	bool isPause = true;
	trig<bool>(boost::bind(&boost_coro::switch_pause_play, anotherCoro, _1), isPause);
	return isPause;
}

bool boost_coro::another_coros_switch(const list<coro_handle>& anotherCoros)
{
	assert_enter();
	bool isPause = true;
	coro_msg_handle<bool> cmh;
	auto h = make_msg_notify(cmh);
	for (auto it = anotherCoros.begin(); it != anotherCoros.end(); it++)
	{
		(*it)->switch_pause_play(h);
	}
	for (int i = (int)anotherCoros.size(); i > 0; i--)
	{
		isPause &= pump_msg(cmh);
	}
	close_msg_notify(cmh);
	return isPause;
}

void boost_coro::run_one()
{
	assert(_strand->running_in_this_thread());
	assert(!_inCoro);
	if (!_quited)
	{
		pull_yield();
	}
}

void boost_coro::pull_yield()
{
	assert(!_quited);
	if (!_suspended)
	{
		(*(coro_pull_type*)_coroPull)();
	}
	else
	{
		assert(!_hasNotify);
		_hasNotify = true;
	}
}

void boost_coro::push_yield()
{
	assert_enter();
	_yieldCount++;
	_inCoro = false;
	(*(coro_push_type*)_coroPush)();
	if (_quited)
	{
		throw coro_force_quit();
	}
	_inCoro = true;
}

void boost_coro::force_quit_cb_handler()
{
	assert(_quited);
	assert(_isForce);
	assert(!_inCoro);
	assert((int)_childOverCount > 0);
	if (0 == --_childOverCount)
	{
		exit_callback();
	}
}

void boost_coro::child_suspend_cb_handler()
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
				_strand->post(boost::bind(&boost_coro::resume, shared_from_this()));
				return;
			}
		}
	}
}

void boost_coro::child_resume_cb_handler()
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
				_strand->post(boost::bind(&boost_coro::suspend, shared_from_this()));
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

void boost_coro::exit_callback()
{
	assert(_quited);
	assert(!_childOverCount);
	while (!_quitHandlerList.empty())
	{
		CHECK_EXCEPTION(_quitHandlerList.front());
		_quitHandlerList.pop_front();
	}
	(*(coro_pull_type*)_coroPull)();
}

void boost_coro::outside_wait_quit_proxy( boost::condition_variable* conVar, boost::mutex* mutex, bool* rt )
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
		_exitCallback.push_back(boost::bind(&boost_coro::outside_wait_quit_handler, shared_from_this(), conVar, mutex, rt, _1));
	}
}

void boost_coro::outside_wait_quit_handler( boost::condition_variable* conVar, boost::mutex* mutex, bool* rt, bool ok )
{
	*rt = ok;
	boost::lock_guard<boost::mutex> lg(*mutex);
	conVar->notify_one();
}

void boost_coro::enable_stack_pool()
{
	assert(0 == _coroIDCount);
	coro_stack_pool::enable();
	coro_buff_pool::enable();
}

void boost_coro::disable_auto_make_timer()
{
	assert(0 == _coroIDCount);
	_autoMakeTimer = false;
}

void boost_coro::check_stack()
{
#ifdef CHECK_CORO_STACK
	if ((size_t)get_sp() < (size_t)_stackTop-_stackSize+1024)
	{
		assert(false);
		throw boost::shared_ptr<string>(new string("协程堆栈异常"));
	}
#endif
}

size_t boost_coro::stack_free_space()
{
	int s = (int)((size_t)get_sp() - ((size_t)_stackTop-_stackSize+1024));
	if (s < 0)
	{
		return 0;
	}
	return (size_t)s;
}