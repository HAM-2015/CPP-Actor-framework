#include "time_out.h"
#include "ios_proxy.h"
#include "coro_stack.h"
#include "super_timer.h"
#include <boost/chrono/duration.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/bind.hpp>
#include <MMSystem.h>

#pragma comment( lib, "Winmm.lib" )

typedef LONG (__stdcall * NT_SET_TIMER_RESOLUTION)
	(
	IN ULONG DesiredTime,
	IN BOOLEAN SetResolution,
	OUT PULONG ActualTime
	);

typedef LONG (__stdcall * NT_QUERY_TIMER_RESOLUTION)
	(
	OUT PULONG MaximumTime,
	OUT PULONG MinimumTime,
	OUT PULONG CurrentTime
	);


class deadline_timer_trig: public timeout_trig
{
public:
	deadline_timer_trig(shared_strand st)
		:timeout_trig(st), _timer(st->get_io_service())
	{

	}
private:
	void expiresTimer()
	{
		boost::system::error_code ec;
		_timer.expires_from_now(_time, ec);
		assert(!ec);
		_timer.async_wait(_strand->wrap_post(boost::bind(&timeout_trig::timeoutHandler, shared_from_this(), ++_timeoutID, _1)));
	}

	void cancelTimer()
	{
		boost::system::error_code ec;
		_timer.cancel(ec);
		assert(!ec);
	}
private:
	boost::asio::basic_deadline_timer<boost::posix_time::ptime> _timer;
};

class system_timer_trig: public timeout_trig
{
public:
	system_timer_trig(shared_strand st)
		:timeout_trig(st), _timer(st->get_io_service())
	{

	}
private:
	void expiresTimer()
	{
		boost::system::error_code ec;
		_timer.expires_from_now(boost::chrono::microseconds(_time.total_microseconds()), ec);
		assert(!ec);
		_timer.async_wait(_strand->wrap_post(boost::bind(&timeout_trig::timeoutHandler, shared_from_this(), ++_timeoutID, _1)));
	}

	void cancelTimer()
	{
		boost::system::error_code ec;
		_timer.cancel(ec);
		assert(!ec);
	}
private:
	boost::asio::basic_waitable_timer<boost::chrono::system_clock> _timer;
};

class steady_timer_trig: public timeout_trig
{
public:
	steady_timer_trig(shared_strand st)
		:timeout_trig(st), _timer(st->get_io_service())
	{

	}
private:
	void expiresTimer()
	{
		boost::system::error_code ec;
		_timer.expires_from_now(boost::chrono::microseconds(_time.total_microseconds()), ec);
		assert(!ec);
		_timer.async_wait(_strand->wrap_post(boost::bind(&timeout_trig::timeoutHandler, shared_from_this(), ++_timeoutID, _1)));
	}

	void cancelTimer()
	{
		boost::system::error_code ec;
		_timer.cancel(ec);
		assert(!ec);
	}
private:
	boost::asio::basic_waitable_timer<boost::chrono::steady_clock> _timer;
};

class high_resolution_timer_trig: public timeout_trig
{
public:
	high_resolution_timer_trig(shared_strand st)
		:timeout_trig(st), _timer(st->get_io_service())
	{

	}
private:
	void expiresTimer()
	{
		boost::system::error_code ec;
		_timer.expires_from_now(boost::chrono::microseconds(_time.total_microseconds()), ec);
		assert(!ec);
		_timer.async_wait(_strand->wrap_post(boost::bind(&timeout_trig::timeoutHandler, shared_from_this(), ++_timeoutID, _1)));
	}

	void cancelTimer()
	{
		boost::system::error_code ec;
		_timer.cancel(ec);
		assert(!ec);
	}
private:
	boost::asio::basic_waitable_timer<boost::chrono::high_resolution_clock> _timer;
};

class super_resolution_timer_trig: public timeout_trig
{
public:
	super_resolution_timer_trig(shared_strand st)
		:timeout_trig(st), _timer(st)
	{

	}
private:
	void expiresTimer()
	{
		_timer.async_wait(_time.total_microseconds(), _strand->wrap_post(boost::bind(&timeout_trig::timeoutHandler, shared_from_this(), ++_timeoutID, _1)));
	}

