#ifdef DISABLE_BOOST_TIMER
#include "waitable_timer.h"
#include "scattered.h"
#ifdef WIN32
#include <Windows.h>

WaitableTimer_::WaitableTimer_()
:_eventsQueue(1024), _exited(false), _extMaxTick(0), _extFinishTime(-1),
_timerHandle(CreateWaitableTimer(NULL, FALSE, NULL))
{
	run_thread th([this] { timerThread(); });
	_timerThread.swap(th);
}

WaitableTimer_::~WaitableTimer_()
{
	{
		std::lock_guard<std::mutex> lg(_ctrlMutex);
		assert(_eventsQueue.empty());
		_exited = true;
		LARGE_INTEGER sleepTime;
		sleepTime.QuadPart = 0;
		SetWaitableTimer(_timerHandle, &sleepTime, 0, NULL, NULL, FALSE);
	}
	_timerThread.join();
	CloseHandle(_timerHandle);
}

void WaitableTimer_::appendEvent(long long abs, long long rel, WaitableTimerEvent_* h)
{
	assert(h->_timerHandle._null);
	h->_timerHandle._null = false;
	std::lock_guard<std::mutex> lg(_ctrlMutex);
	if (abs >= _extMaxTick)
	{
		_extMaxTick = abs;
		h->_timerHandle._queueNode = _eventsQueue.insert(_eventsQueue.end(), std::make_pair(abs, h));
	}
	else
	{
		h->_timerHandle._queueNode = _eventsQueue.insert(std::make_pair(abs, h));
	}
	if ((unsigned long long)abs < (unsigned long long)_extFinishTime)
	{
		_extFinishTime = abs;
		LARGE_INTEGER sleepTime;
		sleepTime.QuadPart = -(LONGLONG)(rel * 10);
		SetWaitableTimer(_timerHandle, &sleepTime, 0, NULL, NULL, FALSE);
	}
}

void WaitableTimer_::timerThread()
{
	run_thread::set_current_thread_name("waitable timer thread");
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	while (true)
	{
		if (WAIT_OBJECT_0 == WaitForSingleObject(_timerHandle, INFINITE) && !_exited)
		{
			long long ct = get_tick_us();
			std::lock_guard<std::mutex> lg(_ctrlMutex);
			_extFinishTime = -1;
			while (!_eventsQueue.empty())
			{
				handler_queue::iterator iter = _eventsQueue.begin();
				if (iter->first > ct)
				{
					_extFinishTime = iter->first;
					LARGE_INTEGER sleepTime;
					sleepTime.QuadPart = -(LONGLONG)((iter->first - ct) * 10);
					SetWaitableTimer(_timerHandle, &sleepTime, 0, NULL, NULL, FALSE);
					break;
				} 
				else
				{
					iter->second->eventHandler();
					_eventsQueue.erase(iter);
				}
			}
		} 
		else
		{
			break;
		}
	}
}
#elif __linux__
#include <sys/timerfd.h>
#include <pthread.h>

WaitableTimer_::WaitableTimer_()
:_eventsQueue(1024), _exited(false), _extMaxTick(0), _extFinishTime(-1),
_timerFd(timerfd_create(CLOCK_MONOTONIC, 0))
{
	run_thread th([this] { timerThread(); });
	_timerThread.swap(th);
}

WaitableTimer_::~WaitableTimer_()
{
	{
		std::lock_guard<std::mutex> lg(_ctrlMutex);
		assert(_eventsQueue.empty());
		_exited = true;
		struct itimerspec newValue = { { 0, 0 }, { 0, 1 } };
		timerfd_settime(_timerFd, TFD_TIMER_ABSTIME, &newValue, NULL);
	}
	_timerThread.join();
	close(_timerFd);
}

