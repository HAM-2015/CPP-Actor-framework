#ifndef __ASYNC_TIMER_H
#define __ASYNC_TIMER_H

#include <algorithm>
#include "run_strand.h"
#include "msg_queue.h"
#include "mem_pool.h"
#include "stack_object.h"

class ActorTimer_;
class overlap_timer;
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
	friend overlap_timer;
	FRIEND_SHARED_PTR(AsyncTimer_);

	struct wrap_base
	{
		virtual void invoke(bool isAdvance = false) = 0;
		virtual void destroy(reusable_mem& reuMem) = 0;
		virtual void set_sign(bool* sign) = 0;
		virtual long long& deadtime_ref() = 0;
		virtual void set_deadtime(long long us) = 0;
		virtual bool is_top_call() = 0;
		virtual wrap_base* interval_handler() = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_base
	{
		typedef RM_CREF(Handler) handler_type;

		wrap_handler(Handler& handler)
			:_handler(std::forward<Handler>(handler)) {}

		void invoke(bool isAdvance = false)
		{
			CHECK_EXCEPTION(_handler);
		}

		void destroy(reusable_mem& reuMem)
		{
			this->~wrap_handler();
			reuMem.deallocate(this);
		}

		void set_sign(bool* sign) {}
		long long& deadtime_ref() { return *(long long*)NULL; }
		void set_deadtime(long long us) {}
		bool is_top_call() { return false; }
		wrap_base* interval_handler() { return NULL; }

		handler_type _handler;
	};

	template <typename Handler>
	struct wrap_advance_handler : public wrap_base
	{
		typedef RM_CREF(Handler) handler_type;

		wrap_advance_handler(Handler& handler)
			:_handler(std::forward<Handler>(handler)) {}

		void invoke(bool isAdvance = false)
		{
			CHECK_EXCEPTION(_handler, isAdvance);
		}

		void destroy(reusable_mem& reuMem)
		{
			this->~wrap_advance_handler();
			reuMem.deallocate(this);
		}

		void set_sign(bool* sign) {}
		long long& deadtime_ref() { return *(long long*)NULL; }
		void set_deadtime(long long us) {}
		bool is_top_call() { return false; }
		wrap_base* interval_handler() { return NULL; }

		handler_type _handler;
	};

	template <typename Handler>
	struct wrap_interval_handler : public wrap_base
	{
		typedef RM_CREF(Handler) handler_type;

		wrap_interval_handler(Handler& handler, wrap_base* intervalHandler)
			:_handler(std::forward<Handler>(handler)), _sign(NULL), _deadtime(0), _intervalHandler(intervalHandler) {}

		void invoke(bool isAdvance = false)
		{
			CHECK_EXCEPTION(_handler, isAdvance, this);
		}

		void destroy(reusable_mem& reuMem)
		{
			if (!_sign)
			{
				_intervalHandler->destroy(reuMem);
			}
			else
			{
				*_sign = true;
			}
			this->~wrap_interval_handler();
			reuMem.deallocate(this);
		}

		void set_sign(bool* sign)
		{
			_sign = sign;
		}

		long long& deadtime_ref()
		{
			return _deadtime;
		}

		void set_deadtime(long long us)
		{
			_deadtime = us;
		}

		bool is_top_call()
		{
			return NULL != _sign;
		}

		wrap_base* interval_handler()
		{
			return _intervalHandler;
		}

		long long _deadtime;
		bool* _sign;
		wrap_base* _intervalHandler;
		handler_type _handler;
	};
private:
	AsyncTimer_(ActorTimer_* actorTimer);
	~AsyncTimer_();
public:
	/*!
	@brief 开启一个定时，在依赖的strand线程中调用
	*/
	template <typename Handler>
	long long timeout(int ms, Handler&& handler)
	{
		return utimeout((long long)ms * 1000, std::forward<Handler>(handler));
	}

	template <typename Handler>
	long long utimeout(long long us, Handler&& handler)
	{
		assert(self_strand()->running_in_this_thread());
		assert(!_handler);
		_isInterval = false;
		_currTimeout = us;
		_handler = wrap_timer_handler(_reuMem, std::forward<Handler>(handler));
		_timerHandle = _actorTimer->timeout(us, _weakThis.lock());
		return _timerHandle._beginStamp;
	}

	template <typename Handler>
	long long timeout2(int ms, Handler&& handler)
	{
		return utimeout2((long long)ms * 1000, std::forward<Handler>(handler));
	}

	template <typename Handler>
	long long utimeout2(long long us, Handler&& handler)
	{
		assert(self_strand()->running_in_this_thread());
		assert(!_handler);
		_isInterval = false;
		_currTimeout = us;
		_handler = wrap_advance_timer_handler(_reuMem, std::forward<Handler>(handler));
		_timerHandle = _actorTimer->timeout(us, _weakThis.lock());
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
		_handler = wrap_timer_handler(_reuMem, std::forward<Handler>(handler));
		_timerHandle = _actorTimer->timeout(us, _weakThis.lock(), true);
		return _timerHandle._beginStamp;
	}

	template <typename Handler>
	long long deadline2(long long us, Handler&& handler)
	{
		assert(self_strand()->running_in_this_thread());
		assert(!_handler);
		_isInterval = false;
		_handler = wrap_advance_timer_handler(_reuMem, std::forward<Handler>(handler));
		_timerHandle = _actorTimer->timeout(us, _weakThis.lock(), true);
		return _timerHandle._beginStamp;
	}

	/*!
	@brief 循环定时调用一个handler，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void interval(int ms, Handler&& handler, bool immed = false)
	{
		uinterval((long long)ms * 1000, std::forward<Handler>(handler), immed);
	}

	template <typename Handler>
	void interval2(int ms, Handler&& handler, bool immed = false)
	{
		uinterval2((long long)ms * 1000, std::forward<Handler>(handler), immed);
	}

	template <typename Handler>
	void uinterval(long long intervalus, Handler&& handler, bool immed = false)
	{
		uinterval2(intervalus, std::bind([](bool isAdvance, Handler& handler)
		{
			handler();
		}, __1, std::forward<Handler>(handler)), immed);
	}

	template <typename Handler>
	void uinterval2(long long intervalus, Handler&& handler, bool immed = false)
	{
		assert(self_strand()->running_in_this_thread());
		assert(!_handler);
		_isInterval = true;
		_currTimeout = intervalus;
		_handler = wrap_interval_timer_handler(_reuMem, [this, intervalus](bool isAdvance, wrap_base* const thisHandler)
		{
			bool sign = false;
			AsyncTimer_* const this_ = this;
			wrap_base* const intervalHandler = thisHandler->interval_handler();
			thisHandler->set_sign(&sign);
			intervalHandler->invoke(isAdvance);
			if (!sign)
			{
				thisHandler->set_sign(NULL);
				if (!isAdvance)
				{
					long long& deadtime = thisHandler->deadtime_ref();
					deadtime += intervalus;
					_timerHandle = _actorTimer->timeout(deadtime, _weakThis.lock(), true);
				}
			}
			else
			{
				intervalHandler->destroy(this_->_reuMem);
			}
		}, wrap_advance_timer_handler(_reuMem, std::forward<Handler>(handler)));
		if (immed)
		{
			_handler->set_deadtime(get_tick_us());
			_handler->invoke();
		}
		else
		{
			_timerHandle = _actorTimer->timeout(intervalus, _weakThis.lock());
			_handler->set_deadtime(_timerHandle._beginStamp + intervalus);
		}
	}

	/*!
	@brief 取消本次计时，在依赖的strand线程中调用
	*/
	void cancel();

	/*!
	@brief 提前本次计时触发，在依赖的strand线程中调用
	*/
	bool advance();

	/*!
	@brief 提前本次计时触发，在依赖的strand线程中调用
	*/
	void tick_advance();

	/*!
	@brief 相对计时下，重新开始计时
	*/
	bool restart();

	/*!
	@brief 是否已经完成
	*/
	bool completed();

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
	static wrap_base* wrap_timer_handler(reusable_mem& reuMem, Handler&& handler)
	{
		typedef wrap_handler<Handler> wrap_type;
		return new(reuMem.allocate(sizeof(wrap_type)))wrap_type(handler);
	}

	template <typename Handler>
	static wrap_base* wrap_advance_timer_handler(reusable_mem& reuMem, Handler&& handler)
	{
		typedef wrap_advance_handler<Handler> wrap_type;
		return new(reuMem.allocate(sizeof(wrap_type)))wrap_type(handler);
	}

	template <typename Handler>
	static wrap_base* wrap_interval_timer_handler(reusable_mem& reuMem, Handler&& handler, wrap_base* intervalHandler)
	{
		typedef wrap_interval_handler<Handler> wrap_type;
		return new(reuMem.allocate(sizeof(wrap_type)))wrap_type(handler, intervalHandler);
	}
private:
	ActorTimer_* _actorTimer;
	wrap_base* _handler;
	long long _currTimeout;
	reusable_mem _reuMem;
	std::weak_ptr<AsyncTimer_> _weakThis;
	ActorTimer_::timer_handle _timerHandle;
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
			:_timestamp(0), _currTimeout(0), _handler(NULL), _isInterval(false) {}

		~timer_handle()
		{
			assert(NULL == _handler);
		}

		long long timestamp()
		{
			return _timestamp;
		}

		bool completed()
		{
			return !_handler;
		}
	private:
		void reset()
		{
			_timestamp = 0;
			_currTimeout = 0;
			_handler = NULL;
		}
	private:
		long long _timestamp;
		long long _currTimeout;
		handler_queue::iterator _queueNode;
		AsyncTimer_::wrap_base* _handler;
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
		utimeout((long long)ms * 1000, timerHandle, std::forward<Handler>(handler));
	}

	template <typename Handler>
	void timeout2(int ms, timer_handle& timerHandle, Handler&& handler)
	{
		utimeout2((long long)ms * 1000, timerHandle, std::forward<Handler>(handler));
	}

	template <typename Handler>
	void utimeout(long long us, timer_handle& timerHandle, Handler&& handler)
	{
		assert(_weakStrand.lock()->running_in_this_thread());
		assert(timerHandle.completed());
		timerHandle._isInterval = false;
		timerHandle._currTimeout = us;
		timerHandle._handler = AsyncTimer_::wrap_timer_handler(_reuMem, std::bind([&timerHandle](Handler& handler)
		{
			timerHandle.reset();
			CHECK_EXCEPTION(handler);
		}, std::forward<Handler>(handler)));
		_timeout(us, timerHandle);
	}

	template <typename Handler>
	void utimeout2(long long us, timer_handle& timerHandle, Handler&& handler)
	{
		assert(_weakStrand.lock()->running_in_this_thread());
		assert(timerHandle.completed());
		timerHandle._isInterval = false;
		timerHandle._currTimeout = us;
		timerHandle._handler = AsyncTimer_::wrap_advance_timer_handler(_reuMem, std::bind([&timerHandle](bool isAdvance, Handler& handler)
		{
			timerHandle.reset();
			CHECK_EXCEPTION(handler, isAdvance);
		}, __1, std::forward<Handler>(handler)));
		_timeout(us, timerHandle);
	}

	/*!
	@brief 开启一个绝对定时，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void deadline(long long us, timer_handle& timerHandle, Handler&& handler)
	{
		assert(_weakStrand.lock()->running_in_this_thread());
		assert(timerHandle.completed());
		timerHandle._isInterval = false;
		timerHandle._handler = AsyncTimer_::wrap_timer_handler(_reuMem, std::bind([&timerHandle](Handler& handler)
		{
			timerHandle.reset();
			CHECK_EXCEPTION(handler);
		}, std::forward<Handler>(handler)));
		_timeout(us, timerHandle, true);
	}

	template <typename Handler>
	void deadline2(long long us, timer_handle& timerHandle, Handler&& handler)
	{
		assert(_weakStrand.lock()->running_in_this_thread());
		assert(timerHandle.completed());
		timerHandle._isInterval = false;
		timerHandle._handler = AsyncTimer_::wrap_advance_timer_handler(_reuMem, std::bind([&timerHandle](bool isAdvance, Handler& handler)
		{
			timerHandle.reset();
			CHECK_EXCEPTION(handler, isAdvance);
		}, __1, std::forward<Handler>(handler)));
		_timeout(us, timerHandle, true);
	}

	/*!
	@brief 循环定时调用一个handler，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void interval(int ms, timer_handle& timerHandle, Handler&& handler, bool immed = false)
	{
		uinterval((long long)ms * 1000, timerHandle, std::forward<Handler>(handler), immed);
	}

	template <typename Handler>
	void interval2(int ms, timer_handle& timerHandle, Handler&& handler, bool immed = false)
	{
		uinterval2((long long)ms * 1000, timerHandle, std::forward<Handler>(handler), immed);
	}

	template <typename Handler>
	void uinterval(long long intervalus, timer_handle& timerHandle, Handler&& handler, bool immed = false)
	{
		uinterval2(intervalus, timerHandle, std::bind([](bool isAdvance, Handler& handler)
		{
			handler();
		}, __1, std::forward<Handler>(handler)), immed);
	}

	template <typename Handler>
	void uinterval2(long long intervalus, timer_handle& timerHandle, Handler&& handler, bool immed = false)
	{
		assert(self_strand()->running_in_this_thread());
		assert(timerHandle.completed());
		timerHandle._isInterval = true;
		timerHandle._currTimeout = intervalus;
		timerHandle._handler = AsyncTimer_::wrap_interval_timer_handler(_reuMem, [this, &timerHandle, intervalus](bool isAdvance, AsyncTimer_::wrap_base* const thisHandler)
		{
			bool sign = false;
			overlap_timer* const this_ = this;
			AsyncTimer_::wrap_base* const intervalHandler = thisHandler->interval_handler();
			thisHandler->set_sign(&sign);
			intervalHandler->invoke(isAdvance);
			if (!sign)
			{
				thisHandler->set_sign(NULL);
				if (!isAdvance)
				{
					long long& deadtime = thisHandler->deadtime_ref();
					deadtime += intervalus;
					_timeout(deadtime, timerHandle, true);
				}
			}
			else
			{
				intervalHandler->destroy(this_->_reuMem);
			}
		}, AsyncTimer_::wrap_advance_timer_handler(_reuMem, std::forward<Handler>(handler)));
		if (immed)
		{
			timerHandle._handler->set_deadtime(get_tick_us());
			timerHandle._handler->invoke();
		}
		else
		{
			_timeout(intervalus, timerHandle);
			timerHandle._handler->set_deadtime(timerHandle._timestamp + intervalus);
		}
	}

	/*!
	@brief 取消一次计时，在依赖的strand线程中调用
	*/
	void cancel(timer_handle& timerHandle);

	/*!
	@brief 提前本次计时触发，在依赖的strand线程中调用
	*/
	bool advance(timer_handle& timerHandle);

	/*!
	@brief 提前本次计时触发，在依赖的strand线程中调用
	*/
	void tick_advance(timer_handle& timerHandle);

	/*!
	@brief 相对计时下，重新开始计时
	*/
	bool restart(timer_handle& timerHandle);

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
	void _cancel(timer_handle& timerHandle);
	void _timeout(long long us, timer_handle& timerHandle, bool deadline = false);
private:
	void* _timer;
	std::weak_ptr<boost_strand>& _weakStrand;
	shared_strand _lockStrand;
	handler_queue _handlerQueue;
	reusable_mem _reuMem;
	long long _extMaxTick;
	long long _extFinishTime;
#ifdef DISABLE_BOOST_TIMER
	stack_obj<io_work, false> _lockIos;
#endif
	int _timerCount;
	bool _looping;
	NONE_COPY(overlap_timer);
};

#endif