#include "bind_qt_run.h"

#ifdef ENABLE_QT_UI

#ifdef ENABLE_QT_ACTOR
bind_qt_run_base::ui_tls::ui_tls()
:_uiStack(64)
{
	_count = 0;
}

bind_qt_run_base::ui_tls::~ui_tls()
{
	assert(0 == _count);
}

void bind_qt_run_base::ui_tls::init()
{
	ui_tls* uiTls = (ui_tls*)_tls.get_space();
	if (!uiTls)
	{
		uiTls = new ui_tls();
		_tls.set_space((void**)uiTls);
	}
	uiTls->_count++;
}

void bind_qt_run_base::ui_tls::reset()
{
	ui_tls* uiTls = (ui_tls*)_tls.get_space();
	if (uiTls && 0 == --uiTls->_count)
	{
		_tls.set_space(NULL);
		delete uiTls;
	}
}

bind_qt_run_base::ui_tls* bind_qt_run_base::ui_tls::push_stack(bind_qt_run_base* s)
{
	assert(s);
	ui_tls* uiTls = (ui_tls*)_tls.get_space();
	assert(uiTls);
	uiTls->_uiStack.push_front(s);
	return uiTls;
}

bind_qt_run_base* bind_qt_run_base::ui_tls::pop_stack()
{
	return pop_stack((ui_tls*)_tls.get_space());
}

bind_qt_run_base* bind_qt_run_base::ui_tls::pop_stack(ui_tls* uiTls)
{
	assert(uiTls && uiTls == (ui_tls*)_tls.get_space());
	assert(!uiTls->_uiStack.empty());
	bind_qt_run_base* r = uiTls->_uiStack.front();
	uiTls->_uiStack.pop_front();
	return r;
}

bool bind_qt_run_base::ui_tls::running_in_this_thread(bind_qt_run_base* s)
{
	ui_tls* uiTls = (ui_tls*)_tls.get_space();
	if (uiTls)
	{
		for (bind_qt_run_base* const ele : uiTls->_uiStack)
		{
			if (ele == s)
			{
				return true;
			}
		}
	}
	return false;
}

tls_space bind_qt_run_base::ui_tls::_tls;
#endif

//////////////////////////////////////////////////////////////////////////

mem_alloc_mt<bind_qt_run_base::task_event> bind_qt_run_base::task_event::_taskAlloc(256);

void* bind_qt_run_base::task_event::operator new(size_t s)
{
	assert(sizeof(task_event) == s);
	return _taskAlloc.allocate();
}

void bind_qt_run_base::task_event::operator delete(void* p)
{
	_taskAlloc.deallocate(p);
}
//////////////////////////////////////////////////////////////////////////

bind_qt_run_base::bind_qt_run_base()
:_waitClose(false), _eventLoop(NULL), _waitCount(0), _inCloseScope(false)
{
	DEBUG_OPERATION(_taskCount = 0);
	_threadID = boost::this_thread::get_id();
#ifdef ENABLE_QT_ACTOR
	ui_tls::init();
#endif
}

bind_qt_run_base::~bind_qt_run_base()
{
	assert(run_in_ui_thread());
	assert(0 == _waitCount);
	assert(0 == _taskCount);
	assert(!_waitClose);
	assert(!_eventLoop);
#ifdef ENABLE_QT_ACTOR
	_qtStrand.reset();
	ui_tls::reset();
#endif
}

boost::thread::id bind_qt_run_base::thread_id()
{
	assert(boost::thread::id() != _threadID);
	return _threadID;
}

bool bind_qt_run_base::run_in_ui_thread()
{
	return boost::this_thread::get_id() == _threadID;
}

bool bind_qt_run_base::running_in_this_thread()
{
#ifdef ENABLE_QT_ACTOR
	return ui_tls::running_in_this_thread(this);
#else
	return run_in_ui_thread();
#endif
}

bool bind_qt_run_base::is_wait_close()
{
	assert(run_in_ui_thread());
	return _waitClose;
}

bool bind_qt_run_base::wait_close_reached()
{
	assert(run_in_ui_thread());
	return 0 == _waitCount;
}

bool bind_qt_run_base::inside_wait_close_loop()
{
	assert(run_in_ui_thread());
	return NULL != _eventLoop;
}

bool bind_qt_run_base::in_close_scope()
{
	return _inCloseScope;
}

void bind_qt_run_base::set_in_close_scope_sign(bool b)
{
	_inCloseScope = b;
}

std::function<void()> bind_qt_run_base::wrap_check_close()
{
	assert(run_in_ui_thread());
	_waitCount++;
	return std::function<void()>(wrap([this]
	{
		check_close();
#ifdef _MSC_VER
	}), reusable_alloc<void, reusable_mem_mt<>>(_reuMem));
#elif __GNUG__
	}));
#endif
}

void bind_qt_run_base::run_one_task(wrap_handler_face* h)
{
#ifdef ENABLE_QT_ACTOR
	ui_tls* uiTls = ui_tls::push_stack(this);
#endif
	h->invoke();
	_reuMem.deallocate(h);
#ifdef ENABLE_QT_ACTOR
	bind_qt_run_base* r = ui_tls::pop_stack(uiTls);
	assert(this == r);
#endif
}

void bind_qt_run_base::enter_wait_close()
{
	assert(run_in_ui_thread());
	assert(!_waitClose);
	if (_waitCount)
	{
		_waitClose = true;
		enter_loop();
		_waitClose = false;
	}
}

void bind_qt_run_base::check_close()
{
	assert(_waitCount > 0);
	_waitCount--;
	if (_waitClose && 0 == _waitCount)
	{
		close_now();
	}
}

void bind_qt_run_base::ui_yield(my_actor* host)
{
	host->trig_guard([this](trig_once_notifer<>&& cb)
	{
		post_task_event(make_wrap_handler(_reuMem, std::bind([](const trig_once_notifer<>& cb)
		{
			cb();
		}, std::move(cb))));
	});
}

#ifdef ENABLE_QT_ACTOR
actor_handle bind_qt_run_base::create_qt_actor(const my_actor::main_func& mainFunc, size_t stackSize /*= QT_UI_ACTOR_STACK_SIZE*/)
{
	assert(!!_qtStrand);
	return my_actor::create(_qtStrand, mainFunc, stackSize);
}

actor_handle bind_qt_run_base::create_qt_actor(my_actor::main_func&& mainFunc, size_t stackSize /*= QT_UI_ACTOR_STACK_SIZE*/)
{
	assert(!!_qtStrand);
	return my_actor::create(_qtStrand, std::move(mainFunc), stackSize);
}

child_actor_handle bind_qt_run_base::create_qt_child_actor(my_actor* host, const my_actor::main_func& mainFunc, size_t stackSize /*= QT_UI_ACTOR_STACK_SIZE*/)
{
	assert(!!_qtStrand);
	return host->create_child_actor(_qtStrand, mainFunc, stackSize);
}

child_actor_handle bind_qt_run_base::create_qt_child_actor(my_actor* host, my_actor::main_func&& mainFunc, size_t stackSize /*= QT_UI_ACTOR_STACK_SIZE*/)
{
	assert(!!_qtStrand);
	return host->create_child_actor(_qtStrand, std::move(mainFunc), stackSize);
}

void bind_qt_run_base::start_qt_strand(io_engine& ios)
{
	assert(run_in_ui_thread());
	_qtStrand = qt_strand::create(ios, this);
}

const shared_qt_strand& bind_qt_run_base::ui_strand()
{
	assert(!!_qtStrand);
	return _qtStrand;
}

#endif
#endif