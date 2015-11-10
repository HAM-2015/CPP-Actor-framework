#ifndef __MEM_POOL_H
#define __MEM_POOL_H

#include <mutex>
#include <memory>
#include <memory.h>
#include "try_move.h"

struct null_mutex
{
	void inline lock() const {};
	void inline unlock() const {};
};

struct mem_alloc_base
{
	mem_alloc_base(){}
	virtual ~mem_alloc_base(){}
	virtual void* allocate() = 0;
	virtual void deallocate(void* p) = 0;
	virtual bool shared() const = 0;
	virtual size_t alloc_size() const = 0;

	bool full() const
	{
		return _blockNumber >= _poolMaxSize;
	}

	size_t _nodeCount;
	size_t _poolMaxSize;
	size_t _blockNumber;
private:
	mem_alloc_base(const mem_alloc_base&){}
	void operator=(const mem_alloc_base&){}
};

//////////////////////////////////////////////////////////////////////////

template <typename DATA, typename MUTEX = std::mutex>
struct mem_alloc_mt : mem_alloc_base
{
	struct node_space;

	union BUFFER
	{
		unsigned char _space[sizeof(DATA)];
		node_space* _link;
	};

	struct node_space
	{
		void set_bf()
		{
#if (_DEBUG || DEBUG)
			memset(get_ptr(), 0xBF, sizeof(DATA));
#endif
		}

		void set_af()
		{
#if (_DEBUG || DEBUG)
			memset(get_ptr(), 0xAF, sizeof(DATA));
#endif
		}

		void* get_ptr()
		{
			return _buff._space;
		}

		void set_head()
		{
#if (_DEBUG || DEBUG)
			_size = sizeof(DATA);
#endif
		}

		void check_head()
		{
			assert(sizeof(DATA) <= _size);
		}

		static node_space* get_node(void* p)
		{
#if (_DEBUG || DEBUG)
			return (node_space*)((unsigned char*)p - sizeof(size_t));
#else
			return (node_space*)p;
#endif
		}

#if (_DEBUG || DEBUG)
		size_t _size;
#endif
		BUFFER _buff;
	};

	mem_alloc_mt(size_t poolSize)
	{
		_nodeCount = 0;
		_poolMaxSize = poolSize;
		_pool = NULL;
		_blockNumber = 0;
	}

	virtual ~mem_alloc_mt()
	{
		std::lock_guard<MUTEX> lg(_mutex);
		node_space* pIt = _pool;
		while (pIt)
		{
			assert(_nodeCount > 0);
			_nodeCount--;
			node_space* t = pIt;
			pIt = pIt->_buff._link;
			free(t);
		}
		assert(0 == _nodeCount);
	}

	void* allocate()
	{
		{
			_mutex.lock();
			_blockNumber++;
			if (_pool)
			{
				_nodeCount--;
				node_space* fixedSpace = _pool;
				_pool = fixedSpace->_buff._link;
				_mutex.unlock();
				fixedSpace->set_af();
				return fixedSpace->get_ptr();
			}
			_mutex.unlock();
		}
		node_space* p = (node_space*)malloc(sizeof(node_space));
		p->set_head();
		return p->get_ptr();
	}

	void deallocate(void* p)
	{
		node_space* space = node_space::get_node(p);
		space->check_head();
		space->set_bf();
		{
			std::lock_guard<MUTEX> lg(_mutex);
			_blockNumber--;
			if (_nodeCount < _poolMaxSize)
			{
				_nodeCount++;
				space->_buff._link = _pool;
				_pool = space;
				return;
			}
		}
		free(space);
	}

	size_t alloc_size() const
	{
		return sizeof(DATA);
	}

	bool shared() const
	{
		return true;
	}

	node_space* _pool;
	MUTEX _mutex;
};

template <typename MUTEX = std::mutex>
struct dymem_alloc_mt : mem_alloc_base
{
	struct dy_node 
	{
		static void* alloc(size_t spaceSize)
		{
#if (_DEBUG || DEBUG)
			return malloc(sizeof(size_t) + spaceSize);
#else
			return malloc(spaceSize);
#endif
		}

