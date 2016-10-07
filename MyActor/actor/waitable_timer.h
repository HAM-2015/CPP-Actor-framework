#ifndef __WAITABLE_TIMER_H
#define __WAITABLE_TIMER_H

#ifdef DISABLE_BOOST_TIMER
#include "msg_queue.h"
#include "mem_pool.h"
#include "run_strand.h"
#include "run_thread.h"

class ActorTimer_;
class WaitableTimerEvent_;
class overlap_timer;

class WaitableTimer_
{
	typedef msg_multimap<long long, WaitableTimerEvent_*> handler_queue;

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
	void appendEvent(long long abs, long long rel, WaitableTimerEvent_* h);
	void removeEvent(timer_handle& th);
	void timerThread();
private:
	long long _extMaxTick;
	long long _extFinishTime;
	handler_queue _eventsQueue;
	std::mutex _ctrlMutex;
	run_thread _timerThread;
#ifdef WIN32
	void* _timerHandle;
#elif __linux__
	int _timerFd;
#endif
	volatile bool _exited;
	NONE_COPY(WaitableTimer_);
};

class WaitableTimerEvent_
{
	friend ActorTimer_;
	friend WaitableTimer_;
	friend overlap_timer;
private:
	WaitableTimerEvent_(io_engine& ios, TimerBoostCompletedEventFace_* timerBoost);
	~WaitableTimerEvent_();
private:
	void eventHandler();
	void cancel(boost::system::error_code& ec);
	void async_wait(long long abs, long long rel, int tc);
private:
	io_engine& _ios;
	WaitableTimer_::timer_handle _timerHandle;
	TimerBoostCompletedEventFace_* _timerBoost;
	int _tcId;
	bool _triged;
	NONE_COPY(WaitableTimerEvent_);
};
#endif

#endif