#include "run_thread.h"

#ifdef _WIN32
run_thread::run_thread()
{
	_handle = NULL;
	_threadID = 0;
}

run_thread::~run_thread()
{
	if (_handle)
	{
		CloseHandle(_handle);
	}
}

DWORD WINAPI run_thread::thread_exec(LPVOID p)
{
	handler_face* handler = (handler_face*)p;
	handler->invoke();
	return 0;
}

void run_thread::join()
{
	if (_handle)
	{
		WaitForSingleObject(_handle, INFINITE);
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

size_t run_thread::cpu_core_number()
{
	size_t cores = 0;
	DWORD size = 0;

	GetLogicalProcessorInformation(NULL, &size);
	if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
	{
		return 0;
	}

	std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(size);
	if (GetLogicalProcessorInformation(&buffer.front(), &size) == FALSE)
	{
		return 0;
	}

	const size_t Elements = size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

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

#elif __linux__

#include <unistd.h>
#include <fstream>
#include <string>

run_thread::run_thread()
{
	_pthread = 0;
}

run_thread::~run_thread()
{

}

void* run_thread::thread_exec(void* p)
{
	handler_face* handler = (handler_face*)p;
	handler->invoke();
	return NULL;
}

void run_thread::join()
{
	if (_pthread)
	{
		pthread_join(_pthread, NULL);
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