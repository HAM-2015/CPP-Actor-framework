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

void bind_qt_run_base::ui_tls::push_stack(bind_qt_run_base* s)
{
	assert(s);
	ui_tls* uiTls = (ui_tls*)_tls.get_space();
	assert(uiTls);
	uiTls->_uiStack.push_front(s);
}

bind_qt_run_base* bind_qt_run_base::ui_tls::pop_stack()
{
	ui_tls* uiTls = (ui_tls*)_tls.get_space();
	assert(uiTls);
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
:_isClosed(false), _waitClose(false), _eventLoop(NULL), _waitCount(0), _tasksQueue(16)
{
	_threadID = boost::this_thread::get_id();
#ifdef ENABLE_QT_ACTOR
	ui_tls::init();
#endif
}

bind_qt_run_base::~bind_qt_run_base()
{
	assert(run_in_ui_thread());
	assert(_isClosed);
	assert(0 == _waitCount);
	assert(!_waitClose);
	assert(!_eventLoop);
	assert(_tasksQueue.empty());
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

void bind_qt_run_base::post_queue_size(size_t fixedSize)
{
	assert(run_in_ui_thread());
	_mutex.lock();
	_tasksQueue.expand_fixed(fixedSize);
	_mutex.unlock();
}

size_t bind_qt_run_base::task_number()
{
	return _tasksQueue.size();
}

std::function<void()> bind_qt_run_base::wrap_check_close()
{
	assert(run_in_ui_thread());
	_waitCount++;
	return wrap([this]
	{
		check_close();
	});
}

void bind_qt_run_base::runOneTask()
{
	_mutex.lock();
	assert(!_tasksQueue.empty());
	wrap_handler_face* h = _tasksQueue.front();
	h->running_now();
	_tasksQueue.pop_front();
	_mutex.unlock();

#ifdef ENABLE_QT_ACTOR
	ui_tls::push_stack(this);
#endif
	h->invoke();
	_reuMem.deallocate(h);
#ifdef ENABLE_QT_ACTOR
	bind_qt_run_base* r = ui_tls::pop_stack();
	assert(this == r);
#endif
}

void bind_qt_run_base::ui_closed()
{
	assert(run_in_ui_thread());
	{
		boost::unique_lock<boost::shared_mutex> sl(_postMutex);
		_isClosed = true;
	}
	while (!_tasksQueue.empty())
	{
		bind_qt_run_base::runOneTask();
	}
}

void bind_qt_run_base::enter_wait_close()
{
	assert(run_in_ui_thread());
	assert(!_waitClose);
	_waitClose = true;
	if (_waitCount)
	{
		enter_loop();
	} 
	else
	{
		close_now();
	}
	//assert(_isClosed);
	_waitClose = false;
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

#ifdef ENABLE_QT_ACTOR
actor_handle bind_qt_run_base::create_qt_actor(const my_actor::main_func& mainFunc, size_t stackSize /*= DEFAULT_STACKSIZE*/)
{
	assert(!_isClosed);
	assert(!!_qtStrand);
	return my_actor::create(_qtStrand, mainFunc, stackSize);
}

actor_handle bind_qt_run_base::create_qt_actor(my_actor::main_func&& mainFunc, size_t stackSize /*= DEFAULT_STACKSIZE*/)
{
	assert(!_isClosed);
	assert(!!_qtStrand);
	return my_actor::create(_qtStrand, std::move(mainFunc), stackSize);
}

void bind_qt_run_base::start_qt_strand(io_engine& ios)
{
	assert(run_in_ui_thread());
	_qtStrand = qt_strand::create(ios, this);
}
#endif
#endif