	void cancelTimer()
	{
		boost::system::error_code ec;
		_timer.cancel(ec);
		assert(!ec);
	}
private:
	super_timer _timer;
};

//////////////////////////////////////////////////////////////////////////
timeout_trig::timeout_trig( shared_strand st )
	:_strand(st), _time(0)
{
	_timeoutID = 0;
	_completed = true;
	_suspend = false;
	_isMillisec = 1000;
}

timeout_trig::~timeout_trig()
{
	assert(_completed);
}

boost::shared_ptr<timeout_trig> timeout_trig::create(shared_strand st, timer_type type /* = deadline_timer */)
{
	switch (type)
	{
	case deadline_timer:
		return timeout_trig::create_deadline(st);
	case system_timer:
		return timeout_trig::create_system(st);
	case steady_timer:
		return timeout_trig::create_steady(st);
	case high_resolution_timer:
		return timeout_trig::create_high_resolution(st);
	case super_resolution_timer:
		return timeout_trig::create_super_resolution(st);
	}
	return boost::shared_ptr<timeout_trig>();
}

boost::shared_ptr<timeout_trig> timeout_trig::create_deadline( shared_strand st )
{
	boost::shared_ptr<timeout_trig> res(new deadline_timer_trig(st));
	return res;
}

boost::shared_ptr<timeout_trig> timeout_trig::create_steady( shared_strand st )
{
	boost::shared_ptr<timeout_trig> res(new steady_timer_trig(st));
	return res;
}

boost::shared_ptr<timeout_trig> timeout_trig::create_high_resolution( shared_strand st )
{
	boost::shared_ptr<timeout_trig> res(new high_resolution_timer_trig(st));
	return res;
}

boost::shared_ptr<timeout_trig> timeout_trig::create_super_resolution( shared_strand st )
{
	boost::shared_ptr<timeout_trig> res(new super_resolution_timer_trig(st));
	return res;
}

boost::shared_ptr<timeout_trig> timeout_trig::create_system( shared_strand st )
{
	boost::shared_ptr<timeout_trig> res(new system_timer_trig(st));
	return res;
}

boost::shared_ptr<timeout_trig> timeout_trig::create_in_pool(boost::shared_ptr<void> sharedPoolMem, void* objPtr, shared_strand st, timer_type tp)
{
	boost::shared_ptr<timeout_trig> res;
	switch (tp)
	{
	case deadline_timer:
		res = boost::shared_ptr<timeout_trig>(new(objPtr) deadline_timer_trig(st),
			boost::bind(&buffer_alloc<timeout_trig>::shared_obj_destroy, sharedPoolMem, _1));
		break;
	case system_timer:
		res = boost::shared_ptr<timeout_trig>(new(objPtr) system_timer_trig(st),
			boost::bind(&buffer_alloc<timeout_trig>::shared_obj_destroy, sharedPoolMem, _1));
		break;
	case steady_timer:
		res = boost::shared_ptr<timeout_trig>(new(objPtr) steady_timer_trig(st),
			boost::bind(&buffer_alloc<timeout_trig>::shared_obj_destroy, sharedPoolMem, _1));
		break;
	case high_resolution_timer:
		res = boost::shared_ptr<timeout_trig>(new(objPtr) high_resolution_timer_trig(st),
			boost::bind(&buffer_alloc<timeout_trig>::shared_obj_destroy, sharedPoolMem, _1));
		break;
	case super_resolution_timer:
		res = boost::shared_ptr<timeout_trig>(new(objPtr) super_resolution_timer_trig(st),
			boost::bind(&buffer_alloc<timeout_trig>::shared_obj_destroy, sharedPoolMem, _1));
		break;
	}
	return res;
}

size_t timeout_trig::object_size(timer_type tp)
{
	switch (tp)
	{
	case deadline_timer:
		return sizeof(deadline_timer_trig);
		break;
	case system_timer:
		return sizeof(system_timer_trig);
		break;
	case steady_timer:
		return sizeof(steady_timer_trig);
		break;
	case high_resolution_timer:
		return sizeof(high_resolution_timer_trig);
		break;
	case super_resolution_timer:
		return sizeof(super_resolution_timer_trig);
		break;
	}
	return -1;
}

