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
		}

		void destroy()
		{
			this->~wrap_handler();
		}

		Handler _h;
	};

	template <typename Handler>
	struct wrap_interval_handler : public wrap_base
	{
		template <typename H>
		wrap_interval_handler(H&& h)
			:_h(std::forward<H>(h)) {}

		void invoke()
		{
			CHECK_EXCEPTION(_h, this);
		}

		void destroy()
		{
			this->~wrap_interval_handler();
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
		_isInterval = false;
		_handler = wrap_timer_handler(std::forward<Handler>(handler));
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
		_isInterval = false;
		_handler = wrap_timer_handler(std::forward<Handler>(handler));
		_timerHandle = _actorTimer->timeout(us, _weakThis.lock(), true);
		return _timerHandle._beginStamp;
	}

	/*!
	@brief 循环定时调用一个handler，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void interval(int tm, Handler&& handler)
	{
		assert(self_strand()->running_in_this_thread());
		assert(!_handler);
		_isInterval = true;
		int intId = ++_intervalCount;
		long long deadtime = get_tick_us() + (long long)tm * 1000;
		_handler = wrap_timer_handler(std::bind([](wrap_base* const intervalHandler)
		{
			intervalHandler->invoke();
		}, wrap_interval_timer_handler(std::bind([this, tm, intId](wrap_base* intervalHandler, Handler& handler, long long& deadtime)
		{
			CHECK_EXCEPTION(handler);
			if (_handler && _isInterval && _intervalCount == intId)
			{
				deadtime += (long long)tm * 1000;
				_timerHandle = _actorTimer->timeout(deadtime, _weakThis.lock(), true);
			}
			else
			{
				intervalHandler->destroy();
				_reuMem.deallocate(intervalHandler);
			}
		}, __1, std::forward<Handler>(handler), deadtime))));
		_timerHandle = _actorTimer->timeout(deadtime, _weakThis.lock(), true);
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
	wrap_base* wrap_timer_handler(Handler&& handler)
	{
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		return new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(std::forward<Handler>(handler));
	}

	template <typename Handler>
	wrap_base* wrap_interval_timer_handler(Handler&& handler)
	{
		typedef wrap_interval_handler<RM_CREF(Handler)> wrap_type;
		return new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(std::forward<Handler>(handler));
	}
private:
	wrap_base* _handler;
	reusable_mem _reuMem;
	ActorTimer_* _actorTimer;
	std::weak_ptr<AsyncTimer_> _weakThis;
	ActorTimer_::timer_handle _timerHandle;
	int _intervalCount;
	bool _isInterval;
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
public:
	class timer_handle;
private:
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
		}

		void destroy()
		{
			this->~wrap_handler();
		}

		Handler _h;
	};

	template <typename Handler>
	struct wrap_interval_handler : public wrap_base
	{
		template <typename H>
		wrap_interval_handler(H&& h)
			:_h(std::forward<H>(h)) {}

		void invoke()
		{
			CHECK_EXCEPTION(_h, this);
		}

		void destroy()
		{
			this->~wrap_interval_handler();
		}

		Handler _h;
	};

	typedef msg_multimap<long long, timer_handle*> handler_queue;

	friend boost_strand;
	friend qt_strand;
	friend uv_strand;
public:
	class timer_handle
	{
		friend overlap_timer;
	public:
		timer_handle()
			:_timestamp(0), _handler(NULL), _intervalCount(0), _isInterval(false) {}

		~timer_handle()
		{
			assert(0 == _handler);
		}

		long long timestamp()
		{
			return _timestamp;
		}
	private:
		bool is_null() const
		{
			return !_handler;
		}

		void reset()
		{
			_timestamp = 0;
			_handler = NULL;
		}
	private:
		long long _timestamp;
		handler_queue::iterator _queueNode;
		wrap_base* _handler;
		int _intervalCount;
		bool _isInterval;
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
		timerHandle._isInterval = false;
		timerHandle._handler = wrap_timer_handler(std::bind([&timerHandle](Handler& handler)
		{
			timerHandle.reset();
			CHECK_EXCEPTION(handler);
		}, std::forward<Handler>(handler)));
		_timeout((long long)ms * 1000, timerHandle);
	}

	/*!
	@brief 开启一个绝对定时，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void deadline(long long us, timer_handle& timerHandle, Handler&& handler)
	{
		assert(_weakStrand.lock()->running_in_this_thread());
		assert(timerHandle.is_null());
		timerHandle._isInterval = false;
		timerHandle._handler = wrap_timer_handler(std::bind([&timerHandle](Handler& handler)
		{
			timerHandle.reset();
			CHECK_EXCEPTION(handler);
		}, std::forward<Handler>(handler)));
		_timeout(us, timerHandle, true);
	}

	/*!
	@brief 循环定时调用一个handler，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void interval(int tm, timer_handle& timerHandle, Handler&& handler)
	{
		assert(self_strand()->running_in_this_thread());
		assert(timerHandle.is_null());
		timerHandle._isInterval = true;
		int intId = ++timerHandle._intervalCount;
		long long deadtime = get_tick_us() + (long long)tm * 1000;
		timerHandle._handler = wrap_timer_handler(std::bind([](wrap_base* const intervalHandler)
		{
			intervalHandler->invoke();
		}, wrap_interval_timer_handler(std::bind([this, &timerHandle, tm, intId](wrap_base* intervalHandler, Handler& handler, long long& deadtime)
		{
			CHECK_EXCEPTION(handler);
			if (timerHandle._handler && timerHandle._isInterval && timerHandle._intervalCount == intId)
			{
				deadtime += (long long)tm * 1000;
				_timeout(deadtime, timerHandle, true);
			}
			else
			{
				intervalHandler->destroy();
				_reuMem.deallocate(intervalHandler);
			}
		}, __1, std::forward<Handler>(handler), deadtime))));
		_timeout(deadtime, timerHandle, true);
	}

	/*!
	@brief 取消一次计时，在依赖的strand线程中调用
	*/
	void cancel(timer_handle& th);

	/*!
	@brief
	*/
	shared_strand self_strand();
private:
	void timer_loop(long long abs, long long rel);
	void event_handler(int tc);
#ifdef DISABLE_BOOST_TIMER
	void post_event(int tc);
#endif
private:
	void _timeout(long long us, timer_handle& timerHandle, bool deadline = false);

	template <typename Handler>
	wrap_base* wrap_timer_handler(Handler&& handler)
	{
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		return new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(std::forward<Handler>(handler));
	}

	template <typename Handler>
	wrap_base* wrap_interval_timer_handler(Handler&& handler)
	{
		typedef wrap_interval_handler<RM_CREF(Handler)> wrap_type;
		return new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(std::forward<Handler>(handler));
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