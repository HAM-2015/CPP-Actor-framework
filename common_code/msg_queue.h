#ifndef __MSG_QUEUE_H
#define __MSG_QUEUE_H

#include "mem_pool.h"
#include "scattered.h"
#include <map>
#include <set>
#include <list>

template <typename T>
class msg_queue
{
	struct node 
	{
		char _data[sizeof(T)];
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

template <typename T>
class msg_list: public std::list<T, pool_alloc<T>>
{
public:
	msg_list(size_t poolSize)
		:std::list<T, pool_alloc<T>>(pool_alloc<T>(poolSize)) {}
};

template <typename Tkey, typename Tval, typename Tcmp = std::less<Tkey>>
class msg_map : public std::map<Tkey, Tval, Tcmp, pool_alloc<std::pair<const Tkey, Tval>> >
{
public:
	msg_map(size_t poolSize)
		:std::map<Tkey, Tval, Tcmp, pool_alloc<std::pair<const Tkey, Tval>> >(Tcmp(), pool_alloc<std::pair<const Tkey, Tval>>(poolSize)){}
};

template <typename Tkey, typename Tcmp = std::less<Tkey>>
class msg_set : public std::set<Tkey, std::less<Tkey>, pool_alloc<Tkey> >
{
public:
	msg_set(size_t poolSize)
		:std::set<Tkey, Tcmp, pool_alloc<Tkey> >(Tcmp(), pool_alloc<Tkey>(poolSize)){}
};

#endif