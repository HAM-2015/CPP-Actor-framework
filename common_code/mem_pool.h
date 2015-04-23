#ifndef __MEM_POOL_H
#define __MEM_POOL_H

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

template <typename DATA>
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

		BYTE _node[sizeof(DATA)];
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

	DATA* allocate()
	{
#ifdef _DEBUG
		_nodeNumber++;
#endif
		if (_pool)
		{
			_poolSize--;
			node_space* fixedSpace = _pool;
			_pool = fixedSpace->_link;
			return (DATA*)fixedSpace;
		}
		return (DATA*)malloc(sizeof(node_space));
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
//////////////////////////////////////////////////////////////////////////

template <typename DATA>
struct mem_alloc_mt
{
	union node_space
	{
		void set_ef()
		{
#ifdef _DEBUG
			memset(this, 0xEF, sizeof(*this));
#endif
		}

		BYTE _node[sizeof(DATA)];
		node_space* _link;
	};

	mem_alloc_mt(size_t poolSize)
	{
		_poolSize = 0;
		_poolMaxSize = poolSize;
		_pool = NULL;
#ifdef _DEBUG
		_nodeNumber = 0;
#endif
	}

	~mem_alloc_mt()
	{
		boost::lock_guard<boost::mutex> lg(_mutex);
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

	DATA* allocate()
	{
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
#ifdef _DEBUG
			_nodeNumber++;
#endif
			if (_pool)
			{
				_poolSize--;
				node_space* fixedSpace = _pool;
				_pool = fixedSpace->_link;
				return (DATA*)fixedSpace;
			}
		}
		return (DATA*)malloc(sizeof(node_space));
	}

	void deallocate(void* p)
	{
		node_space* space = (node_space*)p;
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
#ifdef _DEBUG
			_nodeNumber--;
#endif
			if (_poolSize < _poolMaxSize)
			{
				_poolSize++;
				space->set_ef();
				space->_link = _pool;
				_pool = space;
				return;
			}
		}
		free(space);
	}

	node_space* _pool;
	size_t _poolSize;
	size_t _poolMaxSize;
	boost::mutex _mutex;
#ifdef _DEBUG
	size_t _nodeNumber;
#endif
};

//////////////////////////////////////////////////////////////////////////

template <typename T, typename CREATER, typename DESTORY>
class shared_obj_pool;

template <typename T, typename CREATER, typename DESTORY>
class obj_pool;

template <typename T>
class obj_pool_base
{
public:
	virtual ~obj_pool_base(){};
	virtual	T* new_() = 0;
	virtual void delete_(void* p) = 0;
protected:
	boost::mutex _mutex;
};

template <typename T, typename CREATER, typename DESTORY>
static obj_pool_base<T>* create_pool(size_t poolSize, const CREATER& creater, const DESTORY& destory)
{
	return new obj_pool<T, CREATER, DESTORY>(poolSize, creater, destory);
}

template <typename T, typename CREATER>
static obj_pool_base<T>* create_pool(size_t poolSize, const CREATER& creater)
{
	return create_pool<T>(poolSize, creater, [](T* p)
	{
		typedef T type;
		p->~type();
	});
}

template <typename T, typename CREATER, typename DESTORY>
class obj_pool: public obj_pool_base<T>
{
	struct node 
	{
		BYTE _data[sizeof(T)];
		node* _link;
	};

	friend static obj_pool_base<T>* create_pool<T, CREATER, DESTORY>(size_t, const CREATER&, const DESTORY&);
	friend shared_obj_pool<T, CREATER, DESTORY>;
private:
	obj_pool(size_t poolSize, const CREATER& creater, const DESTORY& destory)
		:_creater(creater), _destory(destory), _poolSize(poolSize), _size(0), _link(NULL)
	{
#ifdef _DEBUG
		_nodeNumber = 0;
#endif
	}
public:
	~obj_pool()
	{
		boost::lock_guard<boost::mutex> lg(_mutex);
		assert(0 == _nodeNumber);
		node* it = _link;
		while (it)
		{
			_size--;
			node* t = it;
			it = it->_link;
			_destory((T*)t->_data);
			free(t);
		}
		assert(0 == _size);
	}
public:
	T* new_()
	{
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
#ifdef _DEBUG
			_nodeNumber++;
#endif
			if (_link)
			{
				_size--;
				node* r = _link;
				_link = _link->_link;
				return (T*)r->_data;
			}
		}
		node* newNode = (node*)malloc(sizeof(node));
		assert((void*)newNode == (void*)newNode->_data);
		_creater(newNode);
		return (T*)newNode->_data;
	}

	void delete_(void* p)
	{
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
#ifdef _DEBUG
			_nodeNumber--;
#endif
			if (_size < _poolSize)
			{
				_size++;
				((node*)p)->_link = _link;
				_link = (node*)p;
				return;
			}
		}
		_destory((T*)((node*)p)->_data);
		free(p);
	}
