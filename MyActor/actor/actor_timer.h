#ifndef __ACTOR_TIMER_H
#define __ACTOR_TIMER_H

#include <algorithm>
#include "run_strand.h"
#include "msg_queue.h"
#include "stack_object.h"

class boost_strand;
class qt_strand;
class uv_strand;
class my_actor;
class generator;
class AsyncTimer_;

/*!
@brief Actor 内部使用的定时器
*/
class ActorTimer_
#ifdef DISABLE_BOOST_TIMER
	: public TimerBoostCompletedEventFace_
#endif
{
	typedef std::shared_ptr<ActorTimerFace_> actor_face_handle;
	typedef msg_multimap<long long, actor_face_handle> handler_queue;

	friend boost_strand;
	friend qt_strand;
	friend uv_strand;
	friend my_actor;
	friend generator;
	friend AsyncTimer_;

	class timer_handle 
	{
		friend ActorTimer_;
	public:
		bool is_null() const
		{
			return 0 == _beginStamp;
		}

		void reset()
		{
			_beginStamp = 0;
		}
		long long _beginStamp = 0;
	private:
		handler_queue::iterator _queueNode;
	};
private:
	ActorTimer_(const shared_strand& strand);
	~ActorTimer_();
private:
	/*!
	@brief 开始计时
	@param us 微秒
	@param host 准备计时的Actor
	@param deadline 是否为绝对时间
	@return 计时句柄，用于cancel
	*/
	timer_handle timeout(long long us, actor_face_handle&& host, bool deadline = false);

	/*!
	@brief 取消计时
	*/
	void cancel(timer_handle& th);

	/*!
	@brief timer循环
	*/
	void timer_loop(long long abs, long long rel);

	/*!
	@brief timer事件
	*/
	void event_handler(int tc);
#ifdef DISABLE_BOOST_TIMER
	void post_event(int tc);
	void cancel_event();
#endif
private:
	void* _timer;
	std::weak_ptr<boost_strand>& _weakStrand;
	shared_strand _lockStrand;
	handler_queue _handlerQueue;
	long long _extMaxTick;
	long long _extFinishTime;
#ifdef DISABLE_BOOST_TIMER
	stack_obj<io_work, false> _lockIos;
#endif
	int _timerCount;
	bool _looping;
	NONE_COPY(ActorTimer_);
};

#endif