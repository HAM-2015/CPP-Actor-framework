#include "coro_stack.h"
#include "shared_data.h"
#include <boost/thread/lock_guard.hpp>

coro_stack_pool::coro_stack_pool()
{
	_stackCount = 0;
	_stackPool.resize(256);
}

coro_stack_pool::~coro_stack_pool()
{
	boost::lock_guard<boost::mutex> lg(_mutex);
	for (size_t mit = 0; mit < _stackPool.size(); mit++)
	{
		if (_stackPool[mit])
		{
			{
				boost::lock_guard<boost::mutex> lg1(_stackPool[mit]->_mutex);
				for (auto it = _stackPool[mit]->_pool.begin(); it != _stackPool[mit]->_pool.end(); it++)
				{
					_all.deallocate((*it)._stack);
					_stackCount--;
				}
			}
			delete _stackPool[mit];
		}
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
		stack_pool_pck** pool = NULL;
		{
			boost::lock_guard<boost::mutex> lg(_coroStackPool->_mutex);
			pool = &_coroStackPool->_stackPool[size/4096-1];
			if (NULL == *pool)
			{
				*pool = new stack_pool_pck;
			}
		}
		boost::lock_guard<boost::mutex> lg((*pool)->_mutex);
		if (!(*pool)->_pool.empty())
		{
			auto r = (*pool)->_pool.front();
			(*pool)->_pool.pop_front();
			return r;
		}
	}
	stack_pck r;
	r._size = size;
	_coroStackPool->_all.allocate(r._stack, size);
	_coroStackPool->_stackCount++;
	return r;
}

void coro_stack_pool::recovery( stack_pck& stack )
{
	stack_pool_pck** pool = NULL;
	{
		//boost::lock_guard<boost::mutex> lg(_coroStackPool->_mutex);
		pool = &_coroStackPool->_stackPool[stack._size/4096-1];
		assert(*pool);
	}
	boost::lock_guard<boost::mutex> lg((*pool)->_mutex);
	(*pool)->_pool.push_front(stack);
}

boost::shared_ptr<coro_stack_pool> coro_stack_pool::_coroStackPool;