		static void set_next(size_t spaceSize, void* d, void* s)
		{
#if (_DEBUG || DEBUG)
			*(void**)((char*)d + sizeof(size_t) + spaceSize - sizeof(void*)) = s;
#else
			*(void**)((char*)d + spaceSize - sizeof(void*)) = s;
#endif
		}

		static void* get_next(size_t spaceSize, void* p)
		{
#if (_DEBUG || DEBUG)
			return *(void**)((char*)p + sizeof(size_t) + spaceSize - sizeof(void*));
#else
			return *(void**)((char*)p + spaceSize - sizeof(void*));
#endif
		}

		static void set_bf(size_t spaceSize, void* p)
		{
#if (_DEBUG || DEBUG)
			memset(get_ptr(spaceSize, p), 0xBF, spaceSize);
#endif
		}

		static void set_af(size_t spaceSize, void* p)
		{
#if (_DEBUG || DEBUG)
			memset(get_ptr(spaceSize, p), 0xAF, spaceSize);
#endif
		}

		static void* get_ptr(size_t spaceSize, void* p)
		{
#if (_DEBUG || DEBUG)
			return (char*)p + sizeof(size_t);
#else
			return p;
#endif
		}

		static void set_head(size_t spaceSize, void* p)
		{
#if (_DEBUG || DEBUG)
			*(size_t*)p = spaceSize;
#endif
		}

		static void check_head(size_t spaceSize, void* p)
		{
#if (_DEBUG || DEBUG)
			assert(spaceSize == *(size_t*)p);
#endif
		}

		static void* get_node(size_t spaceSize, void* p)
		{
#if (_DEBUG || DEBUG)
			return (char*)p - sizeof(size_t);
#else
			return p;
#endif
		}
	};

	dymem_alloc_mt(size_t size, size_t poolSize)
		:_spaceSize(size > sizeof(void*) ? size : sizeof(void*))
	{
		_nodeCount = 0;
		_poolMaxSize = poolSize;
		_pool = NULL;
		_blockNumber = 0;
	}

	virtual ~dymem_alloc_mt()
	{
		std::lock_guard<MUTEX> lg(_mutex);
		void* pIt = _pool;
		while (pIt)
		{
			assert(_nodeCount > 0);
			_nodeCount--;
			void* t = pIt;
			pIt = dy_node::get_next(_spaceSize, pIt);
			free(t);
		}
		assert(0 == _nodeCount);
	}

	void* allocate()
	{
		{
			_mutex.lock();
			_blockNumber++;
			if (_pool)
			{
				_nodeCount--;
				void* fixedSpace = _pool;
				_pool = dy_node::get_next(_spaceSize, fixedSpace);
				_mutex.unlock();
				dy_node::set_af(_spaceSize, fixedSpace);
				return dy_node::get_ptr(_spaceSize, fixedSpace);
			}
			_mutex.unlock();
		}
		void* p = dy_node::alloc(_spaceSize);
		dy_node::set_head(_spaceSize, p);
		return dy_node::get_ptr(_spaceSize, p);
	}

	void deallocate(void* p)
	{
		void* space = dy_node::get_node(_spaceSize, p);
		dy_node::check_head(_spaceSize, space);
		dy_node::set_bf(_spaceSize, space);
		{
			std::lock_guard<MUTEX> lg(_mutex);
			_blockNumber--;
			if (_nodeCount < _poolMaxSize)
			{
				_nodeCount++;
				dy_node::set_next(_spaceSize, space, _pool);
				_pool = space;
				return;
			}
		}
		free(space);
	}

	size_t alloc_size() const
	{
		return _spaceSize;
	}

	virtual bool shared() const
	{
		return true;
	}

	const size_t _spaceSize;
	void* _pool;
	MUTEX _mutex;
};

