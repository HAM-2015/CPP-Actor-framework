#ifndef __MSG_QUEUE_H
#define __MSG_QUEUE_H

#include "mem_pool.h"
#include "scattered.h"

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

template <typename T>
class msg_list
{
	struct node
	{

		BYTE _data[sizeof(T)];
		node* _previous;
		node* _next;
#ifdef _DEBUG
		BYTE _isErase[sizeof(std::shared_ptr<bool>)];

		std::shared_ptr<bool> cast_is_erase()
		{
			return *(std::shared_ptr<bool>*)_isErase;
		}

		void free_is_erase()
		{
			typedef std::shared_ptr<bool> bool_type;
			**(bool_type*)_isErase = true;
			((bool_type*)_isErase)->~bool_type();
		}
#endif
	};
public:
	class node_it
	{
		friend msg_list;
		node* _node;
		std::shared_ptr<bool> _isErase;
	};
public:
	msg_list(size_t poolSize)
		:_alloc(poolSize)
	{
		_size = 0;
		_head._next = &_head;
		_head._previous = &_head;
		DEBUG_OPERATION(_signAll = create_shared_pool<bool>(poolSize, [](void*){}));
	}

	~msg_list()
	{
		clear();
		DEBUG_OPERATION(delete _signAll);
	}

	node_it push_back(const T& p)
	{
		assert(_size < 256);
		_size++;
		node_it newNode;
		newNode._node = new_back();
		DEBUG_OPERATION(newNode._isErase = newNode._node->cast_is_erase());
		new(newNode._node->_data)T(p);
		return newNode;
	}

	node_it push_back(T&& p)
	{
		assert(_size < 256);
		_size++;
		node_it newNode;
		newNode._node = new_back();
		DEBUG_OPERATION(newNode._isErase = newNode._node->cast_is_erase());
		new(newNode._node->_data)T(std::move(p));
		return newNode;
	}

	node_it push_front(const T& p)
	{
		assert(_size < 256);
		_size++;
		node_it newNode;
		newNode._node = new_front();
		DEBUG_OPERATION(newNode._isErase = newNode._node->cast_is_erase());
		new(newNode._node->_data)T(p);
		return newNode;
	}

	node_it push_front(T&& p)
	{
		assert(_size < 256);
		_size++;
		node_it newNode;
		newNode._node = new_front();
		DEBUG_OPERATION(newNode._isErase = newNode._node->cast_is_erase());
		new(newNode._node->_data)T(std::move(p));
		return newNode;
	}

	T& front()
	{
		assert(_size);
		return *(T*)_head._next->_data;
	}

	T& back()
	{
		assert(_size);
		return *(T*)_head._previous->_data;
	}

	void pop_front()
	{
		assert(_size);
		_size--;
		node* frontNode = _head._next;
		_head._next = frontNode->_next;
		_head._next->_previous = frontNode->_previous;
		((T*)frontNode->_data)->~T();
		DEBUG_OPERATION(frontNode->free_is_erase());
		_alloc.deallocate(frontNode);
	}

	void pop_back()
	{
		assert(_size);
		_size--;
		node* backNode = _head._previous;
		_head._previous = backNode->_previous;
		_head._previous->_next = backNode->_next;
		((T*)backNode->_data)->~T();
		DEBUG_OPERATION(backNode->free_is_erase());
		_alloc.deallocate(backNode);
	}

	void erase(node_it& it)
	{
		assert(!(*it._isErase));
		assert(_size);
		_size--;
		node* eraseNode = it._node;
		eraseNode->_previous->_next = eraseNode->_next;
		eraseNode->_next->_previous = eraseNode->_previous;
		((T*)eraseNode->_data)->~T();
		DEBUG_OPERATION(eraseNode->free_is_erase());
		_alloc.deallocate(eraseNode);
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
		while (pIt != &_head)
		{
			_size--;
			((T*)pIt->_data)->~T();
			node* t = pIt;
			pIt = pIt->_next;
			DEBUG_OPERATION(t->free_is_erase());
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
		node* endNode = _head._previous;
		newNode->_next = endNode->_next;
		newNode->_previous = endNode;
		endNode->_next = newNode;
		newNode->_next->_previous = newNode;
		DEBUG_OPERATION(new (newNode->_isErase)std::shared_ptr<bool>(_signAll->new_()));
		DEBUG_OPERATION(*newNode->cast_is_erase() = false);
		return newNode;
	}

	node* new_front()
	{
		node* newNode = _alloc.allocate();
		node* beginNode = _head._next;
		newNode->_previous = beginNode->_previous;
		newNode->_next = beginNode;
		beginNode->_previous = newNode;
		newNode->_previous->_next = newNode;
		DEBUG_OPERATION(new (newNode->_isErase)std::shared_ptr<bool>(_signAll->new_()));
		DEBUG_OPERATION(*newNode->cast_is_erase() = false);
		return newNode;
	}
private:
	node _head;
	size_t _size;
	size_t _idCount;
	mem_alloc<node> _alloc;
	DEBUG_OPERATION(shared_obj_pool_base<bool>* _signAll);
};


#endif