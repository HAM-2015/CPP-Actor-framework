#include "coro_stack.h"
#include "shared_data.h"
#include "time_info.h"
#include <malloc.h>
#include <boost/thread/lock_guard.hpp>

//堆栈清理最小周期(秒)
#define STACK_MIN_CLEAR_CYCLE		30

boost::shared_ptr<coro_stack_pool> coro_stack_pool::_coroStackPool;

coro_stack_pool::coro_stack_pool()
{
	_exit = false;
	_clearWait = false;
	_isBack = false;
	_stackCount = 0;
	_stackTotalSize = 0;
	_nextPck._stack.sp = NULL;
	_stackPool.resize(256);
	for (size_t i = 0; i < _stackPool.size(); i++)
	{
		_stackPool[i] = new stack_pool_pck();
	}
	_clearThread.swap(boost::thread(&coro_stack_pool::clearThread, this));
}

coro_stack_pool::~coro_stack_pool()
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

	for (size_t mit = 0; mit < _stackPool.size(); mit++)
	{
		{
			boost::lock_guard<boost::mutex> lg1(_stackPool[mit]->_mutex);
			for (auto it = _stackPool[mit]->_pool.begin(); it != _stackPool[mit]->_pool.end(); it++)
			{
				free(((char*)it->_stack.sp)-it->_stack.size);
				_stackCount--;
			}
		}
		delete _stackPool[mit];
	}
	assert(0 == _stackCount);
}

void coro_stack_pool::enable()
{
	_coroStackPool = boost::shared_ptr<coro_stack_pool>(new coro_stack_pool());
}

bool coro_stack_pool::isEnable()
{
	return _coroStackPool;
}

stack_pck coro_stack_pool::getStack( size_t size )
{
	assert(size && size % 4096 == 0 && size <= 1024*1024);
	{
		stack_pool_pck* pool = _coroStackPool->_stackPool[size/4096-1];
		boost::lock_guard<boost::mutex> lg(pool->_mutex);
		if (!pool->_pool.empty())
		{
			stack_pck r = pool->_pool.back();
			pool->_pool.pop_back();
			r._tick = 0;
			if (!_coroStackPool->_isBack)
			{
				_coroStackPool->_isBack = (r._stack.sp == _coroStackPool->_nextPck._stack.sp);
			}
			return r;
		}
	}
	_coroStackPool->_stackCount++;
	_coroStackPool->_stackTotalSize += size;
	stack_pck r;
	r._tick = 0;
	r._stack.size = size;
	r._stack.sp = ((char*)malloc(size))+size;
	if (r._stack.sp)
	{
		return r;
	}
	throw boost::shared_ptr<string>(new string("协程栈内存不足"));
}

void coro_stack_pool::recovery( stack_pck& stack )
{
	stack._tick = get_tick_s();
	stack_pool_pck* pool = _coroStackPool->_stackPool[stack._stack.size/4096-1];
	boost::lock_guard<boost::mutex> lg(pool->_mutex);
	pool->_pool.push_back(stack);
}

void coro_stack_pool::clearThread()
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
			if (_clearVar.timed_wait(ul, boost::posix_time::millisec(STACK_MIN_CLEAR_CYCLE*1000+123)))
			{
				break;
			}
			_clearWait = false;
		}
		{
			int extTick = get_tick_s();
			for (size_t mit = 0; mit < _stackPool.size(); mit++)
			{
				_stackPool[mit]->_mutex.lock();
				_nextPck._stack.sp = NULL;
				auto it = _stackPool[mit]->_pool.begin();
				_isBack = _stackPool[mit]->_pool.end() == it;
				while (!_isBack)
				{
					if (extTick - it->_tick >= STACK_MIN_CLEAR_CYCLE)
					{
						stack_pck pck = *it;
						_stackPool[mit]->_pool.erase(it++);
						_isBack = _stackPool[mit]->_pool.end() == it;
						if (!_isBack)
						{
							_nextPck = *it;
						}
						_stackPool[mit]->_mutex.unlock();

						free(((char*)pck._stack.sp)-pck._stack.size);
						_stackCount--;
						_stackTotalSize -= pck._stack.size;

						_stackPool[mit]->_mutex.lock();
					}
					else
					{
						break;
					}
				}
				_stackPool[mit]->_mutex.unlock();
			}
		}
	}
}