//////////////////////////////////////////////////////////////////////////
template <typename DATA>
struct mem_alloc : public mem_alloc_mt<DATA, null_mutex>
{
	mem_alloc(size_t poolSize)
	:mem_alloc_mt<DATA, null_mutex>(poolSize) {}
	
	bool shared() const
	{
		return false;
	}
};

struct dymem_alloc : public dymem_alloc_mt<null_mutex>
{
	dymem_alloc(size_t size, size_t poolSize)
	:dymem_alloc_mt<null_mutex>(size, poolSize) {}

	bool shared() const
	{
		return false;
	}
};

//////////////////////////////////////////////////////////////////////////

template <typename MUTEX = std::mutex>
class reusable_mem_mt
{
	struct node
	{
		unsigned _size;
		node* _next;
	};
public:
	reusable_mem_mt()
	{
		_top = NULL;
#if (_DEBUG || DEBUG)
		_nodeCount = 0;
#endif
	}

	virtual ~reusable_mem_mt()
	{
		std::lock_guard<MUTEX> lg(_mutex);
		while (_top)
		{
			assert(_nodeCount-- > 0);
			void* t = _top;
			_top = _top->_next;
			free(t);
		}
		assert(0 == _nodeCount);
	}

	void* allocate(size_t size)
	{
		void* freeMem = NULL;
		{
			std::lock_guard<MUTEX> lg(_mutex);
			if (_top)
			{
				node* res = _top;
				_top = _top->_next;
				if (res->_size >= size)
				{
					return res;
				}
				assert(_nodeCount-- > 0);
				freeMem = res;
			}
#if (_DEBUG || DEBUG)
			_nodeCount++;
#endif
		}
		if (freeMem)
		{
			free(freeMem);
		}
		return malloc(size < sizeof(node) ? sizeof(node) : size);
	}

	void deallocate(void* p, size_t size)
	{
		std::lock_guard<MUTEX> lg(_mutex);
		node* dp = (node*)p;
		dp->_size = size < sizeof(node) ? sizeof(node) : (unsigned)size;
		dp->_next = _top;
		_top = dp;
	}
private:
	node* _top;
	MUTEX _mutex;
#if (_DEBUG || DEBUG)
	size_t _nodeCount;
#endif
};
//////////////////////////////////////////////////////////////////////////

class reusable_mem : public reusable_mem_mt<null_mutex> {};

//////////////////////////////////////////////////////////////////////////
template<typename _Ty, typename _Mtx = std::mutex>
class pool_alloc_mt
{
public:
	typedef _Ty node_type;
	typedef mem_alloc<_Ty> mem_alloc_type;
	typedef mem_alloc_mt<_Ty, _Mtx> mem_alloc_mt_type;
	typedef typename std::allocator<_Ty>::pointer pointer;
	typedef typename std::allocator<_Ty>::difference_type difference_type;
	typedef typename std::allocator<_Ty>::reference reference;
	typedef typename std::allocator<_Ty>::const_pointer const_pointer;
	typedef typename std::allocator<_Ty>::const_reference const_reference;
	typedef typename std::allocator<_Ty>::size_type size_type;
	typedef typename std::allocator<_Ty>::value_type value_type;

	template<class _Other>
	struct rebind
	{
		typedef pool_alloc_mt<_Other, _Mtx> other;
	};

	pool_alloc_mt(size_t poolSize, bool shared = true)
	{
		if (shared)
		{
			_memAlloc = std::shared_ptr<mem_alloc_base>(new mem_alloc_mt_type(poolSize));
		}
		else
		{
			_memAlloc = std::shared_ptr<mem_alloc_base>(new mem_alloc_type(poolSize));
		}
	}

	~pool_alloc_mt()
	{
	}

	pool_alloc_mt(const pool_alloc_mt& s)
	{
		if (s.is_shared())
		{
			_memAlloc = s._memAlloc;
		}
		else
		{
			_memAlloc = std::shared_ptr<mem_alloc_base>(new mem_alloc_type(s._memAlloc->_poolMaxSize));
		}
	}

