#ifndef __ASYNC_TIMER_H
#define __ASYNC_TIMER_H

#include <algorithm>
#include "shared_strand.h"
#include "msg_queue.h"
#include "mem_pool.h"

class async_timer;

/*!
@brief 定时器性能加速器
*/
class timer_boost
{
	typedef std::shared_ptr<async_timer> async_handle;
	typedef msg_multimap<unsigned long long, async_handle> handler_queue;

	friend async_timer;
	FRIEND_SHARED_PTR(timer_boost);

	class timer_handle
	{
		friend timer_boost;
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
	timer_boost(const shared_strand& strand);
	~timer_boost();
public:
	static std::shared_ptr<timer_boost> create(const shared_strand& strand);
	const shared_strand& self_strand() const;
private:
	timer_handle timeout(unsigned long long us, const async_handle& host);
	void cancel(timer_handle& th);
	void timer_loop(unsigned long long us);
private:
	io_engine& _ios;
	void* _timer;
	bool _looping;
	int _timerCount;
	shared_strand _strand;
	handler_queue _handlerQueue;
	unsigned long long _extMaxTick;
	unsigned long long _extFinishTime;
	std::weak_ptr<timer_boost> _weakThis;
	std::shared_ptr<timer_boost> _lockThis;
};

/*!
@brief 异步定时器，依赖于timer_boost，一个定时循环一个async_timer
*/
class async_timer
{
	friend timer_boost;
	FRIEND_SHARED_PTR(async_timer);

	struct wrap_base
	{
		virtual void invoke(reusable_mem& reuMem) = 0;
		virtual void destory(reusable_mem& reuMem) = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_base
	{
		template <typename H>
		wrap_handler(H&& h)
			:_h(TRY_MOVE(h)) {}

		void invoke(reusable_mem& reuMem)
		{
			Handler cb(std::move(_h));
			destory(reuMem);
			cb();
		}

		void destory(reusable_mem& reuMem)
		{
			this->~wrap_handler();
			reuMem.deallocate(this, sizeof(*this));
		}

		Handler _h;
	};
private:
	async_timer(const std::shared_ptr<timer_boost>& timer);
	~async_timer();
public:
	/*!
	@brief 创建一个定时器
	@param timerBoost timer_boost定时器加速器
	*/
	static std::shared_ptr<async_timer> create(const std::shared_ptr<timer_boost>& timerBoost);
public:
	/*!
	@brief 开启一个定时循环，在timer_boost strand线程中调用
	*/
	template <typename Handler>
	void timeout(int tm, Handler&& handler)
	{
		assert(_timerBoost->_strand->running_in_this_thread());
		assert(!_handler);
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		_handler = new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(TRY_MOVE(handler));
		_timerHandle = _timerBoost->timeout(tm * 1000, _weakThis.lock());
	}

	/*!
	@brief 取消本次计时，在timer_boost strand线程中调用
	*/
	void cancel();

	const shared_strand& self_strand() const;
	const std::shared_ptr<timer_boost>& self_timer_boost() const;
private:
	void timeout_handler();
private:
	wrap_base* _handler;
	reusable_mem _reuMem;
	std::weak_ptr<async_timer> _weakThis;
	std::shared_ptr<timer_boost> _timerBoost;
	timer_boost::timer_handle _timerHandle;
};

#endif