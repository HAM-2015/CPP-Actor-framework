#include "actor_stack.h"
#include "shared_data.h"
#include "scattered.h"

//堆栈清理最小周期(秒)
#define STACK_MIN_CLEAR_CYCLE		30

#ifdef _DEBUG
//测试堆栈释放后是否又被修改了
#define CHECK_STACK(__st__)\
{\
	size_t* p = (size_t*)((char*)(__st__)._stack.sp - (__st__)._stack.size); \
	size_t length = (__st__)._stack.size / sizeof(size_t); \
	for (size_t i = 0; i < length; i++)\
	{\
		assert((size_t)0xCDCDCDCDCDCDCDCD == p[i]);\
	}\
}
#else
#define CHECK_STACK(__st__)
#endif

std::shared_ptr<actor_stack_pool> actor_stack_pool::_actorStackPool;

actor_stack_pool::actor_stack_pool()
{
	_exit = false;
	_clearWait = false;
	_stackCount = 0;
	_stackTotalSize = 0;
	boost::thread rh(&actor_stack_pool::clearThread, this);
	_clearThread.swap(rh);
}

actor_stack_pool::~actor_stack_pool()
{
	{
		boost::lock_guard<boost::mutex> lg(_clearMutex);
		_exit = true;
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
		boost::lock_guard<boost::mutex> lg1(_stackPool[i]._mutex);
		while (!_stackPool[i]._pool.empty())
		{
			stack_pck pck = _stackPool[i]._pool.back();
			_stackPool[i]._pool.pop_back();
			tempPool[ic++] = ((char*)pck._stack.sp) - pck._stack.size;
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

void actor_stack_pool::enable()
{
	_actorStackPool = std::shared_ptr<actor_stack_pool>(new actor_stack_pool());
}

bool actor_stack_pool::isEnable()
{
	return (bool)_actorStackPool;
}

stack_pck actor_stack_pool::getStack( size_t size )
{
	assert(size && size % 4096 == 0 && size <= 1024*1024);
	{
		stack_pool_pck& pool = _actorStackPool->_stackPool[size/4096-1];
		pool._mutex.lock();
		if (!pool._pool.empty())
		{
			stack_pck r = pool._pool.back();
			pool._pool.pop_back();
			pool._mutex.unlock();
			r._tick = 0;
			CHECK_STACK(r);
			return r;
		}
		pool._mutex.unlock();
	}
	_actorStackPool->_stackCount++;
	_actorStackPool->_stackTotalSize += size;
	stack_pck r;
	r._tick = 0;
	r._stack.size = size;
	r._stack.sp = ((char*)malloc(size))+size;
	if (r._stack.sp)
	{
		return r;
	}
	throw std::shared_ptr<string>(new string("Actor栈内存不足"));
}

void actor_stack_pool::recovery( stack_pck& stack )
{
#ifdef _DEBUG
	memset((char*)stack._stack.sp-stack._stack.size, 0xCD, stack._stack.size);
#endif
	stack._tick = get_tick_s();
	stack_pool_pck& pool = _actorStackPool->_stackPool[stack._stack.size/4096-1];
	boost::lock_guard<boost::mutex> lg(pool._mutex);
	pool._pool.push_back(stack);
}

void actor_stack_pool::clearThread()
{
	while (true)
	{
		{
			boost::unique_lock<boost::mutex> ul(_clearMutex);
			if (_exit)
			{
				break;
			}
			_clearWait = true;
			if (_clearVar.timed_wait(ul, boost::posix_time::seconds(STACK_MIN_CLEAR_CYCLE)))
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
				boost::unique_lock<boost::mutex> ul(_clearMutex);
				if (_exit)
				{
					break;
				}
				_clearWait = true;
				if (_clearVar.timed_wait(ul, boost::posix_time::millisec(100)))
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
					stack_pck pck = _stackPool[i]._pool.front();
					_stackPool[i]._pool.pop_front();
					_stackPool[i]._mutex.unlock();

					CHECK_STACK(pck);
					free(((char*)pck._stack.sp) - pck._stack.size);
					_stackCount--;
					_stackTotalSize -= pck._stack.size;
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