#ifndef __MSG_QUEUE_H
#define __MSG_QUEUE_H

template <typename T>
class msg_queue
{
	struct node 
	{
		BYTE _data[sizeof(T)];
		node* _previous;
		node* _next;
	};

	struct mem_alloc 
	{
		union node_space
		{
			void set_ef()
			{
#ifdef _DEBUG
				memset(this, 0xEF, sizeof(*this));
#endif
			}

			node _node;
			node_space* _link;
		};

		mem_alloc(size_t poolSize)
		{
			_poolSize = 0;
			_poolMaxSize = poolSize;
			_pool = NULL;
#ifdef _DEBUG
			_nodeNumber = 0;
#endif
		}

		~mem_alloc()
		{
			assert(0 == _nodeNumber);
			node_space* pIt = _pool;
			while (pIt)
			{
				_poolSize--;
				node_space* t = pIt;
				pIt = pIt->_link;
				free(t);
			}
			assert(0 == _poolSize);
		}

		node* allocate()
		{
#ifdef _DEBUG
			_nodeNumber++;
#endif
			if (_pool)
			{
				_poolSize--;
				node_space* fixedSpace = _pool;
				_pool = fixedSpace->_link;
				return (node*)fixedSpace;
			}
			return (node*)malloc(sizeof(node_space));
		}

		void deallocate(void* p)
		{
#ifdef _DEBUG
			_nodeNumber--;
#endif
			node_space* space = (node_space*)p;
			if (_poolSize < _poolMaxSize)
			{
				_poolSize++;
				space->set_ef();
				space->_link = _pool;
				_pool = space;
				return;
			}
			free(space);
		}

		node_space* _pool;
		size_t _poolSize;
		size_t _poolMaxSize;
#ifdef _DEBUG
		size_t _nodeNumber;
#endif
	};
public:
	msg_queue(size_t poolSize)
		:_alloc(poolSize)
	{
		_size = 0;
		_head._next = &_head;
		_head._previous = &_head;
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
		while (pIt != &_head)
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
		node* endNode = _head._previous;
		newNode->_next = endNode->_next;
		newNode->_previous = endNode;
		endNode->_next = newNode;
		newNode->_next->_previous = newNode;
		return newNode;
	}
private:
	node _head;
	size_t _size;
	mem_alloc _alloc;
};

#endif