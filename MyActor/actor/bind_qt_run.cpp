#include "bind_qt_run.h"

#ifdef ENABLE_QT_UI
bind_qt_run_base::bind_qt_run_base()
:_isClosed(false), _tasksQueue(16)
{
	_threadID = boost::this_thread::get_id();
}

bind_qt_run_base::~bind_qt_run_base()
{
	assert(boost::this_thread::get_id() == _threadID);
	assert(_isClosed);
	assert(_tasksQueue.empty());
}

boost::thread::id bind_qt_run_base::thread_id()
{
	assert(boost::thread::id() != _threadID);
	return _threadID;
}

void bind_qt_run_base::post(const std::function<void()>& h)
{
	{
		boost::shared_lock<boost::shared_mutex> sl(_postMutex);
		if (!_isClosed)
		{
			_mutex.lock();
			_tasksQueue.push_back(h);
			_mutex.unlock();
			postTaskEvent();
			return;
		}
	}
	assert(false);
}

void bind_qt_run_base::post(std::function<void()>&& h)
{
	{
		boost::shared_lock<boost::shared_mutex> sl(_postMutex);
		if (!_isClosed)
		{
			_mutex.lock();
			_tasksQueue.push_back(std::move(h));
			_mutex.unlock();
			sl.unlock();
			postTaskEvent();
			return;
		}
	}
	assert(false);
}

void bind_qt_run_base::post_queue_size(size_t fixedSize)
{
	assert(boost::this_thread::get_id() == _threadID);
	_mutex.lock();
	_tasksQueue.expand_fixed(fixedSize);
	_mutex.unlock();
}

void bind_qt_run_base::runOneTask()
{
	_mutex.lock();
	assert(!_tasksQueue.empty());
	auto h = std::move(_tasksQueue.front());
	_tasksQueue.pop_front();
	_mutex.unlock();
	assert(h);
	h();
}

void bind_qt_run_base::notifyUiClosed()
{
	assert(boost::this_thread::get_id() == _threadID);
	{
		boost::unique_lock<boost::shared_mutex> sl(_postMutex);
		_isClosed = true;
	}
	while (!_tasksQueue.empty())
	{
		bind_qt_run_base::runOneTask();
	}
}

void bind_qt_run_base::userEvent(QEvent* e)
{

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