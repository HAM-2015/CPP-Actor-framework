#include "async_timer.h"
#include "scattered.h"
#include "io_engine.h"
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

timer_boost::timer_boost(const shared_strand& strand)
:_ios(strand->get_io_engine()), _strand(strand), _looping(false), _timerCount(0),
_extMaxTick(0), _extFinishTime(-1), _timer(_ios.getTimer()), _handlerTable(65536)
{

}

timer_boost::~timer_boost()
{
	assert(_handlerTable.empty());
	_ios.freeTimer(_timer);
}

std::shared_ptr<timer_boost> timer_boost::create(const shared_strand& strand)
{
	std::shared_ptr<timer_boost> res(new timer_boost(strand));
	res->_weakThis = res;
	return res;
}

const shared_strand& timer_boost::self_strand()
{
	return _strand;
}

timer_boost::timer_handle timer_boost::timeout(unsigned long long us, const async_handle& host)
{
	assert(_strand->running_in_this_thread());
	assert(us < 0x80000000LL * 1000);
	unsigned long long et = (get_tick_us() + us) & -256;
	timer_handle timerHandle;
	timerHandle._null = false;
	if (et >= _extMaxTick)
	{
		_extMaxTick = et;
		timerHandle._tableNode = _handlerTable.insert(_handlerTable.end(), make_pair(et, host));
	}
	else
	{
		timerHandle._tableNode = _handlerTable.insert(make_pair(et, host));
	}

	if (!_looping)
	{
		_looping = true;
		assert(_handlerTable.size() == 1);
		_extFinishTime = et;
		timer_loop(us);
	}
	else if (et < _extFinishTime)
	{
		boost::system::error_code ec;
		((timer_type*)_timer)->cancel(ec);
		_timerCount++;
		_extFinishTime = et;
		timer_loop(us);
	}
	return timerHandle;
}

void timer_boost::cancel(timer_handle& th)
{
	if (!th._null)
	{
		assert(_strand && _strand->running_in_this_thread());
		th._null = true;
		auto itNode = th._tableNode;
		if (_handlerTable.size() == 1)
		{
			_extMaxTick = 0;
			_handlerTable.erase(itNode);
			boost::system::error_code ec;
			((timer_type*)_timer)->cancel(ec);
			_timerCount++;
			_looping = false;
		}
		else if (itNode->first == _extMaxTick)
		{
			_handlerTable.erase(itNode++);
			if (_handlerTable.end() == itNode)
			{
				itNode--;
			}
			_extMaxTick = itNode->first;
		}
		else
		{
			_handlerTable.erase(itNode);
		}
	}
}

void timer_boost::timer_loop(unsigned long long us)
{
	int tc = ++_timerCount;
	auto sharedThis = _weakThis.lock();
	boost::system::error_code ec;
	((timer_type*)_timer)->expires_from_now(micseconds(us), ec);
	((timer_type*)_timer)->async_wait(_strand->wrap_asio([sharedThis, tc](const boost::system::error_code&)
	{
		timer_boost* this_ = sharedThis.get();
		assert(this_->_strand->running_in_this_thread());
		if (tc == this_->_timerCount)
		{
			this_->_extFinishTime = 0;
			unsigned long long nt = get_tick_us();
			while (!this_->_handlerTable.empty())
			{
				auto iter = this_->_handlerTable.begin();
				if (iter->first > nt + 500)
				{
					this_->_extFinishTime = iter->first;
					this_->timer_loop(iter->first - nt);
					return;
				}
				else
				{
					iter->second->timeout_handler();
					this_->_handlerTable.erase(iter);
				}
			}
			this_->_looping = false;
		}
	}));
}
//////////////////////////////////////////////////////////////////////////

async_timer::async_timer(const std::shared_ptr<timer_boost>& timer)
:_timer(timer), _handler(NULL)
{

}

async_timer::~async_timer()
{
	assert(!_handler);
}

std::shared_ptr<async_timer> async_timer::create(const std::shared_ptr<timer_boost>& timer)
{
	std::shared_ptr<async_timer> res(new async_timer(timer));
	res->_weakThis = res;
	return res;
}

void async_timer::cancel()
{
	if (_handler)
	{
		_handler->destory(_reuMem);
		_handler = NULL;
	}
	_timer->cancel(_timerHandle);
}

const shared_strand& async_timer::self_strand()
{
	return _timer->_strand;
}

const std::shared_ptr<timer_boost>& async_timer::self_timer()
{
	return _timer;
}

void async_timer::timeout_handler()
{
	_timerHandle.reset();
	wrap_base* cb = _handler;
	_handler = NULL;
	cb->invoke(_reuMem);
}