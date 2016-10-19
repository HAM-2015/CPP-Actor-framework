#ifndef __ASYNC_TIMER_H
#define __ASYNC_TIMER_H

#include <algorithm>
#include "run_strand.h"
#include "msg_queue.h"
#include "mem_pool.h"
#include "stack_object.h"

class ActorTimer_;
class qt_strand;
class uv_strand;
class boost_strand;
typedef std::shared_ptr<AsyncTimer_> async_timer;

/*!
@brief 异步定时器，一个定时循环一个AsyncTimer_
*/
class AsyncTimer_ : public ActorTimerFace_
{
	friend boost_strand;
	FRIEND_SHARED_PTR(AsyncTimer_);

	struct wrap_base
	{
		virtual void invoke() = 0;
		virtual void destroy() = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_base
	{
		template <typename H>
		wrap_handler(H&& h)
			:_h(std::forward<H>(h)) {}

		void invoke()
		{
			CHECK_EXCEPTION(_h);
			destroy();
		}

		void destroy()
		{
			this->~wrap_handler();
		}

		Handler _h;
	};
private:
	AsyncTimer_(ActorTimer_* actorTimer);
	~AsyncTimer_();
public:
	/*!
	@brief 开启一个定时，在依赖的strand线程中调用
	*/
	template <typename Handler>
	long long timeout(int tm, Handler&& handler)
	{
		assert(self_strand()->running_in_this_thread());
		assert(!_handler);
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		_handler = new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(std::forward<Handler>(handler));
		_timerHandle = _actorTimer->timeout((long long)tm * 1000, _weakThis.lock(), false);
		return _timerHandle._beginStamp;
	}

	/*!
	@brief 开启一个绝对定时，在依赖的strand线程中调用
	*/
	template <typename Handler>
	long long deadline(long long us, Handler&& handler)
	{
		assert(self_strand()->running_in_this_thread());
		assert(!_handler);
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		_handler = new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(std::forward<Handler>(handler));
		_timerHandle = _actorTimer->timeout(us, _weakThis.lock(), true);
		return _timerHandle._beginStamp;
	}

	/*!
	@brief 循环定时调用一个handler，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void interval(int tm, Handler&& handler)
	{
		_interval(tm, get_tick_us(), std::forward<Handler>(handler));
	}

	/*!
	@brief 取消本次计时，在依赖的strand线程中调用
	*/
	void cancel();

	/*!
	@brief 
	*/
	shared_strand self_strand();

	/*!
	@brief 创建一个依赖同一个shared_strand的定时器
	*/
	async_timer clone();
private:
	void timeout_handler();

	template <typename Handler>
	void _interval(int tm, long long deadtime, Handler&& handler)
	{
		typedef RM_CREF(Handler) Handler_;
		deadtime += (long long)tm * 1000;
		deadline(deadtime, std::bind([this, tm, deadtime](Handler_& handler)
		{
			CHECK_EXCEPTION(handler);
			_interval(tm, deadtime, std::move(handler));
		}, std::forward<Handler>(handler)));
	}
private:
	wrap_base* _handler;
	reusable_mem _reuMem;
	ActorTimer_* _actorTimer;
	std::weak_ptr<AsyncTimer_> _weakThis;
	ActorTimer_::timer_handle _timerHandle;
	NONE_COPY(AsyncTimer_);
};

/*!
@brief 可重叠使用的定时器
*/
class overlap_timer
#ifdef DISABLE_BOOST_TIMER
	: public TimerBoostCompletedEventFace_
#endif
{
	struct wrap_base
	{
		virtual void invoke() = 0;
		virtual void destroy() = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_base
	{
		template <typename H>
		wrap_handler(H&& h)
			:_h(std::forward<H>(h)) {}

		void invoke()
		{
			CHECK_EXCEPTION(_h);
			destroy();
		}

		void destroy()
		{
			this->~wrap_handler();
		}

		Handler _h;
	};

	typedef msg_multimap<long long, wrap_base*> handler_queue;

	friend boost_strand;
	friend qt_strand;
	friend uv_strand;
public:
	class timer_handle
	{
		friend overlap_timer;
	public:
		timer_handle()
			:_beginStamp(0) {}

		~timer_handle()
		{
			assert(0 == _beginStamp);
		}

		long long stamp()
		{
			return _beginStamp;
		}
	private:
		bool is_null() const
		{
			return 0 == _beginStamp;
		}

		void reset()
		{
			_beginStamp = 0;
		}
	private:
		long long _beginStamp;
		handler_queue::iterator _queueNode;
		NONE_COPY(timer_handle);
	};
private:
	overlap_timer(const shared_strand& strand);
	~overlap_timer();
public:
	/*!
	@brief 开启一个定时，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void timeout(int ms, timer_handle& timerHandle, Handler&& handler)
	{
		assert(_weakStrand.lock()->running_in_this_thread());
		assert(timerHandle.is_null());
		_timeout((long long)ms * 1000, timerHandle, wrap_timer_handler(std::bind([&timerHandle](Handler& handler)
		{
			timerHandle.reset();
			CHECK_EXCEPTION(handler);
		}, std::forward<Handler>(handler))));
	}

	/*!
	@brief 开启一个绝对定时，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void deadline(long long us, timer_handle& timerHandle, Handler&& handler)
	{
		assert(_weakStrand.lock()->running_in_this_thread());
		assert(timerHandle.is_null());
		_timeout(us, timerHandle, wrap_timer_handler(std::bind([&timerHandle](Handler& handler)
		{
			timerHandle.reset();
			CHECK_EXCEPTION(handler);
		}, std::forward<Handler>(handler))), true);
	}

	/*!
	@brief 循环定时调用一个handler，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void interval(int tm, timer_handle& timerHandle, Handler&& handler)
	{
		_interval(tm, get_tick_us(), timerHandle, std::forward<Handler>(handler));
	}

	/*!
	@brief 取消一次计时，在依赖的strand线程中调用
	*/
	void cancel(timer_handle& th);
private:
	void timer_loop(long long abs, long long rel);
	void event_handler(int tc);
#ifdef DISABLE_BOOST_TIMER
	void post_event(int tc);
#endif
private:
	void _timeout(long long us, timer_handle& timerHandle, wrap_base* handler, bool deadline = false);

	template <typename Handler>
	wrap_base* wrap_timer_handler(Handler&& handler)
	{
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		return new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(std::forward<Handler>(handler));
	}

	template <typename Handler>
	void _interval(int tm, long long deadtime, timer_handle& timerHandle, Handler&& handler)
	{
		typedef RM_CREF(Handler) Handler_;
		deadtime += (long long)tm * 1000;
		deadline(deadtime, timerHandle, wrap_bind([this, &timerHandle, tm, deadtime](Handler_& handler)
		{
			CHECK_EXCEPTION(handler);
			_interval(tm, deadtime, timerHandle, std::move(handler));
		}, std::forward<Handler>(handler)));
	}
private:
	void* _timer;
	std::weak_ptr<boost_strand>& _weakStrand;
	shared_strand _lockStrand;
	handler_queue _handlerQueue;
	reusable_mem _reuMem;
	long long _extMaxTick;
	long long _extFinishTime;
#ifdef DISABLE_BOOST_TIMER
	stack_obj<boost::asio::io_service::work, false> _lockIos;
#endif
	int _timerCount;
	bool _looping;
	NONE_COPY(overlap_timer);
};

#endif