#ifndef __WAITABLE_TIMER_H
#define __WAITABLE_TIMER_H

#ifdef DISABLE_BOOST_TIMER
#ifdef _MSC_VER
#include "msg_queue.h"
#include "mem_pool.h"
#include "io_engine.h"
#include <boost/thread/thread.hpp>

class ActorTimer_;
class timer_boost;
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
	bool _exit;
	void* _timerHandle;
	unsigned long long _extMaxTick;
	unsigned long long _extFinishTime;
	handler_queue _eventsQueue;
	std::mutex _ctrlMutex;
	boost::thread _timerThread;
};

class WaitableTimerEvent_
{
	struct wrap_base
	{
		virtual void invoke(reusable_mem& reuMem) = 0;
		virtual void invoke_err(reusable_mem& reuMem) = 0;
		virtual void destory(reusable_mem& reuMem) = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_base
	{
		template <typename H>
		wrap_handler(io_engine& ios, H&& h)
			:_lockWork(ios), _h(TRY_MOVE(h)) {}

		void invoke(reusable_mem& reuMem)
		{
			_h(boost::system::error_code());
			destory(reuMem);
		}

		void invoke_err(reusable_mem& reuMem)
		{
			_h(boost::system::error_code(1, boost::system::system_category()));
			destory(reuMem);
		}

		void destory(reusable_mem& reuMem)
		{
			this->~wrap_handler();
			reuMem.deallocate(this, sizeof(*this));
		}

		boost::asio::io_service::work _lockWork;
		Handler _h;
	};

	friend io_engine;
	friend ActorTimer_;
	friend timer_boost;
	friend WaitableTimer_;
private:
	WaitableTimerEvent_(io_engine& ios, WaitableTimer_* timer);
	~WaitableTimerEvent_();
private:
	void eventHandler();
	void expires_from_now(long long us, boost::system::error_code& ec);
	void cancel(boost::system::error_code& ec);

	template <typename Handler>
	void async_wait(Handler&& handler)
	{
		assert(!_handler);
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		_handler = new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(_ios, TRY_MOVE(handler));
		_timer->appendEvent(_eus, this);
	}
private:
	io_engine& _ios;
	long long _eus;
	wrap_base* _handler;
	reusable_mem _reuMem;
	WaitableTimer_* _timer;
	WaitableTimer_::timer_handle _timerHandle;
};
#elif __GNUG__
#error "do not define DISABLE_BOOST_TIMER"
#endif
#endif

#endif