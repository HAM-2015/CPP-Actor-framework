#include "context_pool.h"
#include "scattered.h"
#include "actor_framework.h"
#ifdef FIBER_CORO
#include <Windows.h>
#endif

//清理最小周期(秒)
#define CONTEXT_MIN_CLEAR_CYCLE		30

void ContextPool_::coro_push_interface::operator()()
{
	context_yield::push_yield(_coroInfo);
}

void ContextPool_::coro_pull_interface::operator()()
{
	context_yield::pull_yield(_coroInfo);
}
//////////////////////////////////////////////////////////////////////////

ContextPool_ s_fiberPool;

ContextPool_::ContextPool_()
:_exitSign(false), _clearWait(false), _stackCount(0), _stackTotalSize(0)
{
	boost::thread th(&ContextPool_::clearThread, this);
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
		std::lock_guard<std::mutex> lg1(_contextPool[i]._mutex);
		while (!_contextPool[i]._pool.empty())
		{
			coro_pull_interface* pull = _contextPool[i]._pool.back();
			_contextPool[i]._pool.pop_back();
			context_yield::delete_context(pull->_coroInfo);
			delete pull;
			_stackCount--;
		}
	}
	assert(0 == _stackCount);
}

ContextPool_::coro_pull_interface* ContextPool_::getContext(size_t size)
{
	assert(size && size % 4096 == 0 && size <= 1024 * 1024);
#ifdef FIBER_CORO
#if _WIN32_WINNT >= 0x0600
	if (!IsThreadAFiber())
	{
		ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
	}
#else//#elif _MSC_VER >= 0x0501
	ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
#endif
#endif
	{
		context_pool_pck& pool = s_fiberPool._contextPool[size / 4096 - 1];
		pool._mutex.lock();
		if (!pool._pool.empty())
		{
			coro_pull_interface* oldFiber = pool._pool.back();
			pool._pool.pop_back();
			pool._mutex.unlock();
			oldFiber->_tick = 0;
			return oldFiber;
		}
		pool._mutex.unlock();
	}
	s_fiberPool._stackCount++;
	s_fiberPool._stackTotalSize += size;
	coro_pull_interface* newFiber = new coro_pull_interface;
	newFiber->_tick = 0;
	newFiber->_coroInfo = context_yield::make_context(size, ContextPool_::contextHandler, newFiber);
	if (newFiber->_coroInfo)
	{
		return newFiber;
	}
	delete newFiber;
	throw std::shared_ptr<string>(new string("context 空间不足"));
}

void ContextPool_::recovery(coro_pull_interface* coro)
{
	coro->_tick = get_tick_s();
	context_pool_pck& pool = s_fiberPool._contextPool[coro->_coroInfo->stackSize / 4096 - 1];
	std::lock_guard<std::mutex> lg(pool._mutex);
	pool._pool.push_back(coro);
}

void ContextPool_::contextHandler(context_yield::coro_info* info, void* param)
{
	coro_pull_interface* pull = (coro_pull_interface*)param;
#ifdef _MSC_VER
#ifdef _WIN64
	char space[sizeof(my_actor)+40];
#else
	char space[sizeof(my_actor)+24];
#endif
#elif __GNUG__
#ifdef __x86_64__
	char space[sizeof(my_actor)+48];
#elif __i386__
	char space[sizeof(my_actor)+20];
#endif
#endif
	pull->_space = space;
#if (_DEBUG || DEBUG)
	pull->_spaceSize = sizeof(space);
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
				if (std::cv_status::no_timeout == _clearVar.wait_for(ul, std::chrono::milliseconds(100)))
				{
					break;
				}
				_clearWait = false;
			}
		_checkFree:;
			freeCount = 0;
			int extTick = get_tick_s();
			for (int i = 0; i < 256; i++)
			{
				_contextPool[i]._mutex.lock();
				if (!_contextPool[i]._pool.empty() && extTick - _contextPool[i]._pool.front()->_tick >= CONTEXT_MIN_CLEAR_CYCLE)
				{
					coro_pull_interface* pull = _contextPool[i]._pool.front();
					_contextPool[i]._pool.pop_front();
					_contextPool[i]._mutex.unlock();

					context_yield::delete_context(pull->_coroInfo);

					_stackCount--;
					_stackTotalSize -= pull->_coroInfo->stackSize;
					freeCount++;
					delete pull;
				}
				else
				{
					_contextPool[i]._mutex.unlock();
				}
			}
		} while (freeCount);
	}
}