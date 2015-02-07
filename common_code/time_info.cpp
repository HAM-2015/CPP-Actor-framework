#include "time_info.h"
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

long long get_tick()
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);
	return (long long)((double)quadPart.QuadPart*(1000000./(double)_frequency));
}