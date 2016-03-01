#ifndef __CONTEXT_POOL_H
#define __CONTEXT_POOL_H

#include <mutex>
#include <atomic>
#include <string>
#include <thread>
#include <condition_variable>
#include "msg_queue.h"
#include "context_yield.h"

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

		context_yield::context_info* _coroInfo;
	};

	struct coro_pull_interface
	{
		void operator()();

		context_yield::context_info* _coroInfo;
		coro_handler _currentHandler;
		void* _param;
		void* _space;
		int _tick;
#if (_DEBUG || DEBUG)
		size_t _spaceSize;
#endif
#if (WIN32 && (defined CHECK_SELF) && (_WIN32_WINNT >= 0x0502))
		static DWORD _actorFlsIndex;
#endif
	};
private:
	struct context_pool_pck
	{
		typedef msg_list_shared_alloc<coro_pull_interface*, pool_alloc_mt<void, mem_alloc_mt2<void, null_mutex> > > pool_queue;

		context_pool_pck()
		:_pool(*_alloc), _decommitPool(*_alloc){}
		pool_queue _pool;
		pool_queue _decommitPool;
		static std::mutex* _mutex;
		static pool_queue::shared_node_alloc* _alloc;
	};
public:
	ContextPool_();
	~ContextPool_();
public:
	static coro_pull_interface* getContext(size_t size);
	static void recovery(coro_pull_interface* coro);
	static void install();
	static void uninstall();
private:
	static void contextHandler(context_yield::context_info* info, void* param);
	void clearThread();
private:
	bool _exitSign;
	bool _clearWait;
	context_pool_pck _contextPool[256];
	std::mutex _clearMutex;
	std::thread _clearThread;
	std::atomic<int> _stackCount;
	std::condition_variable _clearVar;
	std::atomic<size_t> _stackTotalSize;
	static ContextPool_* _fiberPool;
};

#endif