#include "context_pool.h"
#include "scattered.h"
#include "actor_framework.h"
#if (WIN32 && __GNUG__)
#include <fibersapi.h>
#endif

//清理最小周期(秒)
#define CONTEXT_MIN_CLEAR_CYCLE		30
#define CONTEXT_MIN_DELETE_CYCLE		300

void ContextPool_::coro_push_interface::yield()
{
	context_yield::push_yield(_coroInfo);
}

void ContextPool_::coro_pull_interface::yield()
{
	context_yield::pull_yield(_coroInfo);
}

#if (WIN32 && (defined CHECK_SELF) && (_WIN32_WINNT >= 0x0502))
DWORD ContextPool_::coro_pull_interface::_actorFlsIndex = -1;
#endif

//////////////////////////////////////////////////////////////////////////

std::mutex* ContextPool_::context_pool_pck::_mutex = NULL;
ContextPool_::context_pool_pck::pool_queue::shared_node_alloc* ContextPool_::context_pool_pck::_alloc = NULL;
//////////////////////////////////////////////////////////////////////////

ContextPool_* ContextPool_::_fiberPool = NULL;

void ContextPool_::install()
{
	if (!_fiberPool)
	{
#if (WIN32 && (defined CHECK_SELF) && (_WIN32_WINNT >= 0x0502))
		ContextPool_::coro_pull_interface::_actorFlsIndex = ::FlsAlloc(NULL);
#endif
		ContextPool_::context_pool_pck::_mutex = new std::mutex;
		ContextPool_::context_pool_pck::_alloc = new context_pool_pck::pool_queue::shared_node_alloc(1000000);
		_fiberPool = new ContextPool_;
	}
}

void ContextPool_::uninstall()
{
	if (_fiberPool)
	{
		delete _fiberPool;
		delete ContextPool_::context_pool_pck::_mutex;
		ContextPool_::context_pool_pck::_mutex = NULL;
		delete ContextPool_::context_pool_pck::_alloc;
		ContextPool_::context_pool_pck::_alloc = NULL;
#if (WIN32 && (defined CHECK_SELF) && (_WIN32_WINNT >= 0x0502))
		::FlsFree(ContextPool_::coro_pull_interface::_actorFlsIndex);
		ContextPool_::coro_pull_interface::_actorFlsIndex = -1;
#endif
		_fiberPool = NULL;
	}
}

ContextPool_::ContextPool_()
:_exitSign(false), _clearWait(false), _stackCount(0), _stackTotalSize(0)
{
	run_thread th([this] { clearThread(); });
	_clearThread.swap(th);
}

ContextPool_::~ContextPool_()
{
	{
		std::lock_guard<std::mutex> lg(_clearMutex);
		_exitSign = true;
		if (_clearWait)
		{
			_clearWait = false;
			_clearVar.notify_one();
		}
	}
	_clearThread.join();

	int ic = 0;
	for (int i = 0; i < 256; i++)
	{
		std::lock_guard<std::mutex> lg1(*_contextPool[i]._mutex);
		while (!_contextPool[i]._pool.empty())
		{
			coro_pull_interface* const pull = _contextPool[i]._pool.back();
			_contextPool[i]._pool.pop_back();
			context_yield::context_info* const info = pull->_coroInfo;
			_stackCount--;
			_stackTotalSize -= info->stackSize + info->reserveSize;
			context_yield::delete_context(info);
			delete pull;
		}
		while (!_contextPool[i]._decommitPool.empty())
		{
			coro_pull_interface* const pull = _contextPool[i]._decommitPool.back();
			_contextPool[i]._decommitPool.pop_back();
			context_yield::context_info* const info = pull->_coroInfo;
			_stackCount--;
			_stackTotalSize -= info->stackSize + info->reserveSize;
			context_yield::delete_context(info);
			delete pull;
		}
	}
	assert(0 == _stackCount);
	assert(0 == _stackTotalSize);
}

