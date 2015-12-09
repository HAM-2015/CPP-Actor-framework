#ifndef __ASYNC_TIMER_H
#define __ASYNC_TIMER_H

#include <algorithm>
#include "shared_strand.h"
#include "msg_queue.h"
#include "mem_pool.h"

class async_timer;

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
	const shared_strand& self_strand();
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
};

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
public:
	async_timer(const std::shared_ptr<timer_boost>& timer);
private:
	~async_timer();
public:
	static std::shared_ptr<async_timer> create(const std::shared_ptr<timer_boost>& timer);
public:
	template <typename Handler>
	void timeout(int tm, Handler&& handler)
	{
		assert(_timer->_strand->running_in_this_thread());
		assert(!_handler);
		typedef wrap_handler<RM_CREF(Handler)> wrap_type;
		_handler = new(_reuMem.allocate(sizeof(wrap_type)))wrap_type(TRY_MOVE(handler));
		_timerHandle = _timer->timeout(tm * 1000, _weakThis.lock());
	}

	void cancel();
	const shared_strand& self_strand();
	const std::shared_ptr<timer_boost>& self_timer();
private:
	void timeout_handler();
private:
	wrap_base* _handler;
	reusable_mem _reuMem;
	std::weak_ptr<async_timer> _weakThis;
	std::shared_ptr<timer_boost> _timer;
	timer_boost::timer_handle _timerHandle;
};

#endif