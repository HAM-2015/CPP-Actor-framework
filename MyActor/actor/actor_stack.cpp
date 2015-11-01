#include "actor_stack.h"
#include "scattered.h"

//堆栈清理最小周期(秒)
#define STACK_MIN_CLEAR_CYCLE		30

#if (_DEBUG || DEBUG)
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

std::shared_ptr<ActorStackPool_> ActorStackPool_::_actorStackPool;

ActorStackPool_::ActorStackPool_()
{
	_exit = false;
	_clearWait = false;
	_stackCount = 0;
	_stackTotalSize = 0;
	boost::thread rh(&ActorStackPool_::clearThread, this);
	_clearThread.swap(rh);
}

ActorStackPool_::~ActorStackPool_()
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
			StackPck_ pck = _stackPool[i]._pool.back();
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

void ActorStackPool_::enable()
{
	_actorStackPool = std::shared_ptr<ActorStackPool_>(new ActorStackPool_());
}

bool ActorStackPool_::isEnable()
{
	return (bool)_actorStackPool;
}

StackPck_ ActorStackPool_::getStack( size_t size )
{
	assert(size && size % 4096 == 0 && size <= 1024*1024);
	{
		stack_pool_pck& pool = _actorStackPool->_stackPool[size/4096-1];
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
	_actorStackPool->_stackCount++;
	_actorStackPool->_stackTotalSize += size;
	StackPck_ r;
	r._tick = 0;
	r._stack.size = size;
	r._stack.sp = ((char*)malloc(size))+size;
	if (r._stack.sp)
	{
		return r;
	}
	throw std::shared_ptr<string>(new string("Actor栈内存不足"));
}

void ActorStackPool_::recovery( StackPck_& stack )
{
#if (_DEBUG || DEBUG)
	memset((char*)stack._stack.sp-stack._stack.size, 0xCD, stack._stack.size);
#endif
	stack._tick = get_tick_s();
	stack_pool_pck& pool = _actorStackPool->_stackPool[stack._stack.size/4096-1];
	boost::lock_guard<boost::mutex> lg(pool._mutex);
	pool._pool.push_back(stack);
}

void ActorStackPool_::clearThread()
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
					StackPck_ pck = _stackPool[i]._pool.front();
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

//////////////////////////////////////////////////////////////////////////

struct enable_stack_pool 
{
	enable_stack_pool()
	{
		ActorStackPool_::enable();
	}
}  _enable_stack_pool;