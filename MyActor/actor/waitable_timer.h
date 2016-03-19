#ifndef __WAITABLE_TIMER_H
#define __WAITABLE_TIMER_H

#ifdef DISABLE_BOOST_TIMER
#include "msg_queue.h"
#include "mem_pool.h"
#include "shared_strand.h"
#include "run_thread.h"

class ActorTimer_;
class TimerBoost_;
class WaitableTimerEvent_;

class WaitableTimer_
{
	typedef msg_multimap<unsigned long long, WaitableTimerEvent_*> handler_queue;

	struct timer_handle
	{
		void reset()
		{
			_null = true;
		}

		bool _null = true;
		handler_queue::iterator _queueNode;
	};

	friend io_engine;
	friend WaitableTimerEvent_;
private:
	WaitableTimer_();
	~WaitableTimer_();
private:
	void appendEvent(long long us, WaitableTimerEvent_* h);
	void removeEvent(timer_handle& th);
	void timerThread();
private:
	unsigned long long _extMaxTick;
	unsigned long long _extFinishTime;
	handler_queue _eventsQueue;
	std::mutex _ctrlMutex;
	run_thread _timerThread;
#ifdef WIN32
	void* _timerHandle;
#elif __linux__
	int _timerFd;
#endif
	volatile bool _exited;
};

class WaitableTimerEvent_
{
	friend ActorTimer_;
	friend TimerBoost_;
	friend WaitableTimer_;
private:
	WaitableTimerEvent_(io_engine& ios, TimerBoostCompletedEventFace_* timerBoost);
	~WaitableTimerEvent_();
private:
	void eventHandler();
	void cancel(boost::system::error_code& ec);
	void async_wait(long long us, int tc);
private:
	io_engine& _ios;
	WaitableTimer_::timer_handle _timerHandle;
	TimerBoostCompletedEventFace_* _timerBoost;
	int _tcId;
	bool _triged;
};
#endif

#endif