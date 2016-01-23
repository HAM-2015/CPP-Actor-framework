#ifndef __ASYNC_TIMER_H
#define __ASYNC_TIMER_H

#include <algorithm>
#include "shared_strand.h"
#include "msg_queue.h"
#include "mem_pool.h"

class AsyncTimer_;
class qt_strand;
class boost_strand;
typedef std::shared_ptr<AsyncTimer_> async_timer;

/*!
@brief 定时器性能加速器
*/
class TimerBoost_
{
	typedef msg_multimap<unsigned long long, async_timer> handler_queue;

	friend AsyncTimer_;
	friend qt_strand;
	friend boost_strand;

	class timer_handle
	{
		friend TimerBoost_;
	public:
		void reset()
		{
			_null = true;
		}
	private:
		bool _null = true;
		handler_queue::iterator _queueNode;
	};
private:
	TimerBoost_(const shared_strand& strand);
	~TimerBoost_(); 
private:
	/*!
	@brief 开始计时
	@param us 微秒
	@param host 准备计时的AsyncTimer_
	@return 计时句柄，用于cancel
	*/
	timer_handle timeout(unsigned long long us, async_timer&& host);

	/*!
	@brief 取消计时
	*/
	void cancel(timer_handle& th);

	/*!
	@brief timer循环
	*/
	void timer_loop(unsigned long long us);
private:
	void* _timer;
	std::weak_ptr<boost_strand>& _weakStrand;
	shared_strand _lockStrand;
	handler_queue _handlerQueue;
	unsigned long long _extMaxTick;
	unsigned long long _extFinishTime;
	int _timerCount;
	bool _looping;
};

/*!
@brief 异步定时器，依赖于TimerBoost_，一个定时循环一个AsyncTimer_
*/
class AsyncTimer_
{
	friend TimerBoost_;
	friend boost_strand;
	FRIEND_SHARED_PTR(AsyncTimer_);

	struct wrap_base
	{
		virtual void invoke(reusable_mem& reuMem) = 0;
		virtual void destroy(reusable_mem& reuMem) = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_base
	{
		template <typename H>
		wrap_handler(H&& h)
			:_h(TRY_MOVE(h)) {}

		void invoke(reusable_mem& reuMem)
		{
			_h();
			destroy(reuMem);
		}

		void destroy(reusable_mem& reuMem)
		{
			this->~wrap_handler();
			reuMem.deallocate(this);
		}

		Handler _h;
	};
private:
	AsyncTimer_(TimerBoost_& timerBoost);
	~AsyncTimer_();
public:
	/*!
	@brief 开启一个定时循环，在依赖的strand线程中调用
	*/
	template <typename Handler>
	void timeout(int tm, Handler&& handler)
	{
		assert(!_handler);
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		_handler = new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(TRY_MOVE(handler));
		_timerHandle = _timerBoost.timeout(tm * 1000, _weakThis.lock());
	}

	/*!
	@brief 取消本次计时，在依赖的strand线程中调用
	*/
	void cancel();

	/*!
	@brief 
	*/
	shared_strand self_strand();
private:
	void timeout_handler();
private:
	wrap_base* _handler;
	reusable_mem _reuMem;
	TimerBoost_& _timerBoost;
	std::weak_ptr<AsyncTimer_> _weakThis;
	TimerBoost_::timer_handle _timerHandle;
};

#endif