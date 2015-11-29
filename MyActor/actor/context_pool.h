#ifndef __CONTEXT_POOL_H
#define __CONTEXT_POOL_H

#include <mutex>
#include <atomic>
#include <string>
#include <condition_variable>
#include <boost/thread.hpp>
#include "msg_queue.h"
#include "context_yield.h"

using namespace std;

/*!
@brief context³Ø
*/
class ContextPool_
{
public:
	struct coro_push_interface;
	typedef void(*coro_handler)(coro_push_interface& push, void* param);
public:
	struct coro_push_interface
	{
		void operator()();

		context_yield::coro_info* _coroInfo;
	};

	struct coro_pull_interface
	{
		void operator()();

		context_yield::coro_info* _coroInfo;
		coro_handler _currentHandler;
		void* _param;
		void* _space;
		int _tick;
#if (_DEBUG || DEBUG)
		size_t _spaceSize;
#endif
	};
private:
	struct context_pool_pck
	{
		context_pool_pck()
		:_pool(100000){}
		std::mutex _mutex;
		msg_list<coro_pull_interface*> _pool;
	};
public:
	ContextPool_();
	~ContextPool_();
public:
	static coro_pull_interface* getContext(size_t size);
	static void recovery(coro_pull_interface* coro);
private:
	static void contextHandler(context_yield::coro_info* info, void* param);
	void clearThread();
private:
	bool _exitSign;
	bool _clearWait;
	context_pool_pck _contextPool[256];
	std::mutex _clearMutex;
	boost::thread _clearThread;
	std::atomic<int> _stackCount;
	std::condition_variable _clearVar;
	std::atomic<size_t> _stackTotalSize;
};

#endif