void WaitableTimer_::appendEvent(long long abs, long long rel, WaitableTimerEvent_* h)
{
	assert(h->_timerHandle._null);
	h->_timerHandle._null = false;
	std::lock_guard<std::mutex> lg(_ctrlMutex);
	if (abs >= _extMaxTick)
	{
		_extMaxTick = abs;
		h->_timerHandle._queueNode = _eventsQueue.insert(_eventsQueue.end(), std::make_pair(abs, h));
	}
	else
	{
		h->_timerHandle._queueNode = _eventsQueue.insert(std::make_pair(abs, h));
	}
	if ((unsigned long long)abs < (unsigned long long)_extFinishTime)
	{
		_extFinishTime = abs;
		struct itimerspec newValue;
		newValue.it_interval = { 0, 0 };
		newValue.it_value.tv_sec = (__time_t)(abs / 1000000);
		newValue.it_value.tv_nsec = (long)(abs % 1000000) * 1000;
		timerfd_settime(_timerFd, TFD_TIMER_ABSTIME, &newValue, NULL);
	}
}

void WaitableTimer_::timerThread()
{
	run_thread::set_current_thread_name("waitable timer thread");
	pthread_attr_t threadAttr;
	struct sched_param pm = { 83 };
	pthread_attr_init(&threadAttr);
	pthread_attr_setschedpolicy(&threadAttr, SCHED_FIFO);
	pthread_attr_setschedparam(&threadAttr, &pm);
	long long exp = 0;
	while (true)
	{
		if (sizeof(exp) == read(_timerFd, &exp, sizeof(exp)) && !_exited)
		{
			long long ct = get_tick_us();
			std::lock_guard<std::mutex> lg(_ctrlMutex);
			_extFinishTime = -1;
			while (!_eventsQueue.empty())
			{
				handler_queue::iterator iter = _eventsQueue.begin();
				if (iter->first > ct)
				{
					_extFinishTime = iter->first;
					struct itimerspec newValue;
					newValue.it_interval = { 0, 0 };
					newValue.it_value.tv_sec = (__time_t)(_extFinishTime / 1000000);
					newValue.it_value.tv_nsec = (long)(_extFinishTime % 1000000) * 1000;
					timerfd_settime(_timerFd, TFD_TIMER_ABSTIME, &newValue, NULL);
					break;
				}
				else
				{
					iter->second->eventHandler();
					_eventsQueue.erase(iter);
				}
			}
		}
		else
		{
			break;
		}
	}
	pthread_attr_destroy(&threadAttr);
}
#endif

void WaitableTimer_::removeEvent(timer_handle& th)
{
	std::lock_guard<std::mutex> lg(_ctrlMutex);
	if (!th._null)
	{
		th._null = true;
		auto itNode = th._queueNode;
		if (_eventsQueue.size() == 1)
		{
			_extMaxTick = 0;
			_extFinishTime = -1;
			_eventsQueue.erase(itNode);
		}
		else if (itNode->first == _extMaxTick)
		{
			_eventsQueue.erase(itNode++);
			if (_eventsQueue.end() == itNode)
			{
				itNode--;
			}
			_extMaxTick = itNode->first;
		}
		else
		{
			_eventsQueue.erase(itNode);
		}
	}
}
//////////////////////////////////////////////////////////////////////////

WaitableTimerEvent_::WaitableTimerEvent_(io_engine& ios, TimerBoostCompletedEventFace_* timerBoost)
:_ios(ios), _timerBoost(timerBoost), _tcId(-1), _triged(true) {}

WaitableTimerEvent_::~WaitableTimerEvent_()
{
	assert(_triged);
}

void WaitableTimerEvent_::eventHandler()
{
	_timerHandle.reset();
	_triged = true;
	_timerBoost->post_event(_tcId);
}

void WaitableTimerEvent_::cancel(boost::system::error_code& ec)
{
	ec.clear();
	_ios._waitableTimer->removeEvent(_timerHandle);
	if (!_triged)
	{
		_triged = true;
		_timerBoost->post_event(_tcId);
	}
}

void WaitableTimerEvent_::async_wait(long long abs, long long rel, int tc)
{
	assert(_triged);
	_triged = false;
	_tcId = tc;
	_ios._waitableTimer->appendEvent(abs, rel, this);
}

#endif