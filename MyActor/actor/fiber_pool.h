#ifndef __FIBER_POOL_H
#define __FIBER_POOL_H

#include "coro_choice.h"
#ifdef FIBER_CORO

#include <mutex>
#include <atomic>
#include <string>
#include <condition_variable>
#include <boost/thread.hpp>
#include "msg_queue.h"

using namespace std;

/*!
@brief fiber³Ø
*/
class FiberPool_
{
	struct FiberStruct_ 
	{
		void* _param;
		void* _seh;
		void* _stackTop;
		void* _stackBottom;
	};
public:
	struct coro_push_interface;
	typedef void(__stdcall * coro_handler)(coro_push_interface& push, void* param);
private:
	struct FiberPck_
	{
		size_t stackSize()
		{
			return (size_t)_fiberHandle->_stackTop - (size_t)_fiberHandle->_stackBottom;
		}

		coro_handler _currentHandler;
		FiberStruct_* _fiberHandle;
		void* _pushHandle;
		int _tick = 0;
	};
public:
	struct coro_push_interface
	{
		void operator()();

		FiberPck_* _fiber;
	};

	struct coro_pull_interface
	{
		void operator()();

		FiberPck_ _fiber;
	};
private:
	struct fiber_pool_pck
	{
		fiber_pool_pck()
		:_pool(100000){}
		std::mutex _mutex;
		msg_list<coro_pull_interface*> _pool;
	};
public:
	FiberPool_();
	~FiberPool_();
public:
	static coro_pull_interface* getFiber(size_t size, void** sp, coro_handler h, void* param = NULL);
	static void recovery(coro_pull_interface* coro);
private:
	static void __stdcall fiberHandler(void* p);
	void clearThread();
private:
	bool _exitSign;
	bool _clearWait;
	fiber_pool_pck _fiberPool[256];
	std::mutex _clearMutex;
	boost::thread _clearThread;
	std::atomic<int> _stackCount;
	std::condition_variable _clearVar;
	std::atomic<size_t> _stackTotalSize;
};
#endif

#endif