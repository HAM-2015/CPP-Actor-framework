#ifndef __RUN_THREAD_H
#define __RUN_THREAD_H

#include "try_move.h"
#include "scattered.h"
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <pthread.h>
#endif

class run_thread
{
	struct handler_face
	{
		virtual void invoke() = 0;
	};

	template <typename Handler>
	struct thread_handler : public handler_face
	{
		template <typename H>
		thread_handler(H&& h)
			:_h(TRY_MOVE(h)) {}

		void invoke()
		{
			_h();
			delete this;
		}

		Handler _h;
	};

public:
	struct thread_id
	{
		friend run_thread;

		bool operator<(const thread_id&) const;
		bool operator>(const thread_id&) const;
		bool operator<=(const thread_id&) const;
		bool operator>=(const thread_id&) const;
		bool operator==(const thread_id&) const;
		bool operator!=(const thread_id&) const;
	private:
#ifdef _WIN32
		DWORD _id;
#elif __linux__
		pthread_t _id;
#endif
	};
public:
	run_thread();

	template <typename Handler>
	run_thread(Handler&& h)
	{
		handler_face* handler = new thread_handler<RM_REF(Handler)>(TRY_MOVE(h));
#ifdef _WIN32
		_threadID = 0;
		_handle = ::CreateThread(NULL, 0, thread_exec, handler, 0, &_threadID);
#elif __linux__
		_pthread = 0;
		::pthread_create(&_pthread, NULL, thread_exec, handler);
#endif
	}

	~run_thread();
private:
#ifdef _WIN32
	static DWORD WINAPI thread_exec(LPVOID p);
#elif __linux__
	static void* thread_exec(void* p);
#endif
public:
	void join();
	thread_id get_id();
	void swap(run_thread& s);
	static thread_id this_thread_id();
private:
#ifdef _WIN32
	HANDLE _handle;
	DWORD _threadID;
#elif __linux__
	pthread_t _pthread;
#endif
	NONE_COPY(run_thread);
};

#endif