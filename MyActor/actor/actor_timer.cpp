#include "actor_timer.h"
#include "scattered.h"
#include "actor_framework.h"
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
_extMaxTick(0), _extFinishTime(-1), _handlerQueue(65536)
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

ActorTimer_::timer_handle ActorTimer_::timeout(unsigned long long us, actor_handle&& host)
{
	if (!_lockStrand)
	{
		_lockStrand = _weakStrand.lock();
#ifdef DISABLE_BOOST_TIMER
		_lockIos.create(_lockStrand->get_io_service());
#endif
	}
	assert(_lockStrand->running_in_this_thread());
	assert(us < 0x80000000LL * 1000);
	unsigned long long et = (get_tick_us() + us) & -256;
	timer_handle timerHandle;
	timerHandle._null = false;
	if (et >= _extMaxTick)
	{
		_extMaxTick = et;
		timerHandle._queueNode = _handlerQueue.insert(_handlerQueue.end(), make_pair(et, std::move(host)));
	}
	else
	{
		timerHandle._queueNode = _handlerQueue.insert(make_pair(et, std::move(host)));
	}
	
	if (!_looping)
	{//定时器已经退出循环，重新启动定时器
		_looping = true;
		assert(_handlerQueue.size() == 1);
		_extFinishTime = et;
		timer_loop(us);
	}
	else if (et < _extFinishTime)
	{//定时期限前于当前定时器期限，取消后重新计时
		boost::system::error_code ec;
		((timer_type*)_timer)->cancel(ec);
		_timerCount++;
		_extFinishTime = et;
		timer_loop(us);
	}
	return timerHandle;
}

void ActorTimer_::cancel(timer_handle& th)
{
	if (!th._null)
	{//删除当前定时器节点
		assert(_lockStrand && _lockStrand->running_in_this_thread());
		th._null = true;
		auto itNode = th._queueNode;
		if (_handlerQueue.size() == 1)
		{
			_extMaxTick = 0;
			_handlerQueue.erase(itNode);
			//如果没有定时任务就退出定时循环
			boost::system::error_code ec;
			((timer_type*)_timer)->cancel(ec);
			_timerCount++;
			_looping = false;
		} 
		else if (itNode->first == _extMaxTick)
		{
			_handlerQueue.erase(itNode++);
			if (_handlerQueue.end() == itNode)
			{
				itNode--;
			}
			_extMaxTick = itNode->first;
		}
		else
		{
			_handlerQueue.erase(itNode);
		}
	}
}

void ActorTimer_::timer_loop(unsigned long long us)
{
	int tc = ++_timerCount;
#ifdef DISABLE_BOOST_TIMER
	((timer_type*)_timer)->async_wait(micseconds(us), tc);
#else
	boost::system::error_code ec;
	((timer_type*)_timer)->expires_from_now(micseconds(us), ec);
	((timer_type*)_timer)->async_wait(_lockStrand->wrap_asio([this, tc](const boost::system::error_code&)
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
		unsigned long long nt = get_tick_us();
		while (!_handlerQueue.empty())
		{
			auto iter = _handlerQueue.begin();
			if (iter->first > nt + 500)
			{
				_extFinishTime = iter->first;
				timer_loop(iter->first - nt);
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