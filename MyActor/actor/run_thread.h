#ifndef __RUN_THREAD_H
#define __RUN_THREAD_H

#include "try_move.h"
#include "scattered.h"
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <pthread.h>
#endif

/*!
@brief 线程对象
*/
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
		thread_handler(H&& handler)
			:_handler(std::forward<H>(handler)) {}

		void invoke()
		{
			CHECK_EXCEPTION(_handler);
			delete this;
		}

		Handler _handler;
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
	run_thread(Handler&& handler)
	{
		handler_face* handlerFace = new thread_handler<RM_CREF(Handler)>(std::forward<Handler>(handler));
#ifdef _WIN32
		_threadID = 0;
		_handle = CreateThread(NULL, 0, thread_exec, handlerFace, 0, &_threadID);
#elif __linux__
		_pthread = 0;
		pthread_create(&_pthread, NULL, thread_exec, handlerFace);
#endif
	}

	template <typename Handler, typename Callback>
	run_thread(Handler&& handler, Callback&& cb)
		:run_thread(std::bind([](Handler& handler, Callback& cb)
	{
		handler();
		cb();
	}, std::forward<Handler>(handler), std::forward<Callback>(cb))) {}

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
	static void set_current_thread_name(const char* name);
	static thread_id this_thread_id();
	static size_t cpu_core_number();
	static size_t cpu_thread_number();
	static void sleep(int ms);
private:
#ifdef _WIN32
	HANDLE _handle;
	DWORD _threadID;
#elif __linux__
	pthread_t _pthread;
#endif
	NONE_COPY(run_thread);
};

/*!
@brief 线程局部存储
*/
struct tls_space
{
	tls_space();
	~tls_space();
	void set_space(void** val);
	void** get_space();
private:
#ifdef WIN32
	DWORD _index;
#elif __linux__
	pthread_key_t _key;
#endif
};

#endif