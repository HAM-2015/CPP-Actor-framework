#include "scattered.h"
#include "run_thread.h"
#include <assert.h>
#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#include <MMSystem.h>
#endif
#include <boost/asio/io_service.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef WIN32
#ifdef _MSC_VER
#pragma comment( lib, "Winmm.lib" )
#endif
typedef LONG(__stdcall * NT_SET_TIMER_RESOLUTION)
(
IN ULONG DesiredTime,
IN BOOLEAN SetResolution,
OUT PULONG ActualTime
);

typedef LONG(__stdcall * NT_QUERY_TIMER_RESOLUTION)
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
		if (!::QueryPerformanceFrequency(&frep))
		{
			_sCycle = 0;
			_msCycle = 0;
			_usCycle = 0;
			assert(false);
			return;
		}
		_sCycle = 1.0 / (double)frep.QuadPart;
		_msCycle = 1000.0 / (double)frep.QuadPart;
		_usCycle = 1000000.0 / (double)frep.QuadPart;
	}

	double _sCycle;
	double _msCycle;
	double _usCycle;
} _pcCycle;
#endif

void move_test::operator=(move_test&& s)
{
	assert(s._count);
	_generation = s._generation + 1;
	_count = s._count;
	_count->_moveCount++;
	s._count.reset();
	if (_count->_cb)
	{
		_count->_cb(_count);
	}
}

void move_test::operator=(const move_test& s)
{
	assert(s._count);
	_generation = s._generation + 1;
	_count = s._count;
	_count->_copyCount++;
	if (_count->_cb)
	{
		_count->_cb(_count);
	}
}

std::ostream& operator <<(std::ostream& out, const move_test& s)
{
	if (s._count)
	{
		out << "(id:" << s._count->_id << ", generation:" << s._generation << ", move:" << s._count->_moveCount << ", copy:" << s._count->_copyCount << ")";
	}
	else
	{
		out << "(id:null, generation:null, move:null, copy:null)";
	}
	return out;
}

std::wostream& operator <<(std::wostream& out, const move_test& s)
{
	if (s._count)
	{
		out << L"(id:" << s._count->_id << L", generation:" << s._generation << L", move:" << s._count->_moveCount << L", copy:" << s._count->_copyCount << L")";
	}
	else
	{
		out << L"(id:null, generation:null, move:null, copy:null)";
	}
	return out;
}

move_test::move_test(move_test&& s)
{
	*this = std::move(s);
}

move_test::move_test(const move_test& s)
{
	*this = s;
}

move_test::move_test(int id, const std::function<void(std::shared_ptr<count>)>& cb)
{
	_count = std::shared_ptr<count>(new count);
	_count->_copyCount = 0;
	_count->_moveCount = 0;
	_count->_id = id;
	_count->_cb = cb;
	_generation = 0;
}

move_test::move_test(int id)
{
	_count = std::shared_ptr<count>(new count);
	_count->_copyCount = 0;
	_count->_moveCount = 0;
	_count->_id = id;
	_generation = 0;
}

move_test::move_test()
:_generation(0)
{

}

move_test::~move_test()
{

}
//////////////////////////////////////////////////////////////////////////

std::string get_time_string_us()
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u.%06u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds(), (int)time.fractional_seconds());
	return buff;
}

std::string get_time_string_ms()
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u.%03u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds(), (int)time.fractional_seconds() / 1000);
	return buff;
}

std::string get_time_string_s()
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds());
	return buff;
}

std::string get_time_string_file_s()
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u.%02u.%02u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds());
	return buff;
}

void print_time_us()
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	printf("%u-%02u-%02u %02u:%02u:%02u.%06u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds(), (int)time.fractional_seconds());
}

void print_time_ms()
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	printf("%u-%02u-%02u %02u:%02u:%02u.%03u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds(), (int)time.fractional_seconds() / 1000);
}

void print_time_s()
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	printf("%u-%02u-%02u %02u:%02u:%02u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds());
}

void print_time_us(std::ostream& out)
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u.%06u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds(), (int)time.fractional_seconds());
	out << buff;
}

void print_time_ms(std::ostream& out)
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u.%03u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds(), (int)time.fractional_seconds() / 1000);
	out << buff;
}

void print_time_s(std::ostream& out)
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds());
	out << buff;
}

void print_time_us(std::wostream& out)
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u.%06u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds(), (int)time.fractional_seconds());
	out << buff;
}

void print_time_ms(std::wostream& out)
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u.%03u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds(), (int)time.fractional_seconds() / 1000);
	out << buff;
}

void print_time_s(std::wostream& out)
{
	auto tm = boost::posix_time::microsec_clock::local_time();
	auto date = tm.date();
	auto time = tm.time_of_day();
	char buff[32];
	snPrintf(buff, sizeof(buff), "%u-%02u-%02u %02u:%02u:%02u", (int)date.year(), (int)date.month(), (int)date.day(), \
		(int)time.hours(), (int)time.minutes(), (int)time.seconds());
	out << buff;
}

#ifdef WIN32

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

#elif __linux__

void enable_high_resolution()
{
}

long long get_tick_us()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000000 + ts.tv_nsec/1000;
}

long long get_tick_ms()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000 + ts.tv_nsec/1000000;
}

int get_tick_s()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int)ts.tv_sec;
}

#endif

#ifdef __GNUG__

void* get_sp()
{
	void* result;
#ifdef __x86_64__
	__asm__("movq %%rsp, %0": "=r"(result));
#elif __i386__
	__asm__("movl %%esp, %0": "=r"(result));
#elif __arm__
	__asm__("mov %0, %%sp": "=r"(result));
#endif
	return result;
}

unsigned long long cpu_tick()
{
#if (defined __x86_64__) || (defined __i386__)
	unsigned int __a, __d;
	__asm__("rdtsc" : "=a" (__a), "=d" (__d));
	return ((unsigned long long)__a) | (((unsigned long long)__d) << 32);
#elif __arm__
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long long)ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
}
#endif
//////////////////////////////////////////////////////////////////////////

#ifdef WIN32
tls_space::tls_space()
{
	_index = TlsAlloc();
}

tls_space::~tls_space()
{
	TlsFree(_index);
}

void tls_space::set_space(void** val)
{
	TlsSetValue(_index, (LPVOID)val);
}

void** tls_space::get_space()
{
	return (void**)::TlsGetValue(_index);
}
#elif __linux__
tls_space::tls_space()
{
	pthread_key_create(&_key, NULL);
}

tls_space::~tls_space()
{
	pthread_key_delete(_key);
}

void tls_space::set_space(void** val)
{
	pthread_setspecific(_key, val);
}

void** tls_space::get_space()
{
	return (void**)::pthread_getspecific(_key);
}
#endif