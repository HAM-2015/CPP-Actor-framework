#include "actor_timer.h"
#include "scattered.h"

actor_timer::actor_timer(shared_strand strand)
:_ios(strand->get_ios_proxy()), _looping(false), _weakStrand(strand), _timerCount(0),
_extTimerFinish(-1), _timer((timer_type*)_ios.getTimer()), _listAlloc(4096), _handlerTable(4096)
{

}

actor_timer::~actor_timer()
{
	assert(_handlerTable.empty());
	assert(!_strand);
	_ios.freeTimer(_timer);
}

actor_timer::timer_handle actor_timer::time_out(unsigned long long us, const std::function<void()>& h)
{
	if (!_strand)
	{
		_strand = _weakStrand.lock();
	}
	assert(_strand->running_in_this_thread());
	unsigned long long et = get_tick_us() + us;
	auto node = _handlerTable.insert(make_pair(et, handler_list()));
	auto& nl = node.first->second;
	if (!nl)
	{
		nl = handler_list(new msg_list<call_back, list_alloc>(_listAlloc));
	}

	nl->push_front(h);
	timer_handle timerHandle;
	timerHandle._handlerList = nl;
	timerHandle._handlerNode = nl->begin();
	timerHandle._tableNode = node.first;
	
	if (!_looping)
	{
		_looping = true;
		assert(_handlerTable.size() == 1);
		_extTimerFinish = et;
		timer_loop(us);
	}
	else if (et < _extTimerFinish)
	{
		boost::system::error_code ec;
		_timer->cancel(ec);
		_timerCount++;
		_extTimerFinish = et;
		timer_loop(us);
	}
	return timerHandle;
}

void actor_timer::cancel(timer_handle& th)
{
	assert(_strand->running_in_this_thread());
	auto hl = th._handlerList.lock();
	if (hl)
	{
		hl->erase(th._handlerNode);
		if (hl->empty())
		{
			_handlerTable.erase(th._tableNode);
			if (_handlerTable.empty())
			{
				boost::system::error_code ec;
				_timer->cancel(ec);
				_timerCount++;
				_looping = false;
			}
		}
	}
}

void actor_timer::timer_loop(unsigned long long us)
{
	boost::system::error_code ec;
	_timer->expires_from_now(boost::chrono::microseconds(us), ec);
	size_t tc = ++_timerCount;
	_timer->async_wait(_strand->wrap_post([this, tc](const boost::system::error_code&)
	{
		if (tc == _timerCount)
		{
			_extTimerFinish = 0;
			unsigned long long nt = get_tick_us();
			while (!_handlerTable.empty())
			{
				auto iter = _handlerTable.begin();
				if (iter->first > nt + 500)
				{
					_extTimerFinish = iter->first;
					timer_loop(iter->first - nt);
					return;
				}
				else
				{
					handler_list hl;
					iter->second.swap(hl);
					_handlerTable.erase(iter);
					for (auto it = hl->begin(); it != hl->end(); it++)
					{
						(*it)();
					}
				}
			}
			_looping = false;
			_strand.reset();
		}
		else if (tc == _timerCount - 1)
		{
			_strand.reset();
		}
	}));
}