	template<class _Other>
	pool_alloc_mt(const pool_alloc_mt<_Other, _Mtx>& s)
	{
		if (s.is_shared())
		{
// 			if (sizeof(_Ty) > s._memAlloc->alloc_size())
// 			{
// 				_memAlloc = std::shared_ptr<mem_alloc_base>(new mem_alloc_mt_type(s._memAlloc->_poolMaxSize));
// 			} 
// 			else
			{
				assert(sizeof(_Ty) <= s._memAlloc->alloc_size());
				_memAlloc = s._memAlloc;
			}
		}
		else
		{
			_memAlloc = std::shared_ptr<mem_alloc_base>(new mem_alloc_type(s._memAlloc->_poolMaxSize));
		}
	}

	pool_alloc_mt& operator=(const pool_alloc_mt& s)
	{
		assert(false);
		return *this;
	}

	template<class _Other>
	pool_alloc_mt& operator=(const pool_alloc_mt<_Other, _Mtx>& s)
	{
		assert(false);
		return *this;
	}

	bool operator==(const pool_alloc_mt& s)
	{
		return is_shared() == s.is_shared();
	}

	void deallocate(pointer _Ptr, size_type)
	{
		_memAlloc->deallocate(_Ptr);
	}

	pointer allocate(size_type _Count)
	{
		assert(1 == _Count);
		return (pointer)_memAlloc->allocate();
	}

	void construct(_Ty *_Ptr, const _Ty& _Val)
	{
		new ((void *)_Ptr) _Ty(_Val);
	}

	void construct(_Ty *_Ptr, _Ty&& _Val)
	{
		new ((void *)_Ptr) _Ty(std::move(_Val));
	}

	template<class _Uty>
	void destroy(_Uty *_Ptr)
	{
		_Ptr->~_Uty();
	}

	size_t max_size() const
	{
		return ((size_t)(-1) / sizeof (_Ty));
	}

	bool is_shared() const
	{
		return _memAlloc->shared();
	}

	void enable_shared(size_t poolSize = -1)
	{
		if (!_memAlloc->shared())
		{
			_memAlloc = std::shared_ptr<mem_alloc_base>(new mem_alloc_mt_type(-1 == poolSize ? _memAlloc->_poolMaxSize : poolSize));
		}
	}

	std::shared_ptr<mem_alloc_base> _memAlloc;
};
//////////////////////////////////////////////////////////////////////////

template<typename _Ty>
class pool_alloc
{
public:
	typedef _Ty node_type;
	typedef mem_alloc<_Ty> mem_alloc_type;
	typedef typename std::allocator<_Ty>::pointer pointer;
	typedef typename std::allocator<_Ty>::difference_type difference_type;
	typedef typename std::allocator<_Ty>::reference reference;
	typedef typename std::allocator<_Ty>::const_pointer const_pointer;
	typedef typename std::allocator<_Ty>::const_reference const_reference;
	typedef typename std::allocator<_Ty>::size_type size_type;
	typedef typename std::allocator<_Ty>::value_type value_type;

	template<class _Other>
	struct rebind
	{
		typedef pool_alloc<_Other> other;
	};

	pool_alloc(size_t poolSize)
		:_memAlloc(poolSize)
	{
	}

	~pool_alloc()
	{
	}

	pool_alloc(const pool_alloc& s)
		:_memAlloc(s._memAlloc._poolMaxSize)
	{
	}

	template<class _Other>
	pool_alloc(const pool_alloc<_Other>& s)
		: _memAlloc(s._memAlloc._poolMaxSize)
	{
	}

	pool_alloc& operator=(const pool_alloc& s)
	{
		assert(false);
		return *this;
	}

	template<class _Other>
	pool_alloc& operator=(const pool_alloc<_Other>& s)
	{
		assert(false);
		return *this;
	}

	bool operator==(const pool_alloc& s)
	{
		return true;
	}

	void deallocate(pointer _Ptr, size_type)
	{
		_memAlloc.deallocate(_Ptr);
	}

	pointer allocate(size_type _Count)
	{
		assert(1 == _Count);
		return (pointer)_memAlloc.allocate();
	}

