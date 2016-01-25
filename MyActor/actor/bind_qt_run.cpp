#include "bind_qt_run.h"

#ifdef ENABLE_QT_UI

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

bind_qt_run_base::bind_qt_run_base()
:_isClosed(false), _waitClose(false), _eventLoop(NULL), _waitCount(0), _tasksQueue(16)
{
	_threadID = boost::this_thread::get_id();
}

bind_qt_run_base::~bind_qt_run_base()
{
	assert(run_in_ui_thread());
	assert(_isClosed);
	assert(0 == _waitCount);
	assert(_tasksQueue.empty());
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

void bind_qt_run_base::post_queue_size(size_t fixedSize)
{
	assert(run_in_ui_thread());
	_mutex.lock();
	_tasksQueue.expand_fixed(fixedSize);
	_mutex.unlock();
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
	_tasksQueue.pop_front();
	_mutex.unlock();
	h->invoke(_reuMem);
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
actor_handle bind_qt_run_base::create_qt_actor(io_engine& ios, const my_actor::main_func& mainFunc, size_t stackSize /*= DEFAULT_STACKSIZE*/)
{
	assert(!_isClosed);
	return my_actor::create(make_qt_strand(ios), mainFunc, stackSize);
}

actor_handle bind_qt_run_base::create_qt_actor(const my_actor::main_func& mainFunc, size_t stackSize /*= DEFAULT_STACKSIZE*/)
{
	assert(!_isClosed);
	return my_actor::create(make_qt_strand(), mainFunc, stackSize);
}

shared_qt_strand bind_qt_run_base::make_qt_strand()
{
	return qt_strand::create(this);
}

shared_qt_strand bind_qt_run_base::make_qt_strand(io_engine& ios)
{
	return qt_strand::create(ios, this);
}
#endif
#endif