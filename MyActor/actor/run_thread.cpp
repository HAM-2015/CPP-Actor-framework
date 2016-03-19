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
		::CloseHandle(_handle);
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
		::WaitForSingleObject(_handle, INFINITE);
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
	r._id = ::GetCurrentThreadId();
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

#elif __linux__

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
		::pthread_join(_pthread, NULL);
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
	r._id = ::pthread_self();
	return r;
}

void run_thread::swap(run_thread& s)
{
	if (this != &s)
	{
		std::swap(_pthread, s._pthread);
	}
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