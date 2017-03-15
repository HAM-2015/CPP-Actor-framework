#include "run_thread.h"

#ifdef _WIN32
run_thread::run_thread()
{
	_handle = NULL;
	_threadID = 0;
}

run_thread::~run_thread()
{
	assert(!_handle);
}

DWORD WINAPI run_thread::thread_exec(LPVOID p)
{
	as_ptype<handler_face>(p)->invoke();
	return 0;
}

void run_thread::join()
{
	assert(this_thread_id() != get_id());
	if (_handle)
	{
		WaitForSingleObject(_handle, INFINITE);
		CloseHandle(_handle);
		_handle = NULL;
		_threadID = 0;
	}
}

run_thread::thread_id run_thread::get_id()
{
	thread_id r;
	r._id = _threadID;
	return r;
}

run_thread::thread_id run_thread::this_thread_id()
{
	thread_id r;
	r._id = GetCurrentThreadId();
	return r;
}

void run_thread::swap(run_thread& s)
{
	if (this != &s)
	{
		std::swap(_handle, s._handle);
		std::swap(_threadID, s._threadID);
	}
}

void run_thread::set_current_thread_name(const char* name)
{
	//https://msdn.microsoft.com/en-us/library/xcb2z8hs(v=VS.71).aspx
	struct
	{
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} info = { 0x1000, name, GetCurrentThreadId(), 0 };
#ifdef _MSC_VER
	__try { RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info); }
	__except (EXCEPTION_EXECUTE_HANDLER) {}
#elif (__GNUG__ && ENABLE_GCC_SEH)
	__seh_try { RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info); }
	__seh_except(EXCEPTION_EXECUTE_HANDLER) {}
	__seh_end_except
#endif
}

size_t run_thread::cpu_core_number()
{
	DWORD size = 0;

	GetLogicalProcessorInformation(NULL, &size);
	if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
	{
		return 0;
	}

	const size_t Elements = size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
	std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(Elements);
	if (GetLogicalProcessorInformation(&buffer.front(), &size) == FALSE)
	{
		return 0;
	}

	size_t cores = 0;
	for (size_t i = 0; i < Elements; ++i)
	{
		if (buffer[i].Relationship == RelationProcessorCore)
		{
			cores++;
		}
	}
	return cores;
}

size_t run_thread::cpu_thread_number()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return (size_t)info.dwNumberOfProcessors;
}

void run_thread::sleep(int ms)
{
	Sleep(ms);
}

bool run_thread::set_affinity(int i)
{
	return mask_affinity((unsigned long long)1 << i);
}

bool run_thread::mask_affinity(unsigned long long mask)
{
	assert(_handle);
	assert(mask && 0 == (mask & (mask - 1)));
	return 0 != SetThreadAffinityMask(_handle, (DWORD_PTR)mask);
}

bool run_thread::set_ideal(int i)
{
	assert(_handle);
	assert(0 <= i);
	return (DWORD)-1 != SetThreadIdealProcessor(_handle, (DWORD)i);
}
//////////////////////////////////////////////////////////////////////////

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
	return (void**)TlsGetValue(_index);
}
#elif __linux__

#include <unistd.h>
#include <fstream>
#include <string>
#include <sys/prctl.h>
#include <sched.h>

run_thread::run_thread()
{
	_pthread = 0;
}

run_thread::~run_thread()
{
	assert(!_pthread);
}

void* run_thread::thread_exec(void* p)
{
	as_ptype<handler_face>(p)->invoke();
	return NULL;
}

void run_thread::join()
{
	assert(this_thread_id() != get_id());
	if (_pthread)
	{
		pthread_join(_pthread, NULL);
		_pthread = 0;
	}
}

run_thread::thread_id run_thread::get_id()
{
	thread_id r;
	r._id = _pthread;
	return r;
}

run_thread::thread_id run_thread::this_thread_id()
{
	thread_id r;
	r._id = pthread_self();
	return r;
}

void run_thread::swap(run_thread& s)
{
	if (this != &s)
	{
		std::swap(_pthread, s._pthread);
	}
}

void run_thread::set_current_thread_name(const char* name)
{
	prctl(PR_SET_NAME, name);
}

size_t run_thread::cpu_core_number()
{
	try
	{
		std::ifstream proc_cpuinfo ("/proc/cpuinfo");
		const std::string physical_id("physical id");
		const std::string core_id("core id");
		size_t cores = 0;
		std::string line;
		while (getline(proc_cpuinfo, line))
		{
			if (!line.empty())
			{
				const size_t i = line.find(':');
				if (i >= line.size())
				{
					return sysconf(_SC_NPROCESSORS_CONF);
				}
				if (i >= core_id.size())
				{
					const size_t j = line.find(core_id);
					if (j < line.size())
					{
						size_t k = 0;
						for (; k < j; k++)
						{
							if ('\t' != line[k] && ' ' != line[k])
							{
								break;
							}
						}
						if (k == j)
						{
							for (k += core_id.size(); k < i; k++)
							{
								if ('\t' != line[k] && ' ' != line[k])
								{
									break;
								}
							}
							if (k == i)
							{
								cores++;
							}
						}
					}
				}
			}
		}
		return 0 != cores ? cores : sysconf(_SC_NPROCESSORS_CONF);
	}
	catch (...)
	{
		return sysconf(_SC_NPROCESSORS_CONF);
	}
}

size_t run_thread::cpu_thread_number()
{
	return (size_t)sysconf(_SC_NPROCESSORS_ONLN);
}

void run_thread::sleep(int ms)
{
	if (ms)
	{
		usleep((__useconds_t)ms * 1000);
	}
	else
	{
		sched_yield();
	}
}

bool run_thread::set_affinity(int i)
{
	return mask_affinity((unsigned long long)1 << i);
}

bool run_thread::mask_affinity(unsigned long long mask)
{
	assert(_pthread);
	assert(mask && 0 == (mask & (mask - 1)));
	cpu_set_t cpumask;
	if (0 == pthread_getaffinity_np(_pthread, sizeof(cpumask), &cpumask))
	{
		for (size_t i = 0; i < fixed_array_length(cpumask.__bits); i++)
		{
			if (cpumask.__bits[i])
			{
				CPU_ZERO(&cpumask);
				cpumask.__bits[i] = (__cpu_mask)mask;
				return 0 == pthread_setaffinity_np(_pthread, sizeof(cpumask), &cpumask);
			}
		}
	}
	return false;
}

bool run_thread::set_ideal(int i)
{
	assert(_pthread);
	cpu_set_t cpumask;
	assert(0 <= i && fixed_array_length(cpumask.__bits) > (size_t)i);
	CPU_ZERO(&cpumask);
	cpumask.__bits[i] = 255;
	return 0 == pthread_setaffinity_np(_pthread, sizeof(cpumask), &cpumask);
}
//////////////////////////////////////////////////////////////////////////

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
	return (void**)pthread_getspecific(_key);
}
#endif

bool run_thread::thread_id::operator<(const thread_id& s) const
{
	return _id < s._id;
}

bool run_thread::thread_id::operator>(const thread_id& s) const
{
	return _id > s._id;
}

bool run_thread::thread_id::operator<=(const thread_id& s) const
{
	return _id <= s._id;
}

bool run_thread::thread_id::operator>=(const thread_id& s) const
{
	return _id >= s._id;
}
bool run_thread::thread_id::operator==(const thread_id& s) const
{
	return _id == s._id;
}

bool run_thread::thread_id::operator!=(const thread_id& s) const
{
	return _id != s._id;
}