	void construct(_Ty *_Ptr, const _Ty& _Val)
	{
		new ((void *)_Ptr) _Ty(_Val);
	}

	void construct(_Ty *_Ptr, _Ty&& _Val)
	{
		new ((void *)_Ptr) _Ty(std::move(_Val));
	}

	template<class _Uty>
	void destroy(_Uty *_Ptr)
	{
		_Ptr->~_Uty();
	}

	size_t max_size() const
	{
		return ((size_t)(-1) / sizeof (_Ty));
	}

	bool is_shared() const
	{
		return _memAlloc.shared();
	}

	mem_alloc_type _memAlloc;
};
//////////////////////////////////////////////////////////////////////////

template <typename T, typename CREATER, typename DESTORY, typename MUTEX = std::mutex>
class SharedObjPool_;

template <typename T, typename CREATER, typename DESTORY, typename MUTEX = std::mutex>
class ObjPool_;

template <typename T>
class obj_pool
{
public:
	virtual ~obj_pool(){};
	virtual	T* pick() = 0;
	virtual void recycle(void* p) = 0;
};

template <typename T, typename CREATER, typename DESTORY>
static obj_pool<T>* create_pool(size_t poolSize, CREATER&& creater, DESTORY&& destory)
{
	return new ObjPool_<T, RM_CREF(CREATER), RM_CREF(DESTORY)>(poolSize, TRY_MOVE(creater), TRY_MOVE(destory));
}

template <typename T, typename CREATER>
static obj_pool<T>* create_pool(size_t poolSize, CREATER&& creater)
{
	return create_pool<T>(poolSize, TRY_MOVE(creater), [](T* p)
	{
		typedef T type;
		p->~type();
	});
}

template <typename T, typename MUTEX, typename CREATER, typename DESTORY>
static obj_pool<T>* create_pool_mt(size_t poolSize, CREATER&& creater, DESTORY&& destory)
{
	return new ObjPool_<T, RM_CREF(CREATER), RM_CREF(DESTORY), MUTEX>(poolSize, TRY_MOVE(creater), TRY_MOVE(destory));
}

template <typename T, typename MUTEX, typename CREATER>
static obj_pool<T>* create_pool_mt(size_t poolSize, CREATER&& creater)
{
	return create_pool_mt<T, MUTEX>(poolSize, TRY_MOVE(creater), [](T* p)
	{
		typedef T type;
		p->~type();
	});
}

