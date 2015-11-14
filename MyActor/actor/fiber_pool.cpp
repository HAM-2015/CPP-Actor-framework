#include "coro_choice.h"
#ifdef FIBER_CORO

#include "fiber_pool.h"
#include "scattered.h"
#include <Windows.h>

//fiber清理最小周期(秒)
#define FIBER_MIN_CLEAR_CYCLE		30

void FiberPool_::coro_push_interface::operator()()
{
	SwitchToFiber(_fiber->_pushHandle);
}

void FiberPool_::coro_pull_interface::operator()()
{
	_fiber._pushHandle = GetCurrentFiber();
	SwitchToFiber(_fiber._fiberHandle);
}
//////////////////////////////////////////////////////////////////////////

FiberPool_ s_fiberPool;

FiberPool_::FiberPool_()
:_exitSign(false), _clearWait(false), _stackCount(0), _stackTotalSize(0)
{
	boost::thread th(&FiberPool_::clearThread, this);
	_clearThread.swap(th);
}

FiberPool_::~FiberPool_()
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
		std::lock_guard<std::mutex> lg1(_fiberPool[i]._mutex);
		while (!_fiberPool[i]._pool.empty())
		{
			coro_pull_interface* pull = _fiberPool[i]._pool.back();
			_fiberPool[i]._pool.pop_back();
			DeleteFiber(pull->_fiber._fiberHandle);
			delete pull;
			_stackCount--;
		}
	}
	assert(0 == _stackCount);
}

FiberPool_::coro_pull_interface* FiberPool_::getFiber(size_t size, void** sp, coro_handler h, void* param /*= NULL*/)
{
	assert(size && size % 4096 == 0 && size <= 1024 * 1024);
	void* currentFiber = NULL;
	if (IsThreadAFiber())
	{
		currentFiber = GetCurrentFiber();
	}
	else
	{
		currentFiber = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
	}
	{
		fiber_pool_pck& pool = s_fiberPool._fiberPool[size / 4096 - 1];
		pool._mutex.lock();
		if (!pool._pool.empty())
		{
			coro_pull_interface* oldFiber = pool._pool.back();
			pool._pool.pop_back();
			pool._mutex.unlock();
			oldFiber->_fiber._pushHandle = currentFiber;
			oldFiber->_fiber._currentHandler = h;
			oldFiber->_fiber._fiberHandle->_param = param;
			oldFiber->_fiber._tick = 0;
			*sp = oldFiber->_fiber._fiberHandle->_stackTop;
			SwitchToFiber(oldFiber->_fiber._fiberHandle);
			return oldFiber;
		}
		pool._mutex.unlock();
	}
	s_fiberPool._stackCount++;
	s_fiberPool._stackTotalSize += size;
	coro_pull_interface* newFiber = new coro_pull_interface;
	newFiber->_fiber._fiberHandle = (FiberStruct_*)CreateFiberEx(size, 0, FIBER_FLAG_FLOAT_SWITCH, FiberPool_::fiberHandler, newFiber);
	if (newFiber->_fiber._fiberHandle)
	{
		newFiber->_fiber._pushHandle = currentFiber;
		newFiber->_fiber._currentHandler = h;
		newFiber->_fiber._fiberHandle->_param = param;
		newFiber->_fiber._tick = 0;
		*sp = newFiber->_fiber._fiberHandle->_stackTop;
		SwitchToFiber(newFiber->_fiber._fiberHandle);
		return newFiber;
	}
	delete newFiber;
	throw std::shared_ptr<string>(new string("Fiber不足"));
}

void FiberPool_::recovery(coro_pull_interface* coro)
{
	coro->_fiber._tick = get_tick_s();
	fiber_pool_pck& pool = s_fiberPool._fiberPool[coro->_fiber.stackSize() / 4096 - 1];
	std::lock_guard<std::mutex> lg(pool._mutex);
	pool._pool.push_back(coro);
}

void __stdcall FiberPool_::fiberHandler(void* p)
{
	coro_pull_interface* pull = (coro_pull_interface*)p;
	FiberPck_& fiber_ = pull->_fiber;
	while (true)
	{
		{
			coro_push_interface push = { &fiber_ };
			fiber_._currentHandler(push, fiber_._fiberHandle->_param);
		}
		SwitchToFiber(fiber_._pushHandle);
	}
}

void FiberPool_::clearThread()
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
			if (std::cv_status::no_timeout == _clearVar.wait_for(ul, std::chrono::seconds(FIBER_MIN_CLEAR_CYCLE)))
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
				_fiberPool[i]._mutex.lock();
				if (!_fiberPool[i]._pool.empty() && extTick - _fiberPool[i]._pool.front()->_fiber._tick >= FIBER_MIN_CLEAR_CYCLE)
				{
					coro_pull_interface* pull = _fiberPool[i]._pool.front();
					_fiberPool[i]._pool.pop_front();
					_fiberPool[i]._mutex.unlock();

					DeleteFiber(pull->_fiber._fiberHandle);

					_stackCount--;
					_stackTotalSize -= pull->_fiber.stackSize();
					freeCount++;
					delete pull;
				}
				else
				{
					_fiberPool[i]._mutex.unlock();
				}
			}
		} while (freeCount);
	}
}
#endif