void timeout_trig::enable_high_resolution()
{
	HMODULE hNtDll = LoadLibrary(TEXT("NtDll.dll"));
	if (hNtDll)
	{
		NT_QUERY_TIMER_RESOLUTION _NtQueryTimerResolution = (NT_QUERY_TIMER_RESOLUTION)GetProcAddress(hNtDll, "NtQueryTimerResolution");
		NT_SET_TIMER_RESOLUTION _NtSetTimerResolution = (NT_SET_TIMER_RESOLUTION)GetProcAddress(hNtDll, "NtSetTimerResolution");
		if (_NtQueryTimerResolution && _NtSetTimerResolution)
		{
			ULONG MaximumTime = 0;
			ULONG MinimumTime = 0;
			ULONG CurrentTime = 0;
			if (!_NtQueryTimerResolution(&MaximumTime, &MinimumTime, &CurrentTime))
			{
				ULONG ActualTime = 0;
				if (!_NtSetTimerResolution(MinimumTime, TRUE, &ActualTime))
				{
					FreeLibrary(hNtDll);
					return;
				}
			}
		}
		FreeLibrary(hNtDll);
	}
	timeBeginPeriod(1);
}

void timeout_trig::enable_realtime_priority()
{
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
}

long long timeout_trig::get_tick()
{
	return query_performance_counter::getTick();
}

void timeout_trig::timeOut( int ms, const boost::function<void ()>& h )
{
	assert(_strand->running_in_this_thread());
	assert(_completed);
	_h = h;
	unsigned long long tms;
	if (ms >= 0)
	{
		tms = (unsigned int)ms;
	} 
	else
	{
		tms = 0xFFFFFFFFFFFF;
	}
	_time = boost::posix_time::microsec(tms*_isMillisec);
	_completed = false;
	_stampBegin = boost::posix_time::microsec_clock::universal_time();
	if (!_suspend)
	{
		expiresTimer();
	}
	else
	{
		_stampEnd = _stampBegin;
	}
}

void timeout_trig::timeOutAgain(int ms, const boost::function<void ()>& h)
{
	cancel();
	timeOut(ms, h);
}

int timeout_trig::surplusTime()
{
	assert(_strand->running_in_this_thread());
	if (!_completed)
	{
		boost::posix_time::time_duration dt;
		if (_suspend)
		{
			dt = _stampEnd-_stampBegin;
		} 
		else
		{
			dt = boost::posix_time::microsec_clock::universal_time()-_stampBegin;
		}
		if (dt >= _time)
		{
			return 0;
		}
		auto st = _time - dt;
		if (st.total_milliseconds() <= 0x7FFFFFFF)
		{
			return (int)st.total_milliseconds();
		} 
	}
	return -1;
}

void timeout_trig::cancel()
{
	assert(_strand->running_in_this_thread());
	if (!_completed)
	{
		_completed = true;
		_h.clear();

		cancelTimer();
	}
}

void timeout_trig::suspend()
{
	assert(_strand->running_in_this_thread());
	if (!_suspend)
	{
		_suspend = true;
		if (!_completed)
		{
			cancelTimer();
			_stampEnd = boost::posix_time::microsec_clock::universal_time();
			auto tt = _stampBegin+_time;
			if (_stampEnd > tt)
			{
				_stampEnd = tt;
			}
		}
	}
}

void timeout_trig::resume()
{
	assert(_strand->running_in_this_thread());
	if (_suspend)
	{
		_suspend = false;
		if (!_completed)
		{
			assert(_time >= _stampEnd-_stampBegin);
			_time -= _stampEnd-_stampBegin;
			expiresTimer();
			_stampBegin = boost::posix_time::microsec_clock::universal_time();
		}
	}
}

void timeout_trig::trigger()
{
	assert(_strand->running_in_this_thread());
	if (!_completed)
	{
		_completed = true;
		cancelTimer();
		boost::function<void ()> h = _h;
		_h.clear();
		assert(h);
		h();
	}
}

void timeout_trig::timeoutHandler( int id, const boost::system::error_code& error )
{
	assert(_strand->running_in_this_thread());
	if (id == _timeoutID && !error)
	{
		if (!_suspend && !_completed)
		{
			_completed = true;
			boost::function<void ()> h = _h;
			_h.clear();
			assert(h);
			h();
		} 
	}
}

void timeout_trig::enableMicrosec()
{
	_isMillisec = 1;
}