#include "super_timer.h"
#include <boost/bind.hpp>
#include <Windows.h>


long long _frequency = 0;

struct set_frequency 
{
	set_frequency()
	{
		LARGE_INTEGER frep;
		if (!QueryPerformanceFrequency(&frep))
		{
			return;
		}
		_frequency = frep.QuadPart;
	}
} _set;

super_timer::super_timer( shared_strand strand )
{
	_strand = strand;
	_hasTm = false;
	_idCount = 0;
}

super_timer::~super_timer()
{
	assert(!_hasTm);
}

void super_timer::async_wait( long long microsecond, const boost::function<void (boost::system::error_code)>& h )
{
	assert(_strand->running_in_this_thread());
	assert(!_hasTm);
	_hasTm = true;
	_idCount++;
	_nd = query_performance_counter::registTimer(_strand->wrap(
		boost::bind(&super_timer::notifyHandler, this, h, _idCount)), microsecond);
}

void super_timer::cancel(boost::system::error_code& ec)
{
	assert(_strand->running_in_this_thread());
	if (_hasTm)
	{
		_hasTm = false;
		ec.clear();
		query_performance_counter::cancelTimer(_nd);
	}
	else
	{
		ec = boost::system::error_code(1, boost::system::system_category());
	}
}

void super_timer::notifyHandler(const boost::function<void (boost::system::error_code)>& h, int id)
{
	assert(_strand->running_in_this_thread());
	if (_hasTm && id == _idCount)
	{
		_hasTm = false;
		query_performance_counter::cancelTimer(_nd);
		h(boost::system::error_code());
	}
}

//////////////////////////////////////////////////////////////////////////

query_performance_counter::query_performance_counter()
{
	_close = false;
}

query_performance_counter::~query_performance_counter()
{
	assert(_timerList.empty());
	_close = true;
	_timerThread.join();
}

bool query_performance_counter::enable()
{
	if (!_this)
	{
		_this = new query_performance_counter();
		_this->_timerThread.swap(boost::thread(boost::bind(&query_performance_counter::timerThread, _this)));
	}
	return true;
}

void query_performance_counter::close()
{
	if (_this)
	{
		delete _this;
		_this = NULL;
	}
}

list<super_timer::timer_pck>::iterator query_performance_counter::registTimer(const boost::function<void ()>& h, long long timeoutMicrosecond)
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);

	list<super_timer::timer_pck>::iterator nd;
	boost::lock_guard<boost::mutex> lg(_this->_mutex);
	_this->_timerList.push_front(super_timer::timer_pck(h, quadPart.QuadPart + (timeoutMicrosecond*_frequency)/1000000));
	nd = _this->_timerList.begin();
	return nd;
}

void query_performance_counter::cancelTimer(list<super_timer::timer_pck>::iterator nd)
{
	boost::lock_guard<boost::mutex> lg(_this->_mutex);
	_this->_timerList.erase(nd);
}

long long query_performance_counter::getTick()
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);
	return (long long)((double)quadPart.QuadPart*(1000000./(double)_frequency));
}

void query_performance_counter::timerThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
	LARGE_INTEGER quadPart;
	while (!_close)
	{
		QueryPerformanceCounter(&quadPart);

		boost::lock_guard<boost::mutex> lg(_mutex);
		for (auto it = _timerList.begin(); it != _timerList.end(); it++)
		{
			super_timer::timer_pck& pk = *it;
			if (pk._bNofify && quadPart.QuadPart >= pk._trigTick)
			{
				pk._bNofify = false;
				pk._h();
			}
		}
	}
}

query_performance_counter* query_performance_counter::_this = 0;