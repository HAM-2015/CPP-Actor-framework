#include "actor_stack.h"
#include "scattered.h"
#include <chrono>
#include <assert.h>

//堆栈清理最小周期(秒)
#define STACK_MIN_CLEAR_CYCLE		30

#if (_DEBUG || DEBUG)
//测试堆栈释放后是否又被修改了
#define CHECK_STACK(__st__)\
{\
	size_t* p = (size_t*)((char*)(__st__)._stackTop - (__st__)._stackSize); \
	size_t length = (__st__)._stackSize / sizeof(size_t); \
	for (size_t i = 0; i < length; i++)\
	{\
		assert((size_t)0xCDCDCDCDCDCDCDCD == p[i]);\
	}\
}
#else
#define CHECK_STACK(__st__)
#endif

ActorStackPool_ _actorStackPool;

ActorStackPool_::ActorStackPool_()
:_exitSign(false), _clearWait(false), _stackCount(0), _stackTotalSize(0),
_clearThread(&ActorStackPool_::clearThread, this)
{
}

ActorStackPool_::~ActorStackPool_()
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
	vector<void*> tempPool;
	tempPool.resize(_stackCount);
	for (int i = 0; i < 256; i++)
	{
		std::lock_guard<std::mutex> lg1(_stackPool[i]._mutex);
		while (!_stackPool[i]._pool.empty())
		{
			StackPck_ pck = _stackPool[i]._pool.back();
			_stackPool[i]._pool.pop_back();
			tempPool[ic++] = ((char*)pck._stackTop) - pck._stackSize;
			CHECK_STACK(pck);
			_stackCount--;
		}
	}
	assert(0 == _stackCount);
	sort(tempPool.begin(), tempPool.end());
	for (int j = 0; j < (int)tempPool.size(); j++)
	{
		free(tempPool[j]);
	}
}

StackPck_ ActorStackPool_::getStack( size_t size )
{
	assert(size && size % 4096 == 0 && size <= 1024*1024);
	{
		stack_pool_pck& pool = _actorStackPool._stackPool[size/4096-1];
		pool._mutex.lock();
		if (!pool._pool.empty())
		{
			StackPck_ r = pool._pool.back();
			pool._pool.pop_back();
			pool._mutex.unlock();
			r._tick = 0;
			CHECK_STACK(r);
			return r;
		}
		pool._mutex.unlock();
	}
	_actorStackPool._stackCount++;
	_actorStackPool._stackTotalSize += size;
	StackPck_ sp = { ((char*)malloc(size)) + size, size, 0 };
	if (sp._stackTop)
	{
		assert(0 == ((size_t)sp._stackTop % sizeof(void*)));
		return sp;
	}
	throw std::shared_ptr<string>(new string("Actor栈内存不足"));
}

void ActorStackPool_::recovery( StackPck_& stack )
{
#if (_DEBUG || DEBUG)
	memset((char*)stack._stackTop-stack._stackSize, 0xCD, stack._stackSize);
#endif
	stack._tick = get_tick_s();
	stack_pool_pck& pool = _actorStackPool._stackPool[stack._stackSize/4096-1];
	std::lock_guard<std::mutex> lg(pool._mutex);
	pool._pool.push_back(stack);
}

void ActorStackPool_::clearThread()
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
			if (std::cv_status::no_timeout == _clearVar.wait_for(ul, std::chrono::seconds(STACK_MIN_CLEAR_CYCLE)))
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
				_stackPool[i]._mutex.lock();
				if (!_stackPool[i]._pool.empty() && extTick - _stackPool[i]._pool.front()._tick >= STACK_MIN_CLEAR_CYCLE)
				{
					StackPck_ pck = _stackPool[i]._pool.front();
					_stackPool[i]._pool.pop_front();
					_stackPool[i]._mutex.unlock();

					CHECK_STACK(pck);
					free(((char*)pck._stackTop) - pck._stackSize);
					_stackCount--;
					_stackTotalSize -= pck._stackSize;
					freeCount++;
				}
				else
				{
					_stackPool[i]._mutex.unlock();
				}
			}
		} while (freeCount);
	}
}