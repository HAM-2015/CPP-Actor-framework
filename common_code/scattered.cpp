#include "scattered.h"
#include <assert.h>
#include <Windows.h>

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

struct pc_cycle
{
	pc_cycle()
	{
		LARGE_INTEGER frep;
		if (!QueryPerformanceFrequency(&frep))
		{
			_sCycle = 0;
			_msCycle = 0;
			_usCycle = 0;
			assert(false);
			return;
		}
		_sCycle = 1.0/(double)frep.QuadPart;
		_msCycle = 1000.0/(double)frep.QuadPart;
		_usCycle = 1000000.0/(double)frep.QuadPart;
	}

	double _sCycle;
	double _msCycle;
	double _usCycle;
} _pcCycle;

void enable_high_resolution()
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

void enable_realtime_priority()
{
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
}

void set_priority(int p)
{
	SetPriorityClass(GetCurrentProcess(), p);
}

long long get_tick_us()
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);
	return (long long)((double)quadPart.QuadPart*_pcCycle._usCycle);
}

long long get_tick_ms()
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);
	return (long long)((double)quadPart.QuadPart*_pcCycle._msCycle);
}

int get_tick_s()
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);
	return (int)((double)quadPart.QuadPart*_pcCycle._sCycle);
}
//////////////////////////////////////////////////////////////////////////

void passing_test::operator=(passing_test&& s)
{
	assert(s._count);
	_count = s._count;
	_count->_moveCount++;
	s._count.reset();
	if (_count->_cb)
	{
		_count->_cb(_count);
	}
}

void passing_test::operator=(const passing_test& s)
{
	assert(s._count);
	_count = s._count;
	_count->_copyCount++;
	if (_count->_cb)
	{
		_count->_cb(_count);
	}
}

passing_test::passing_test(passing_test&& s)
{
	*this = std::move(s);
}

passing_test::passing_test(const passing_test& s)
{
	*this = s;
}

passing_test::passing_test(int id, const std::function<void(std::shared_ptr<count>)>& cb)
{
	_count = std::shared_ptr<count>(new count);
	_count->_copyCount = 0;
	_count->_moveCount = 0;
	_count->_id = id;
	_count->_cb = cb;
}

passing_test::passing_test(int id)
{
	_count = std::shared_ptr<count>(new count);
	_count->_copyCount = 0;
	_count->_moveCount = 0;
	_count->_id = id;
}

passing_test::~passing_test()
{

}
