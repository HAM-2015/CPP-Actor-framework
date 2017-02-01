#include "actor_timer.h"
#include "scattered.h"
#include "my_actor.h"
#ifndef DISABLE_BOOST_TIMER
#ifdef DISABLE_HIGH_TIMER
#include <boost/asio/deadline_timer.hpp>
typedef boost::asio::deadline_timer timer_type;
typedef boost::posix_time::microseconds micseconds;
#else
#include <boost/chrono/system_clocks.hpp>
#include <boost/asio/high_resolution_timer.hpp>
typedef boost::asio::basic_waitable_timer<boost::chrono::high_resolution_clock> timer_type;
typedef boost::chrono::microseconds micseconds;
#endif
#else
#include "waitable_timer.h"
typedef WaitableTimerEvent_ timer_type;
typedef long long micseconds;
#endif

ActorTimer_::ActorTimer_(const shared_strand& strand)
:_weakStrand(strand->_weakThis), _looping(false), _timerCount(0),
_extMaxTick(0), _extFinishTime(-1), _handlerQueue(MEM_POOL_LENGTH)
{
#ifdef DISABLE_BOOST_TIMER
	_timer = new timer_type(strand->get_io_engine(), this);
#else
	_timer = new timer_type(strand->get_io_engine());
#endif
}

ActorTimer_::~ActorTimer_()
{
	assert(_handlerQueue.empty());
	delete (timer_type*)_timer;
}

ActorTimer_::timer_handle ActorTimer_::timeout(long long us, actor_face_handle&& host, bool deadline)
{
	assert(_weakStrand.lock()->running_in_this_thread());
	if (!_lockStrand)
	{
		_lockStrand = _weakStrand.lock();
#ifdef DISABLE_BOOST_TIMER
		_lockIos.create(_lockStrand->get_io_engine());
#endif
	}
	assert(_lockStrand->running_in_this_thread());
	timer_handle timerHandle;
	timerHandle._beginStamp = get_tick_us();
	long long et = deadline ? us : (timerHandle._beginStamp + us);
	if (et >= _extMaxTick)
	{
		_extMaxTick = et;
		timerHandle._queueNode = _handlerQueue.insert(_handlerQueue.end(), std::make_pair(et, std::move(host)));
	}
	else
	{
		timerHandle._queueNode = _handlerQueue.insert(std::make_pair(et, std::move(host)));
	}
	
	if (!_looping)
	{//定时器已经退出循环，重新启动定时器
		_looping = true;
		assert(_handlerQueue.size() == 1);
		_extFinishTime = et;
		timer_loop(et, et - timerHandle._beginStamp);
	}
	else if ((unsigned long long)et < (unsigned long long)_extFinishTime)
	{//定时期限前于当前定时器期限，取消后重新计时
		boost::system::error_code ec;
		as_ptype<timer_type>(_timer)->cancel(ec);
		_timerCount++;
		_extFinishTime = et;
		timer_loop(et, et - timerHandle._beginStamp);
	}
	return timerHandle;
}

void ActorTimer_::cancel(timer_handle& th)
{
	assert(_weakStrand.lock()->running_in_this_thread());
	if (!th.is_null())
	{//删除当前定时器节点
		assert(_lockStrand);
		th.reset();
		handler_queue::iterator itNode = th._queueNode;
		if (_handlerQueue.size() == 1)
		{
			_extMaxTick = 0;
			_handlerQueue.erase(itNode);
			//如果没有定时任务就退出定时循环
			boost::system::error_code ec;
			as_ptype<timer_type>(_timer)->cancel(ec);
			_timerCount++;
			_looping = false;
		}
		else if (itNode->first == _extMaxTick)
		{
			_handlerQueue.erase(itNode++);
			if (_handlerQueue.end() == itNode)
			{
				_extMaxTick = (--itNode)->first;
			}
			else
			{
				assert(itNode->first == _extMaxTick);
			}
		}
		else
		{
			_handlerQueue.erase(itNode);
		}
	}
}

void ActorTimer_::timer_loop(long long abs, long long rel)
{
	int tc = ++_timerCount;
#ifdef DISABLE_BOOST_TIMER
	as_ptype<timer_type>(_timer)->async_wait(micseconds(abs), micseconds(rel), tc);
#else
	boost::system::error_code ec;
#ifdef DISABLE_HIGH_TIMER
	as_ptype<timer_type>(_timer)->expires_from_now(micseconds(rel), ec);
#else
	as_ptype<timer_type>(_timer)->expires_at(timer_type::time_point(micseconds(abs)), ec);
#endif
	as_ptype<timer_type>(_timer)->async_wait(_lockStrand->wrap_asio_once([this, tc](const boost::system::error_code&)
	{
		event_handler(tc);
	}));
#endif
}

#ifdef DISABLE_BOOST_TIMER
void ActorTimer_::post_event(int tc)
{
	assert(_lockStrand);
	_lockStrand->post([this, tc]
	{
		event_handler(tc);
		if (!_lockStrand)
		{
			_lockIos.destroy();
		}
	});
}
#endif

void ActorTimer_::event_handler(int tc)
{
	assert(_lockStrand->running_in_this_thread());
	if (tc == _timerCount)
	{
		_extFinishTime = 0;
		while (!_handlerQueue.empty())
		{
			handler_queue::iterator iter = _handlerQueue.begin();
			long long ct = get_tick_us();
			if (iter->first > ct)
			{
				_extFinishTime = iter->first;
				timer_loop(_extFinishTime, _extFinishTime - ct);
				return;
			}
			else
			{
				iter->second->timeout_handler();
				_handlerQueue.erase(iter);
			}
		}
		_looping = false;
		_lockStrand.reset();
	}
	else if (tc == _timerCount - 1)
	{
		_lockStrand.reset();
	}
}