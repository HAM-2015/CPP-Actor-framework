#ifdef DISABLE_BOOST_TIMER
#ifdef _MSC_VER
#include "waitable_timer.h"
#include "scattered.h"
#include <Windows.h>

WaitableTimer_::WaitableTimer_()
:_eventsQueue(1024)
{
	_exit = false;
	_extMaxTick = 0;
	_extFinishTime = -1;
	_timerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
	boost::thread th(&WaitableTimer_::timerThread, this);
	_timerThread.swap(th);
}

WaitableTimer_::~WaitableTimer_()
{
	{
		std::lock_guard<std::mutex> lg(_ctrlMutex);
		assert(_eventsQueue.empty());
		_exit = true;
		LARGE_INTEGER sleepTime;
		sleepTime.QuadPart = 0;
		SetWaitableTimer(_timerHandle, &sleepTime, 0, NULL, NULL, FALSE);
	}
	_timerThread.join();
	CloseHandle(_timerHandle);
}

void WaitableTimer_::appendEvent(long long us, WaitableTimerEvent_* h)
{
	assert(us > 0);
	assert(h->_timerHandle._null);
	h->_timerHandle._null = false;
	unsigned long long et = us + get_tick_us();
	std::lock_guard<std::mutex> lg(_ctrlMutex);
	if (et >= _extMaxTick)
	{
		_extMaxTick = et;
		h->_timerHandle._queueNode = _eventsQueue.insert(_eventsQueue.end(), std::make_pair(et, h));
	}
	else
	{
		h->_timerHandle._queueNode = _eventsQueue.insert(std::make_pair(et, h));
	}
	if (et < _extFinishTime)
	{
		_extFinishTime = et;
		LARGE_INTEGER sleepTime;
		sleepTime.QuadPart = us * 10;
		sleepTime.QuadPart = -sleepTime.QuadPart;
		SetWaitableTimer(_timerHandle, &sleepTime, 0, NULL, NULL, FALSE);
	}
}

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

void WaitableTimer_::timerThread()
{
	while (true)
	{
		if (WAIT_OBJECT_0 == WaitForSingleObject(_timerHandle, INFINITE) && !_exit)
		{
			unsigned long long nt = get_tick_us();
			std::lock_guard<std::mutex> lg(_ctrlMutex);
			_extFinishTime = -1;
			while (!_eventsQueue.empty())
			{
				auto iter = _eventsQueue.begin();
				if (iter->first > nt)
				{
					_extFinishTime = iter->first;
					LARGE_INTEGER sleepTime;
					sleepTime.QuadPart = nt - iter->first;
					sleepTime.QuadPart *= 10;
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
//////////////////////////////////////////////////////////////////////////

WaitableTimerEvent_::WaitableTimerEvent_(io_engine& ios, WaitableTimer_* timer)
:_ios(ios), _timer(timer), _handler(NULL)
{

}

WaitableTimerEvent_::~WaitableTimerEvent_()
{
	assert(!_handler);
}

void WaitableTimerEvent_::eventHandler()
{
	_timerHandle.reset();
	wrap_base* cb = _handler;
	_handler = NULL;
	cb->invoke(_reuMem);
}

void WaitableTimerEvent_::cancel(boost::system::error_code& ec)
{
	ec.clear();
	_timer->removeEvent(_timerHandle);
	if (_handler)
	{
		wrap_base* cb = _handler;
		_handler = NULL;
		cb->invoke_err(_reuMem);
	}
}
#elif __GNUG__
#error "do not define DISABLE_BOOST_TIMER"
#endif
#endif