ContextPool_::coro_pull_interface* ContextPool_::getContext(size_t size)
{
	assert(size && size % 4096 == 0 && size <= 1024 * 1024);
	assert(context_yield::is_thread_a_fiber());
	size = std::max(size, (size_t)CORO_CONTEXT_STATE_SPACE);
	do
	{
		{
			context_pool_pck& pool = _fiberPool->_contextPool[size / 4096 - 1];
			pool._mutex->lock();
			if (!pool._pool.empty())
			{
				coro_pull_interface* oldFiber = pool._pool.back();
				pool._pool.pop_back();
				pool._mutex->unlock();
				oldFiber->_tick = 0;
				return oldFiber;
			}
			if (!pool._decommitPool.empty())
			{
				coro_pull_interface* oldFiber = pool._decommitPool.back();
				pool._decommitPool.pop_back();
				pool._mutex->unlock();
				oldFiber->_tick = 0;
				return oldFiber;
			}
			pool._mutex->unlock();
		}
		coro_pull_interface* newFiber = new coro_pull_interface;
		newFiber->_tick = 0;
		newFiber->_coroInfo = context_yield::make_context(size, ContextPool_::contextHandler, newFiber);
		if (newFiber->_coroInfo)
		{
			_fiberPool->_stackCount++;
			_fiberPool->_stackTotalSize += newFiber->_coroInfo->stackSize + newFiber->_coroInfo->reserveSize;
			return newFiber;
		}
		delete newFiber;
		size += PAGE_SIZE;
	} while (size <= 1024 * 1024);
	return NULL;
}

void ContextPool_::recovery(coro_pull_interface* coro)
{
	coro->_tick = get_tick_s();
	context_pool_pck& pool = _fiberPool->_contextPool[coro->_coroInfo->stackSize / 4096 - 1];
	std::lock_guard<std::mutex> lg(*pool._mutex);
	pool._pool.push_back(coro);
}

void ContextPool_::contextHandler(context_yield::context_info* info, void* param)
{
	coro_pull_interface* pull = (coro_pull_interface*)param;
	char space[sizeof(my_actor)+64];
	pull->_space = space;
#if (_DEBUG || DEBUG)
	pull->_spaceSize = sizeof(space);
#endif
	assert((size_t)get_sp() > (size_t)info->stackTop - PAGE_SIZE + 256);
#if (WIN32 && (defined CHECK_SELF) && (_WIN32_WINNT >= 0x0502))
	::FlsSetValue(coro_pull_interface::_actorFlsIndex, (my_actor*)space);
#endif
	context_yield::push_yield(info);
	while (true)
	{
		{
			coro_push_interface push = { info };
			pull->_currentHandler(push, pull->_param);
		}
		context_yield::push_yield(info);
	}
}

void ContextPool_::clearThread()
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> ul(_clearMutex);
			if (_exitSign)
			{
				break;
			}
			_clearWait = true;
			if (std::cv_status::no_timeout == _clearVar.wait_for(ul, std::chrono::seconds(CONTEXT_MIN_CLEAR_CYCLE)))
			{
				break;
			}
			_clearWait = false;
		}
		size_t freeCount;
		goto _checkFree;
		do
		{
			{
				std::unique_lock<std::mutex> ul(_clearMutex);
				if (_exitSign)
				{
					break;
				}
				_clearWait = true;
				if (std::cv_status::no_timeout == _clearVar.wait_for(ul, std::chrono::milliseconds(1)))
				{
					break;
				}
				_clearWait = false;
			}
		_checkFree:;
			freeCount = 0;
			int extTick = get_tick_s();
			for (int i = 255; i >= 0; i--)
			{
				context_pool_pck& contextPool = _contextPool[i];
				contextPool._mutex->lock();
				if (!contextPool._pool.empty() && extTick - contextPool._pool.front()->_tick >= CONTEXT_MIN_CLEAR_CYCLE)
				{
					coro_pull_interface* const pull = contextPool._pool.front();
					contextPool._pool.pop_front();
					contextPool._mutex->unlock();
					context_yield::decommit_context(pull->_coroInfo);
					contextPool._mutex->lock();
					contextPool._decommitPool.push_front(pull);
					contextPool._mutex->unlock();
				}
				else if (!contextPool._decommitPool.empty() && extTick - contextPool._decommitPool.front()->_tick >= CONTEXT_MIN_DELETE_CYCLE)
				{
					coro_pull_interface* const pull = contextPool._decommitPool.front();
					contextPool._decommitPool.pop_front();
					contextPool._mutex->unlock();
					context_yield::context_info* const info = pull->_coroInfo;
					freeCount++;
					_stackCount--;
					_stackTotalSize -= info->stackSize + info->reserveSize;
					context_yield::delete_context(info);
					delete pull;
				}
				else
				{
					contextPool._mutex->unlock();
				}
			}
		} while (freeCount);
	}
}