template <typename T, typename CREATER, typename DESTORY, typename MUTEX>
class ObjPool_ : public obj_pool<T>
{
	struct node
	{
		unsigned char _data[sizeof(T)];
		node* _link;
	};
public:
	template <typename Creater, typename Destory>
	ObjPool_(size_t poolSize, Creater&& creater, Destory&& destory)
		:_creater(TRY_MOVE(creater)), _destory(TRY_MOVE(destory)), _poolMaxSize(poolSize), _nodeCount(0), _link(NULL)
	{
#if (_DEBUG || DEBUG)
		_blockNumber = 0;
#endif
	}
public:
	~ObjPool_()
	{
		std::lock_guard<MUTEX> lg(_mutex);
		assert(0 == _blockNumber);
		node* it = _link;
		while (it)
		{
			assert(_nodeCount > 0);
			_nodeCount--;
			node* t = it;
			it = it->_link;
			_destory((T*)t->_data);
			free(t);
		}
		assert(0 == _nodeCount);
	}
public:
	T* pick()
	{
		{
			std::lock_guard<MUTEX> lg(_mutex);
#if (_DEBUG || DEBUG)
			_blockNumber++;
#endif
			if (_link)
			{
				_nodeCount--;
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

	void recycle(void* p)
	{
		{
			std::lock_guard<MUTEX> lg(_mutex);
#if (_DEBUG || DEBUG)
			_blockNumber--;
#endif
			if (_nodeCount < _poolMaxSize)
			{
				_nodeCount++;
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
	MUTEX _mutex;
	node* _link;
	size_t _poolMaxSize;
	size_t _nodeCount;
#if (_DEBUG || DEBUG)
	size_t _blockNumber;
#endif
};
//////////////////////////////////////////////////////////////////////////

template <typename T>
class shared_obj_pool
{
public:
	virtual ~shared_obj_pool(){};
	virtual	std::shared_ptr<T> pick() = 0;
};

template <typename T, typename CREATER, typename DESTORY>
static shared_obj_pool<T>* create_shared_pool(size_t poolSize, CREATER&& creater, DESTORY&& destory)
{
	return new SharedObjPool_<T, RM_CREF(CREATER), RM_CREF(DESTORY)>(poolSize, TRY_MOVE(creater), TRY_MOVE(destory));
}

template <typename T, typename CREATER>
static shared_obj_pool<T>* create_shared_pool(size_t poolSize, CREATER&& creater)
{
	return create_shared_pool<T>(poolSize, TRY_MOVE(creater), [](T* p)
	{
		typedef T type;
		p->~type();
	});
}

template <typename T, typename MUTEX, typename CREATER, typename DESTORY>
static shared_obj_pool<T>* create_shared_pool_mt(size_t poolSize, CREATER&& creater, DESTORY&& destory)
{
	return new SharedObjPool_<T, RM_CREF(CREATER), RM_CREF(DESTORY), MUTEX>(poolSize, TRY_MOVE(creater), TRY_MOVE(destory));
}

template <typename T, typename MUTEX, typename CREATER>
static shared_obj_pool<T>* create_shared_pool_mt(size_t poolSize, CREATER&& creater)
{
	return create_shared_pool_mt<T, MUTEX>(poolSize, TRY_MOVE(creater), [](T* p)
	{
		typedef T type;
		p->~type();
	});
}

struct RefAlloc_ 
{
	mem_alloc_base*& _refCountAlloc;
	size_t _nodeCount;
};

template<typename _Tp, typename MUTEX>
class CreateRefAlloc_;

template<typename MUTEX>
class CreateRefAlloc_<void, MUTEX>
{
public:
	typedef size_t      size_type;
	typedef void*       pointer;
	typedef const void* const_pointer;
	typedef void        value_type;

	template<typename _Tp1>
	struct rebind
	{
		typedef CreateRefAlloc_<_Tp1, MUTEX> other;
	};

	CreateRefAlloc_(RefAlloc_* refAll)
 		:_refAll(refAll) {}

	RefAlloc_* _refAll;
};

template<typename _Tp, typename MUTEX>
class CreateRefAlloc_
{
public:
	typedef size_t     size_type;
	typedef _Tp*       pointer;
	typedef const _Tp* const_pointer;
	typedef _Tp&       reference;
	typedef const _Tp& const_reference;
	typedef _Tp        value_type;

	template<typename _Tp1>
	struct rebind
	{
		typedef CreateRefAlloc_<_Tp1, MUTEX> other;
	};

	CreateRefAlloc_(RefAlloc_* refAll)
		:_refAll(refAll) {}

	CreateRefAlloc_(const CreateRefAlloc_& s)
		:_refAll(s._refAll) {}

	template<typename _Tp1>
	CreateRefAlloc_(const CreateRefAlloc_<_Tp1, MUTEX>& s)
		: _refAll(s._refAll) {}

	~CreateRefAlloc_()
	{}

	pointer allocate(size_type n, const void* = 0)
	{
		assert(1 == n);
		_refAll->_refCountAlloc = new mem_alloc_mt<_Tp, MUTEX>(_refAll->_nodeCount);
		throw bool();
		return NULL;
	}

	void deallocate(pointer p, size_type n)
	{
		assert(false);
	}

	size_type max_size() const
	{
		return size_t(-1) / sizeof(_Tp);
	}

	template <typename... Args>
	void construct(void* p, Args&&... args)
	{
		new(p)_Tp(TRY_MOVE(args)...);
	}

	void destroy(_Tp* p)
	{
		p->~_Tp();
	}

	RefAlloc_* _refAll;
};

template<typename _Tp>
class ref_count_alloc;

template<>
class ref_count_alloc<void>
{
public:
	typedef size_t      size_type;
	typedef void*       pointer;
	typedef const void* const_pointer;
	typedef void        value_type;

	template<typename _Tp1>
	struct rebind
	{
		typedef ref_count_alloc<_Tp1> other;
	};

	ref_count_alloc(mem_alloc_base* refCountAlloc)
		:_refCountAlloc(refCountAlloc)
	{
		static_assert(sizeof(ref_count_alloc<void>) == sizeof(CreateRefAlloc_<void, null_mutex>), "");
	}

	mem_alloc_base* _refCountAlloc;
};

template<typename _Tp>
class ref_count_alloc
{
public:
	typedef size_t     size_type;
	typedef _Tp*       pointer;
	typedef const _Tp* const_pointer;
	typedef _Tp&       reference;
	typedef const _Tp& const_reference;
	typedef _Tp        value_type;

	template<typename _Tp1>
	struct rebind
	{
		typedef ref_count_alloc<_Tp1> other;
	};

	ref_count_alloc(mem_alloc_base* refCountAlloc)
		:_refCountAlloc(refCountAlloc) {}

	ref_count_alloc(const ref_count_alloc& s)
		:_refCountAlloc(s._refCountAlloc) {}

	template<typename _Tp1>
	ref_count_alloc(const ref_count_alloc<_Tp1>& s)
		: _refCountAlloc(s._refCountAlloc)
	{
		assert(_refCountAlloc->alloc_size() >= sizeof(_Tp));
	}

	~ref_count_alloc()
	{
		static_assert(sizeof(ref_count_alloc<_Tp>) == sizeof(CreateRefAlloc_<_Tp, null_mutex>), "");
	}

	pointer allocate(size_type n, const void* = 0)
	{
		assert(1 == n);
		return (pointer)_refCountAlloc->allocate();
	}

	void deallocate(pointer p, size_type n)
	{
		assert(1 == n);
		_refCountAlloc->deallocate(p);
	}

	size_type max_size() const
	{
		return size_t(-1) / sizeof(_Tp);
	}

	template <typename... Args>
	void construct(void* p, Args&&... args)
	{
		new(p)_Tp(TRY_MOVE(args)...);
	}

	void destroy(_Tp* p)
	{
		p->~_Tp();
	}

	mem_alloc_base* _refCountAlloc;
};

template <typename T, typename MUTEX, typename DESTORY>
mem_alloc_base* make_ref_count_alloc(size_t poolSize, DESTORY&& destory)
{
	mem_alloc_base* refCountAlloc = NULL;
	try
	{
		RefAlloc_ refAll = { refCountAlloc, poolSize };
		std::shared_ptr<T>(NULL, TRY_MOVE(destory), CreateRefAlloc_<void, MUTEX>(&refAll));
	}
	catch (...) {}
	return refCountAlloc;
}

template <typename T, typename CREATER, typename DESTORY, typename MUTEX>
class SharedObjPool_ : public shared_obj_pool<T>
{
public:
	template <typename Creater, typename Destory>
	SharedObjPool_(size_t poolSize, Creater&& creater, Destory&& destory)
		:_dataAlloc(poolSize, TRY_MOVE(creater), TRY_MOVE(destory)), _refCountAlloc(NULL)
	{
		_refCountAlloc = make_ref_count_alloc<T, MUTEX>(poolSize, [this](T*){});
	}
public:
	~SharedObjPool_()
	{
		delete _refCountAlloc;
	}
public:
	std::shared_ptr<T> pick()
	{
		return std::shared_ptr<T>(_dataAlloc.pick(), [this](T* p){_dataAlloc.recycle(p); }, ref_count_alloc<void>(_refCountAlloc));
	}
private:
	ObjPool_<T, CREATER, DESTORY, MUTEX> _dataAlloc;
	mem_alloc_base* _refCountAlloc;
};

#endif