#ifndef __MSG_QUEUE_H
#define __MSG_QUEUE_H

#include "mem_pool.h"

template <typename T>
class msg_queue
{
	struct node 
	{
		BYTE _data[sizeof(T)];
		node* _next;
	};
public:
	msg_queue(size_t poolSize)
		:_alloc(poolSize)
	{
		_size = 0;
		_head._next = NULL;
		_tail = &_head;
	}

	~msg_queue()
	{
		clear();
	}

	void push_back(const T& p)
	{
		assert(_size < 256);
		_size++;
		new(new_back()->_data)T(p);
	}

	void push_back(T&& p)
	{
		assert(_size < 256);
		_size++;
		new(new_back()->_data)T(std::move(p));
	}

	T& front()
	{
		assert(_size && _head._next);
		return *(T*)_head._next->_data;
	}

	T& back()
	{
		assert(_size);
		return *(T*)_tail->_data;
	}

	void pop_front()
	{
		assert(_size);
		if (0 == --_size)
		{
			_tail = &_head;
		}
		node* frontNode = _head._next;
		_head._next = frontNode->_next;
		((T*)frontNode->_data)->~T();
		_alloc.deallocate(frontNode);
	}

	size_t size()
	{
		return _size;
	}

	bool empty()
	{
		return !_size;
	}

	void clear()
	{
		node* pIt = _head._next;
		while (pIt)
		{
			_size--;
			((T*)pIt->_data)->~T();
			node* t = pIt;
			pIt = pIt->_next;
			_alloc.deallocate(t);
		}
		assert(0 == _size);
	}

	void expand_fixed(size_t fixedSize)
	{
		if (fixedSize > _alloc._poolMaxSize)
		{
			_alloc._poolMaxSize = fixedSize;
		}
	}
private:
	node* new_back()
	{
		node* newNode = _alloc.allocate();
		_tail->_next = newNode;
		_tail = newNode;
		_tail->_next = NULL;
		return newNode;
	}
private:
	node _head;
	node* _tail;
	size_t _size;
	mem_alloc<node> _alloc;
};

#endif