private:
	CREATER _creater;
	DESTORY _destory;
	node* _link;
	size_t _poolSize;
	size_t _size;
#ifdef _DEBUG
	size_t _nodeNumber;
#endif
};
//////////////////////////////////////////////////////////////////////////

template <typename T>
class shared_obj_pool_base
{
public:
	virtual ~shared_obj_pool_base(){};
	virtual	std::shared_ptr<T> new_() = 0;
};

template <typename T, typename CREATER, typename DESTORY>
static shared_obj_pool_base<T>* create_shared_pool(size_t poolSize, const CREATER& creater, const DESTORY& destory)
{
	return new shared_obj_pool<T, CREATER, DESTORY>(poolSize, creater, destory);
}

template <typename T, typename CREATER>
static shared_obj_pool_base<T>* create_shared_pool(size_t poolSize, const CREATER& creater)
{
	return create_shared_pool<T>(poolSize, creater, [](T* p)
	{
		typedef T type;
		p->~type();
	});
}

template <typename T, typename CREATER, typename DESTORY>
class shared_obj_pool: public shared_obj_pool_base<T>
{
	template <typename RC>
	struct create_alloc 
	{
		template<class Other>
		struct rebind
		{
			typedef create_alloc<Other> other;
		};

		create_alloc(void*& refCountAlloc, size_t poolSize)
			:_refCountAlloc(refCountAlloc), _poolSize(poolSize) {}

		create_alloc(const create_alloc<RC>& s)
			:_refCountAlloc(s._refCountAlloc), _poolSize(s._poolSize) {}

		template<class Other>
		create_alloc(const create_alloc<Other>& s)
			: _refCountAlloc(s._refCountAlloc), _poolSize(s._poolSize) {}

		RC* allocate(size_t count)
		{
			assert(1 == count);
			_refCountAlloc = new mem_alloc_mt<RC>(_poolSize);
			return (RC*)malloc(sizeof(RC));
		}

		void deallocate(RC* ptr, size_t count)
		{
			assert(1 == count);
			free(ptr);
			delete (mem_alloc_mt<RC>*)_refCountAlloc;
			_refCountAlloc = NULL;
		}

		void destroy(RC* ptr)
		{
			ptr->~RC();
		}

		void*& _refCountAlloc;
		size_t _poolSize;
	};

	template <typename RC>
	struct ref_count_alloc 
	{
		template<class Other>
		struct rebind
		{
			typedef ref_count_alloc<Other> other;
		};

		ref_count_alloc(void* refCountAlloc)
			:_refCountAlloc(refCountAlloc) {}

		ref_count_alloc(const ref_count_alloc<RC>& s)
			:_refCountAlloc(s._refCountAlloc) {}

		template<class Other>
		ref_count_alloc(const ref_count_alloc<Other>& s)
			: _refCountAlloc(s._refCountAlloc) {}

		RC* allocate(size_t count)
		{
			assert(1 == count);
			return ((mem_alloc_mt<RC>*)_refCountAlloc)->allocate();
		}

		void deallocate(RC* ptr, size_t count)
		{
			assert(1 == count);
			((mem_alloc_mt<RC>*)_refCountAlloc)->deallocate(ptr);
		}

		void destroy(RC* ptr)
		{
			ptr->~RC();
		}

		void* _refCountAlloc;
		boost::mutex* _mutex;
	};

	friend static shared_obj_pool_base<T>* create_shared_pool<T, CREATER, DESTORY>(size_t, const CREATER&, const DESTORY&);
private:
	shared_obj_pool(size_t poolSize, const CREATER& creater, const DESTORY& destory)
		:_dataAlloc(poolSize, creater, destory)
	{
		_lockAlloc = std::shared_ptr<T>(NULL, [](T*){}, create_alloc<void>(_refCountAlloc, poolSize));
	}
public:
	~shared_obj_pool()
	{
		_lockAlloc.reset();
		assert(!_refCountAlloc);
	}
public:
	std::shared_ptr<T> new_()
	{
		return std::shared_ptr<T>(_dataAlloc.new_(), [this](T* p){_dataAlloc.delete_(p); }, ref_count_alloc<void>(_refCountAlloc));
	}
private:
	obj_pool<T, CREATER, DESTORY> _dataAlloc;
	std::shared_ptr<T> _lockAlloc;
	void* _refCountAlloc;
};

#endif