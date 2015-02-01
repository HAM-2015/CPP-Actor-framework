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

//////////////////////////////////////////////////////////////////////////

coro_stack::coro_stack()
{

}

coro_stack::coro_stack( void** sp, size_t* size )
{
	_sp = sp;
	_size = size;
}

coro_stack::~coro_stack()
{

}

void coro_stack::allocate( boost::coroutines::stack_context & stackCon, size_t size )
{
	if (coro_stack_pool::isEnable())
	{
		_stack = coro_stack_pool::getStack(size);
	}
	else
	{
		boost::coroutines::stack_allocator all;
		all.allocate(_stack._stack, size);
	}
	stackCon = _stack._stack;
	*_sp = stackCon.sp;
	*_size = stackCon.size;
}

void coro_stack::deallocate( boost::coroutines::stack_context & stackCon )
{
	if (coro_stack_pool::isEnable())
	{
		stackCon = boost::coroutines::stack_context();
		coro_stack_pool::recovery(_stack);
	} 
	else
	{
		boost::coroutines::stack_allocator all;
		all.deallocate(stackCon);
	}
}

//////////////////////////////////////////////////////////////////////////
coro_buff_pool::coro_buff_pool()
{
	_buffCount = 0;
}

coro_buff_pool::~coro_buff_pool()
{
	for (auto mit = _buffPool.begin(); mit != _buffPool.end(); mit++)
	{
		auto lst = mit->second;
		for (auto it = lst->_pool.begin(); it != lst->_pool.end(); it++)
		{
			free(*it);
			_buffCount--;
		}
		delete lst;
	}
	assert(0 == _buffCount);
}

void coro_buff_pool::enable()
{
	if (!_coroBuffPool)
	{
		_coroBuffPool = boost::shared_ptr<coro_buff_pool>(new coro_buff_pool);
	}
}

void* coro_buff_pool::getBuff( size_t size )
{
	{
		buff_pool_pck** pool = NULL;
		{
			boost::lock_guard<boost::mutex> lg(_coroBuffPool->_mutex);
			pool = &_coroBuffPool->_buffPool[size];
			if (NULL == *pool)
			{
				*pool = new buff_pool_pck;
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
	_coroBuffPool->_buffCount++;
	void* ptr = malloc(size);
	if (!ptr)
	{
		throw msg_data::pool_memory_exception();
	}
	return ptr;
}

void coro_buff_pool::recovery(void* ptr, size_t size)
{
#ifdef _DEBUG
	memset(ptr, 0xCD, size);
#endif
	buff_pool_pck** pool = NULL;
	{
		boost::lock_guard<boost::mutex> lg(_coroBuffPool->_mutex);
		pool = &_coroBuffPool->_buffPool[size];
	}
	boost::lock_guard<boost::mutex> lg((*pool)->_mutex);
	(*pool)->_pool.push_front(ptr);
}

boost::shared_ptr<coro_buff_pool> coro_buff_pool::_coroBuffPool;
