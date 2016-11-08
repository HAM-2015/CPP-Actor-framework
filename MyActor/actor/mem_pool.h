#ifndef __MEM_POOL_H
#define __MEM_POOL_H

#include <mutex>
#include <memory>
#include <memory.h>
#include "try_move.h"
#include "scattered.h"

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

	size_t pool_size() const
	{
		return _poolMaxSize;
	}

	bool overflow() const
	{
		return _freeNumber >= _poolMaxSize;
	}

	size_t _nodeCount;
	size_t _poolMaxSize;
	size_t _freeNumber;
	NONE_COPY(mem_alloc_base);
};

//////////////////////////////////////////////////////////////////////////

template <typename DATA = void, typename MUTEX = std::mutex>
struct mem_alloc_mt;

template <typename MUTEX>
struct mem_alloc_mt<void, MUTEX>
{
	typedef MUTEX mutex_type;

	template <typename _Other>
	struct rebind
	{
		typedef mem_alloc_mt<_Other, MUTEX> other;
	};

	template <typename _Other>
	struct rebind_no_mt
	{
		typedef mem_alloc_mt<_Other, null_mutex> other;
	};
};

template <typename DATA, typename MUTEX>
struct mem_alloc_mt : protected MUTEX, public mem_alloc_base
{
	struct node_space;

	typedef MUTEX mutex_type;

	template <typename _Other>
	struct rebind 
	{
		typedef mem_alloc_mt<_Other, MUTEX> other;
	};

	template <typename _Other>
	struct rebind_no_mt
	{
		typedef mem_alloc_mt<_Other, null_mutex> other;
	};

#if (_DEBUG || DEBUG)
	struct hold_space
	{
		void operator =(node_space* link)
		{
			_link = link;
		}

		operator node_space*()
		{
			return _link;
		}

		__space_align char _[MEM_ALIGN(sizeof(DATA), sizeof(void*))];
		node_space* _link;
	};
#endif

	union BUFFER
	{
#if (_DEBUG || DEBUG)
		__space_align char _space[MEM_ALIGN(sizeof(DATA), sizeof(void*)) + sizeof(void*)];
		hold_space _link;
#else
		__space_align char _space[MEM_ALIGN(sizeof(DATA), sizeof(void*))];
		node_space* _link;
#endif
	};

	struct node_space
	{
		void set_bf()
		{
#if (_DEBUG || DEBUG)
			memset(get_ptr(), 0xBF, sizeof(_buff._space));
#endif
		}

		void set_af()
		{
#if (_DEBUG || DEBUG)
			memset(get_ptr(), 0xAF, sizeof(_buff._space));
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
		_freeNumber = 0;
		_pool = NULL;
	}

	~mem_alloc_mt()
	{
		std::lock_guard<MUTEX> lg(*this);
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
			MUTEX::lock();
			_freeNumber++;
			if (_pool)
			{
				_nodeCount--;
				node_space* fixedSpace = _pool;
				_pool = fixedSpace->_buff._link;
				MUTEX::unlock();
				fixedSpace->set_af();
				return fixedSpace->get_ptr();
			}
			MUTEX::unlock();
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
			std::lock_guard<MUTEX> lg(*this);
			_freeNumber--;
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
};

template <typename DATA = void, typename MUTEX = std::mutex>
struct mem_alloc_mt2;

template <typename MUTEX>
struct mem_alloc_mt2<void, MUTEX>
{
	typedef MUTEX mutex_type;

	template <typename _Other>
	struct rebind
	{
		typedef mem_alloc_mt2<_Other, MUTEX> other;
	};

	template <typename _Other>
	struct rebind_no_mt
	{
		typedef mem_alloc_mt2<_Other, null_mutex> other;
	};
};

template <typename DATA, typename MUTEX>
struct mem_alloc_mt2 : protected MUTEX, public mem_alloc_base
{
	typedef typename mem_alloc_mt<DATA>::node_space node_space;
	typedef MUTEX mutex_type;

	template <typename _Other>
	struct rebind
	{
		typedef mem_alloc_mt2<_Other, MUTEX> other;
	};

	template <typename _Other>
	struct rebind_no_mt
	{
		typedef mem_alloc_mt2<_Other, null_mutex> other;
	};

	mem_alloc_mt2(size_t poolSize)
	{
		_nodeCount = poolSize;
		_poolMaxSize = poolSize;
		_freeNumber = 0;
		_pool = NULL;
		static_assert(sizeof(node_space) % sizeof(void*) == 0, "");
		_pblock = (node_space*)malloc(sizeof(node_space) * poolSize);
		for (size_t i = 0; i < poolSize; i++)
		{
			node_space* t = _pool;
			_pool = _pblock + i;
			_pool->set_head();
			_pool->_buff._link = t;
		}
	}

	~mem_alloc_mt2()
	{
		std::lock_guard<MUTEX> lg(*this);
		node_space* pIt = _pool;
		while (pIt)
		{
			assert(_nodeCount > 0);
			_nodeCount--;
			node_space* t = pIt;
			pIt = pIt->_buff._link;
			if (t < _pblock || t >= _pblock + _poolMaxSize)
			{
				free(t);
			}
		}
		free(_pblock);
		assert(0 == _nodeCount);
	}

	void* allocate()
	{
		{
			MUTEX::lock();
			_freeNumber++;
			if (_pool)
			{
				_nodeCount--;
				node_space* fixedSpace = _pool;
				_pool = fixedSpace->_buff._link;
				MUTEX::unlock();
				fixedSpace->set_af();
				return fixedSpace->get_ptr();
			}
			MUTEX::unlock();
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
			std::lock_guard<MUTEX> lg(*this);
			_freeNumber--;
			if (_nodeCount < _poolMaxSize || (space >= _pblock && space < _pblock + _poolMaxSize))
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

	node_space* _pblock;
	node_space* _pool;
};

template <typename MUTEX = std::mutex>
struct dymem_alloc_mt : protected MUTEX, public mem_alloc_base
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
		_freeNumber = 0;
	}

	~dymem_alloc_mt()
	{
		std::lock_guard<MUTEX> lg(*this);
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
			MUTEX::lock();
			_freeNumber++;
			if (_pool)
			{
				_nodeCount--;
				void* fixedSpace = _pool;
				_pool = dy_node::get_next(_spaceSize, fixedSpace);
				MUTEX::unlock();
				dy_node::set_af(_spaceSize, fixedSpace);
				return dy_node::get_ptr(_spaceSize, fixedSpace);
			}
			MUTEX::unlock();
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
			std::lock_guard<MUTEX> lg(*this);
			_freeNumber--;
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
};

//////////////////////////////////////////////////////////////////////////
template <typename DATA = void>
struct mem_alloc;

template <>
struct mem_alloc<void>
{
	template <typename _Other>
	struct rebind
	{
		typedef mem_alloc<_Other> other;
	};
};

template <typename DATA>
struct mem_alloc : public mem_alloc_mt<DATA, null_mutex>
{
	template <typename _Other>
	struct rebind
	{
		typedef mem_alloc<_Other> other;
	};

	mem_alloc(size_t poolSize)
	:mem_alloc_mt<DATA, null_mutex>(poolSize) {}
	
	bool shared() const
	{
		return false;
	}
};

template <typename DATA = void>
struct mem_alloc2;

template <>
struct mem_alloc2<void>
{
	template <typename _Other>
	struct rebind
	{
		typedef mem_alloc2<_Other> other;
	};
};

template <typename DATA>
struct mem_alloc2 : public mem_alloc_mt2<DATA, null_mutex>
{
	template <typename _Other>
	struct rebind
	{
		typedef mem_alloc2<_Other> other;
	};

	mem_alloc2(size_t poolSize)
	:mem_alloc_mt2<DATA, null_mutex>(poolSize) {}

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
class reusable_mem_mt : protected MUTEX
{
#pragma pack(push, 1)
	struct node
	{
		size_t _size;
		union
		{
			node* _next;
			void* _addr[1];
		};
	};
#pragma pack(pop)
public:
	reusable_mem_mt()
	{
		_top = NULL;
#if (_DEBUG || DEBUG)
		_nodeCount = 0;
#endif
	}

	~reusable_mem_mt()
	{
		std::lock_guard<MUTEX> lg(*this);
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
			std::lock_guard<MUTEX> lg(*this);
			if (_top)
			{
				node* res = _top;
				_top = _top->_next;
				if (res->_size >= size)
				{
					return res->_addr;
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
		node* newNode = (node*)malloc(sizeof(_top->_size) + (size < sizeof(_top->_next) ? sizeof(_top->_next) : size));
		newNode->_size = size;
		return newNode->_addr;
	}

	void deallocate(void* p)
	{
		node* dp = (node*)((char*)p - sizeof(_top->_size));
		std::lock_guard<MUTEX> lg(*this);
		dp->_next = _top;
		_top = dp;
	}
private:
	node* _top;
#if (_DEBUG || DEBUG)
	size_t _nodeCount;
#endif
};
//////////////////////////////////////////////////////////////////////////

class reusable_mem : public reusable_mem_mt<null_mutex> {};

//////////////////////////////////////////////////////////////////////////

template <typename _Ty = void, typename _Reusalble = reusable_mem>
struct reusable_alloc;

template <typename _Reusalble>
struct reusable_alloc<void, _Reusalble>
{
	typedef void value_type;

	template <typename _Other>
	struct rebind
	{
		typedef reusable_alloc<_Other, _Reusalble> other;
	};

	reusable_alloc(_Reusalble& reu)
		:_reuMem(&reu) {}

	_Reusalble* _reuMem;
};

template <typename _Ty, typename _Reusalble>
struct reusable_alloc
{
	typedef typename std::allocator<_Ty>::pointer pointer;
	typedef typename std::allocator<_Ty>::difference_type difference_type;
	typedef typename std::allocator<_Ty>::reference reference;
	typedef typename std::allocator<_Ty>::const_pointer const_pointer;
	typedef typename std::allocator<_Ty>::const_reference const_reference;
	typedef typename std::allocator<_Ty>::size_type size_type;
	typedef typename std::allocator<_Ty>::value_type value_type;

	template <typename _Other>
	struct rebind
	{
		typedef reusable_alloc<_Other, _Reusalble> other;
	};
public:
	reusable_alloc()
		:_reuMem(NULL) {}

	reusable_alloc(_Reusalble& reu)
		:_reuMem(&reu) {}

	template <typename _Other>
	reusable_alloc(const reusable_alloc<_Other, _Reusalble>& reu)
		:_reuMem(reu._reuMem) {}

	void* allocate(size_t n)
	{
		assert(_reuMem);
		assert(1 == n);
		return _reuMem->allocate(sizeof(_Ty));
	}

	void deallocate(void* p, size_t n)
	{
		assert(_reuMem);
		assert(1 == n);
		_reuMem->deallocate(p);
	}

	void destroy(_Ty* p)
	{
		p->~_Ty();
	}

	_Reusalble* _reuMem;
};
//////////////////////////////////////////////////////////////////////////
template <typename _Ty = void, typename _All = mem_alloc_mt<>>
class pool_alloc_mt;

template <typename _All>
class pool_alloc_mt<void, _All>
{
public:
	typedef void value_type;

	template <typename _Other>
	struct rebind
	{
		typedef pool_alloc_mt<_Other, _All> other;
	};

	template <typename _Other>
	struct try_rebind
	{
		typedef pool_alloc_mt<_Other, _All> other;
	};

	pool_alloc_mt(size_t poolSize, bool shared = true)
		:_poolSize(poolSize), _shared(shared) {}

	size_t _poolSize;
	bool _shared;
};

template <typename _Ty, typename _All>
class pool_alloc_mt
{
public:
	typedef _Ty node_type;
	typedef typename _All::template rebind_no_mt<_Ty>::other mem_alloc_type;
	typedef typename _All::template rebind<_Ty>::other mem_alloc_mt_type;
	typedef typename std::allocator<_Ty>::pointer pointer;
	typedef typename std::allocator<_Ty>::difference_type difference_type;
	typedef typename std::allocator<_Ty>::reference reference;
	typedef typename std::allocator<_Ty>::const_pointer const_pointer;
	typedef typename std::allocator<_Ty>::const_reference const_reference;
	typedef typename std::allocator<_Ty>::size_type size_type;
	typedef typename std::allocator<_Ty>::value_type value_type;

	template <typename _Other>
	struct rebind
	{
		typedef pool_alloc_mt<_Other, _All> other;
	};

	template <typename _Other>
	struct try_rebind
	{
		typedef pool_alloc_mt<_Ty, _All> other;
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

	pool_alloc_mt(const pool_alloc_mt<void, _All>& s)
		:pool_alloc_mt(s._poolSize, s._shared) {}

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

	template <typename _Other>
	pool_alloc_mt(const pool_alloc_mt<_Other, _All>& s)
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

	pool_alloc_mt& operator=(const pool_alloc_mt<void, _All>& s)
	{
		assert(false);
		return *this;
	}

	template <typename _Other>
	pool_alloc_mt& operator=(const pool_alloc_mt<_Other, _All>& s)
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

	template <class _Uty>
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

template <typename _Ty = void, typename _All = mem_alloc<>>
class pool_alloc;

template <typename _All>
class pool_alloc<void, _All>
{
public:
	typedef void value_type;

	template <typename _Other>
	struct rebind
	{
		typedef pool_alloc<_Other, _All> other;
	};

	template <typename _Other>
	struct try_rebind 
	{
		typedef pool_alloc<_Other, _All> other;
	};

	pool_alloc(size_t poolSize = sizeof(void*))
		:_poolSize(poolSize) {}

	size_t _poolSize;
};

template <typename _Ty, typename _All>
class pool_alloc
{
public:
	typedef _Ty node_type;
	typedef typename _All::template rebind<_Ty>::other mem_alloc_type;
	typedef typename std::allocator<_Ty>::pointer pointer;
	typedef typename std::allocator<_Ty>::difference_type difference_type;
	typedef typename std::allocator<_Ty>::reference reference;
	typedef typename std::allocator<_Ty>::const_pointer const_pointer;
	typedef typename std::allocator<_Ty>::const_reference const_reference;
	typedef typename std::allocator<_Ty>::size_type size_type;
	typedef typename std::allocator<_Ty>::value_type value_type;

	template <typename _Other>
	struct rebind
	{
		typedef pool_alloc<_Other, _All> other;
	};

	template <typename _Other>
	struct try_rebind
	{
		typedef pool_alloc<_Ty, _All> other;
	};

	pool_alloc(size_t poolSize)
		:_memAlloc(poolSize)
	{
	}

	~pool_alloc()
	{
	}

	pool_alloc(const pool_alloc<void, _All>& s)
		:_memAlloc(s._poolSize) {}

	pool_alloc(const pool_alloc& s)
		:_memAlloc(s._memAlloc._poolMaxSize) {}

	template <typename _Other>
	pool_alloc(const pool_alloc<_Other, _All>& s)
		: _memAlloc(s._memAlloc._poolMaxSize) {}

	pool_alloc& operator=(const pool_alloc& s)
	{
		assert(false);
		return *this;
	}

	template <typename _Other>
	pool_alloc& operator=(const pool_alloc<_Other, _All>& s)
	{
		assert(false);
		return *this;
	}

	bool operator==(const pool_alloc<void, _All>& s)
	{
		return true;
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

	template <class _Uty>
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

template <typename T, typename CREATER, typename DESTROYER, typename MUTEX>
class SharedObjPool_;

template <typename T, typename CREATER, typename DESTROYER, typename MUTEX>
class SharedObjPool2_;

template <typename T, typename CREATER, typename DESTROYER, typename MUTEX>
class ObjPool_;

template <typename T, typename CREATER, typename DESTROYER, typename MUTEX>
class ObjPool2_;

template <typename T>
class obj_pool
{
public:
	virtual ~obj_pool(){};
	virtual	T* pick() = 0;
	virtual void recycle(T* p) = 0;
};

template <typename T, typename MUTEX, typename CREATER, typename DESTROYER>
static obj_pool<T>* create_pool_mt(size_t poolSize, CREATER&& creater, DESTROYER&& destroyer)
{
	return new ObjPool_<T, RM_CREF(CREATER), RM_CREF(DESTROYER), MUTEX>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
}

template <typename T, typename MUTEX, typename CREATER>
static obj_pool<T>* create_pool_mt(size_t poolSize, CREATER&& creater)
{
	return create_pool_mt<T, MUTEX>(poolSize, TRY_MOVE(creater), [](T* p)->bool
	{
		typedef T type;
		p->~type();
		return true;
	});
}

template <typename T, typename MUTEX>
static obj_pool<T>* create_pool_mt(size_t poolSize)
{
	return create_pool_mt<T, MUTEX>(poolSize, [](void* p)
	{
		new(p)T();
	}, [](T* p)->bool
	{
		typedef T type;
		p->~type();
		return true;
	});
}

template <typename T, typename CREATER, typename DESTROYER>
static obj_pool<T>* create_pool(size_t poolSize, CREATER&& creater, DESTROYER&& destroyer)
{
	return create_pool_mt<T, null_mutex>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
}

template <typename T, typename CREATER>
static obj_pool<T>* create_pool(size_t poolSize, CREATER&& creater)
{
	return create_pool_mt<T, null_mutex>(poolSize, TRY_MOVE(creater));
}

template <typename T>
static obj_pool<T>* create_pool(size_t poolSize)
{
	return create_pool_mt<T, null_mutex>(poolSize);
}

template <typename T, typename MUTEX, typename CREATER, typename DESTROYER>
static obj_pool<T>* create_pool_mt2(size_t poolSize, CREATER&& creater, DESTROYER&& destroyer)
{
	return new ObjPool2_<T, RM_CREF(CREATER), RM_CREF(DESTROYER), MUTEX>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
}

template <typename T, typename MUTEX, typename CREATER>
static obj_pool<T>* create_pool_mt2(size_t poolSize, CREATER&& creater)
{
	return create_pool_mt2<T, MUTEX>(poolSize, TRY_MOVE(creater), [](T* p)->bool
	{
		typedef T type;
		p->~type();
		return true;
	});
}

template <typename T, typename MUTEX>
static obj_pool<T>* create_pool_mt2(size_t poolSize)
{
	return create_pool_mt2<T, MUTEX>(poolSize, [](void* p)
	{
		new(p)T();
	}, [](T* p)->bool
	{
		typedef T type;
		p->~type();
		return true;
	});
}

template <typename T, typename CREATER, typename DESTROYER>
static obj_pool<T>* create_pool2(size_t poolSize, CREATER&& creater, DESTROYER&& destroyer)
{
	return create_pool_mt2<T, null_mutex>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
}

template <typename T, typename CREATER>
static obj_pool<T>* create_pool2(size_t poolSize, CREATER&& creater)
{
	return create_pool_mt2<T, null_mutex>(poolSize, TRY_MOVE(creater));
}

template <typename T>
static obj_pool<T>* create_pool2(size_t poolSize)
{
	return create_pool_mt2<T, null_mutex>(poolSize);
}

template <typename T, typename CREATER, typename DESTROYER, typename MUTEX>
class ObjPool_ : protected MUTEX, public obj_pool<T>
{
	struct node
	{
		__space_align char _data[sizeof(T)];
		node* _link;
	};
public:
	template <typename Creater, typename Destroyer>
	ObjPool_(size_t poolSize, Creater&& creater, Destroyer&& destroyer)
		:_creater(TRY_MOVE(creater)), _destroyer(TRY_MOVE(destroyer)), _poolSize(poolSize), _nodeCount(0), _link(NULL)
	{
#if (_DEBUG || DEBUG)
		_blockNumber = 0;
#endif
	}
public:
	~ObjPool_()
	{
		std::lock_guard<MUTEX> lg(*this);
		assert(0 == _blockNumber);
		node* it = _link;
		while (it)
		{
			assert(_nodeCount > 0);
			_nodeCount--;
			node* t = it;
			it = it->_link;
			bool ok = _destroyer((T*)t->_data);
			assert(ok);
			free(t);
		}
		assert(0 == _nodeCount);
	}
public:
	T* pick()
	{
		{
			std::lock_guard<MUTEX> lg(*this);
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
		try
		{
			_creater(newNode);
		}
		catch (...)
		{
#if (_DEBUG || DEBUG)
			MUTEX::lock();
			_blockNumber--;
			MUTEX::unlock();
#endif
			free(newNode);
			throw;
		}
		return (T*)newNode->_data;
	}

	void recycle(T* p)
	{
		{
			std::lock_guard<MUTEX> lg(*this);
#if (_DEBUG || DEBUG)
			_blockNumber--;
#endif
			if (_nodeCount < _poolSize)
			{
				_nodeCount++;
				((node*)p)->_link = _link;
				_link = (node*)p;
				return;
			}
		}
		if (_destroyer(p))
		{
			free(p);
		}
		else
		{
			std::lock_guard<MUTEX> lg(*this);
			_nodeCount++;
			((node*)p)->_link = _link;
			_link = (node*)p;
		}
	}
private:
	CREATER _creater;
	DESTROYER _destroyer;
	node* _link;
	size_t _poolSize;
	size_t _nodeCount;
#if (_DEBUG || DEBUG)
	size_t _blockNumber;
#endif
};

template <typename T, typename CREATER, typename DESTROYER, typename MUTEX>
class ObjPool2_ : protected MUTEX, public obj_pool<T>
{
	struct node
	{
		__space_align char _data[sizeof(T)];
		node* _link;
	};
public:
	template <typename Creater, typename Destroyer>
	ObjPool2_(size_t poolSize, Creater&& creater, Destroyer&& destroyer)
		:_creater(TRY_MOVE(creater)), _destroyer(TRY_MOVE(destroyer)), _nodeAlloc(poolSize), _nodeCount(0), _link(NULL)
	{
#if (_DEBUG || DEBUG)
		_blockNumber = 0;
#endif
	}
public:
	~ObjPool2_()
	{
		std::lock_guard<MUTEX> lg(*this);
		assert(0 == _blockNumber);
		node* it = _link;
		while (it)
		{
			assert(_nodeCount > 0);
			_nodeCount--;
			node* t = it;
			it = it->_link;
			bool ok = _destroyer((T*)t->_data);
			assert(ok);
			_nodeAlloc.deallocate(t);
		}
		assert(0 == _nodeCount);
	}
public:
	T* pick()
	{
		{
			std::lock_guard<MUTEX> lg(*this);
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
		node* newNode = (node*)_nodeAlloc.allocate();
		assert((void*)newNode == (void*)newNode->_data);
		try
		{
			_creater(newNode);
		}
		catch (...)
		{
#if (_DEBUG || DEBUG)
			MUTEX::lock();
			_blockNumber--;
			MUTEX::unlock();
#endif
			_nodeAlloc.deallocate(newNode);
			throw;
		}
		return (T*)newNode->_data;
	}

	void recycle(T* p)
	{
		{
			std::lock_guard<MUTEX> lg(*this);
#if (_DEBUG || DEBUG)
			_blockNumber--;
#endif
			if (_nodeCount < _nodeAlloc.pool_size())
			{
				_nodeCount++;
				((node*)p)->_link = _link;
				_link = (node*)p;
				return;
			}
		}
		if (_destroyer(p))
		{
			_nodeAlloc.deallocate(p);
		}
		else
		{
			std::lock_guard<MUTEX> lg(*this);
			_nodeCount++;
			((node*)p)->_link = _link;
			_link = (node*)p;
		}
	}
private:
	CREATER _creater;
	DESTROYER _destroyer;
	mem_alloc2<node> _nodeAlloc;
	node* _link;
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

template <typename T, typename MUTEX, typename CREATER, typename DESTROYER>
static shared_obj_pool<T>* create_shared_pool_mt(size_t poolSize, CREATER&& creater, DESTROYER&& destroyer)
{
	return new SharedObjPool_<T, RM_CREF(CREATER), RM_CREF(DESTROYER), MUTEX>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
}

template <typename T, typename MUTEX, typename CREATER>
static shared_obj_pool<T>* create_shared_pool_mt(size_t poolSize, CREATER&& creater)
{
	return create_shared_pool_mt<T, MUTEX>(poolSize, TRY_MOVE(creater), [](T* p)->bool
	{
		typedef T type;
		p->~type();
		return true;
	});
}

template <typename T, typename MUTEX>
static shared_obj_pool<T>* create_shared_pool_mt(size_t poolSize)
{
	return create_shared_pool_mt<T, MUTEX>(poolSize, [](void* p)
	{
		new(p)T();
	}, [](T* p)->bool
	{
		typedef T type;
		p->~type();
		return true;
	});
}

template <typename T, typename CREATER, typename DESTROYER>
static shared_obj_pool<T>* create_shared_pool(size_t poolSize, CREATER&& creater, DESTROYER&& destroyer)
{
	return create_shared_pool_mt<T, null_mutex>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
}

template <typename T, typename CREATER>
static shared_obj_pool<T>* create_shared_pool(size_t poolSize, CREATER&& creater)
{
	return create_shared_pool_mt<T, null_mutex>(poolSize, TRY_MOVE(creater));
}

template <typename T>
static shared_obj_pool<T>* create_shared_pool(size_t poolSize)
{
	return create_shared_pool_mt<T, null_mutex>(poolSize);
}

template <typename T, typename MUTEX, typename CREATER, typename DESTROYER>
static shared_obj_pool<T>* create_shared_pool_mt2(size_t poolSize, CREATER&& creater, DESTROYER&& destroyer)
{
	return new SharedObjPool2_<T, RM_CREF(CREATER), RM_CREF(DESTROYER), MUTEX>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
}

template <typename T, typename MUTEX, typename CREATER>
static shared_obj_pool<T>* create_shared_pool_mt2(size_t poolSize, CREATER&& creater)
{
	return create_shared_pool_mt2<T, MUTEX>(poolSize, TRY_MOVE(creater), [](T* p)->bool
	{
		typedef T type;
		p->~type();
		return true;
	});
}

template <typename T, typename MUTEX>
static shared_obj_pool<T>* create_shared_pool_mt2(size_t poolSize)
{
	return create_shared_pool_mt2<T, MUTEX>(poolSize, [](void* p)
	{
		new(p)T();
	}, [](T* p)->bool
	{
		typedef T type;
		p->~type();
		return true;
	});
}

template <typename T, typename CREATER, typename DESTROYER>
static shared_obj_pool<T>* create_shared_pool2(size_t poolSize, CREATER&& creater, DESTROYER&& destroyer)
{
	return create_shared_pool_mt2<T, null_mutex>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
}

template <typename T, typename CREATER>
static shared_obj_pool<T>* create_shared_pool2(size_t poolSize, CREATER&& creater)
{
	return create_shared_pool_mt2<T, null_mutex>(poolSize, TRY_MOVE(creater));
}

template <typename T>
static shared_obj_pool<T>* create_shared_pool2(size_t poolSize)
{
	return create_shared_pool_mt2<T, null_mutex>(poolSize);
}

struct RefAlloc_ 
{
	mem_alloc_base*& _refCountAlloc;
	size_t _nodeCount;
};

template <typename _Tp, typename ALLOC>
class CreateRefAlloc_;

template <typename ALLOC>
class CreateRefAlloc_<void, ALLOC>
{
public:
	typedef size_t      size_type;
	typedef void*       pointer;
	typedef const void* const_pointer;
	typedef void        value_type;

	template <typename _Tp1>
	struct rebind
	{
		typedef CreateRefAlloc_<_Tp1, ALLOC> other;
	};

	CreateRefAlloc_(RefAlloc_* refAll)
 		:_refAll(refAll) {}

	RefAlloc_* _refAll;
};

template <typename _Tp, typename ALLOC>
class CreateRefAlloc_
{
	typedef typename ALLOC::template rebind<_Tp>::other alloc_type;
public:
	typedef size_t     size_type;
	typedef _Tp*       pointer;
	typedef const _Tp* const_pointer;
	typedef _Tp&       reference;
	typedef const _Tp& const_reference;
	typedef _Tp        value_type;

	template <typename _Tp1>
	struct rebind
	{
		typedef CreateRefAlloc_<_Tp1, ALLOC> other;
	};

	CreateRefAlloc_(RefAlloc_* refAll)
		:_refAll(refAll) {}

	CreateRefAlloc_(const CreateRefAlloc_& s)
		:_refAll(s._refAll) {}

	template <typename _Tp1>
	CreateRefAlloc_(const CreateRefAlloc_<_Tp1, ALLOC>& s)
		: _refAll(s._refAll) {}

	~CreateRefAlloc_()
	{}

	pointer allocate(size_type n, const void* = 0)
	{
		assert(1 == n);
		_refAll->_refCountAlloc = new alloc_type(_refAll->_nodeCount);
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

template <typename _Tp, typename _Ty, typename ALLOC>
class CreateSharedSpaceAlloc_;

template <typename _Ty, typename ALLOC>
class CreateSharedSpaceAlloc_<void, _Ty, ALLOC>
{
public:
	typedef size_t      size_type;
	typedef void*       pointer;
	typedef const void* const_pointer;
	typedef void        value_type;

	template <typename _Tp1>
	struct rebind
	{
		typedef CreateSharedSpaceAlloc_<_Tp1, _Ty, ALLOC> other;
	};

	CreateSharedSpaceAlloc_(RefAlloc_* refAll)
		:_refAll(refAll) {}

	void* const _pl = NULL;
	RefAlloc_* _refAll;
};

template <typename _Tp, typename _Ty, typename ALLOC>
class CreateSharedSpaceAlloc_
{
	struct obj_space
	{
		__space_align char _ty[sizeof(_Ty)];
		__space_align char _tp[sizeof(_Tp)];
	};
	typedef typename ALLOC::template rebind<obj_space>::other alloc_type;
public:
	typedef size_t     size_type;
	typedef _Tp*       pointer;
	typedef const _Tp* const_pointer;
	typedef _Tp&       reference;
	typedef const _Tp& const_reference;
	typedef _Tp        value_type;

	template <typename _Tp1>
	struct rebind
	{
		typedef CreateSharedSpaceAlloc_<_Tp1, _Ty, ALLOC> other;
	};

	CreateSharedSpaceAlloc_(RefAlloc_* refAll)
		:_refAll(refAll) {}

	CreateSharedSpaceAlloc_(const CreateSharedSpaceAlloc_& s)
		:_refAll(s._refAll) {}

	template <typename _Tp1>
	CreateSharedSpaceAlloc_(const CreateSharedSpaceAlloc_<_Tp1, _Ty, ALLOC>& s)
		: _refAll(s._refAll) {}

	~CreateSharedSpaceAlloc_()
	{}

	pointer allocate(size_type n, const void* = 0)
	{
		assert(1 == n);
		_refAll->_refCountAlloc = new alloc_type(_refAll->_nodeCount);
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

	void* const _pl = NULL;
	RefAlloc_* _refAll;
};

template <typename _Tp>
class ref_count_alloc;

template <>
class ref_count_alloc<void>
{
public:
	typedef size_t      size_type;
	typedef void*       pointer;
	typedef const void* const_pointer;
	typedef void        value_type;

	template <typename _Tp1>
	struct rebind
	{
		typedef ref_count_alloc<_Tp1> other;
	};

	ref_count_alloc(mem_alloc_base* refCountAlloc)
		:_refCountAlloc(refCountAlloc)
	{
		static_assert(sizeof(ref_count_alloc<void>) == sizeof(CreateRefAlloc_<void, mem_alloc<void>>), "");
	}

	mem_alloc_base* _refCountAlloc;
};

template <typename _Tp>
class ref_count_alloc
{
public:
	typedef size_t     size_type;
	typedef _Tp*       pointer;
	typedef const _Tp* const_pointer;
	typedef _Tp&       reference;
	typedef const _Tp& const_reference;
	typedef _Tp        value_type;

	template <typename _Tp1>
	struct rebind
	{
		typedef ref_count_alloc<_Tp1> other;
	};

	ref_count_alloc(mem_alloc_base* refCountAlloc)
		:_refCountAlloc(refCountAlloc) {}

	ref_count_alloc(const ref_count_alloc& s)
		:_refCountAlloc(s._refCountAlloc) {}

	template <typename _Tp1>
	ref_count_alloc(const ref_count_alloc<_Tp1>& s)
		: _refCountAlloc(s._refCountAlloc)
	{
		assert(_refCountAlloc->alloc_size() >= sizeof(_Tp));
	}

	~ref_count_alloc()
	{
		static_assert(sizeof(ref_count_alloc<_Tp>) == sizeof(CreateRefAlloc_<_Tp, mem_alloc<void>>), "");
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

template <typename _Ty, typename _Tp = void>
class ref_count_alloc2;

template <typename _Ty>
class ref_count_alloc2<_Ty, void>
{
public:
	typedef size_t      size_type;
	typedef void*       pointer;
	typedef const void* const_pointer;
	typedef void        value_type;

	template <typename _Tp1>
	struct rebind
	{
		typedef ref_count_alloc2<_Ty, _Tp1> other;
	};

	ref_count_alloc2(void* space, mem_alloc_base* refCountAlloc)
		:_space(space), _refCountAlloc(refCountAlloc)
	{
		static_assert(sizeof(ref_count_alloc2<_Ty, void>) == sizeof(CreateSharedSpaceAlloc_<void, _Ty, mem_alloc<void>>), "");
	}

	void* _space;
	mem_alloc_base* _refCountAlloc;
};

template <typename _Ty, typename _Tp>
class ref_count_alloc2
{
public:
	typedef size_t     size_type;
	typedef _Tp*       pointer;
	typedef const _Tp* const_pointer;
	typedef _Tp&       reference;
	typedef const _Tp& const_reference;
	typedef _Tp        value_type;

	template <typename _Tp1>
	struct rebind
	{
		typedef ref_count_alloc2<_Ty, _Tp1> other;
	};

	ref_count_alloc2(void* space, mem_alloc_base* refCountAlloc)
		:_space(space), _refCountAlloc(refCountAlloc) {}

	ref_count_alloc2(const ref_count_alloc2& s)
		:_space(s._space), _refCountAlloc(s._refCountAlloc) {}

	template <typename _Tp1>
	ref_count_alloc2(const ref_count_alloc2<_Ty, _Tp1>& s)
		: _space(s._space), _refCountAlloc(s._refCountAlloc)
	{
		assert(_refCountAlloc->alloc_size() >= sizeof(_Tp));
	}

	~ref_count_alloc2()
	{
		static_assert(sizeof(ref_count_alloc2<_Ty, _Tp>) == sizeof(CreateSharedSpaceAlloc_<_Tp, _Ty, mem_alloc<void>>), "");
	}

	pointer allocate(size_type n, const void* = 0)
	{
		assert(1 == n);
		return (pointer)((char*)_space + MEM_ALIGN(sizeof(_Ty), sizeof(void*)));
	}

	void deallocate(pointer p, size_type n)
	{
		assert(1 == n);
		assert((pointer)((char*)_space + MEM_ALIGN(sizeof(_Ty), sizeof(void*))) == p);
		_refCountAlloc->deallocate(_space);
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

	void* _space;
	mem_alloc_base* _refCountAlloc;
};

template <typename T, typename ALLOC, typename DESTROYER>
mem_alloc_base* make_ref_count_alloc(size_t poolSize, DESTROYER&& destroyer)
{
	mem_alloc_base* refCountAlloc = NULL;
	try
	{
		RefAlloc_ refAll = { refCountAlloc, poolSize };
		std::shared_ptr<T>(NULL, TRY_MOVE(destroyer), CreateRefAlloc_<void, ALLOC>(&refAll));
	}
	catch (...) {}
	return refCountAlloc;
}

template <typename T, typename ALLOC, typename DESTROYER>
mem_alloc_base* make_shared_space_alloc(size_t poolSize, DESTROYER&& destroyer)
{
	mem_alloc_base* refCountAlloc = NULL;
	try
	{
		RefAlloc_ refAll = { refCountAlloc, poolSize };
		std::shared_ptr<T>(NULL, TRY_MOVE(destroyer), CreateSharedSpaceAlloc_<void, T, ALLOC>(&refAll));
	}
	catch (...) {}
	return refCountAlloc;
}

template <typename Class, typename... Args>
std::shared_ptr<Class> make_shared_space_obj(mem_alloc_base* alloc, Args&&... args)
{
	void* space = alloc->allocate();
	return std::shared_ptr<Class>(new(space)Class(std::forward<Args>(args)...), [](Class* p)
	{
		p->~Class();
	}, ref_count_alloc2<Class>(space, alloc));
}

template <typename T, typename CREATER, typename DESTROYER, typename MUTEX>
class SharedObjPool_ : public shared_obj_pool<T>
{
public:
	template <typename Creater, typename Destroyer>
	SharedObjPool_(size_t poolSize, Creater&& creater, Destroyer&& destroyer)
	{
		_refCountAlloc = make_ref_count_alloc<T, mem_alloc_mt<void, MUTEX>>(poolSize, [this](T*){});
		_dataAlloc = create_pool_mt<T, MUTEX>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
	}
public:
	~SharedObjPool_()
	{
		delete _dataAlloc;
		delete _refCountAlloc;
	}
public:
	std::shared_ptr<T> pick()
	{
		return std::shared_ptr<T>(_dataAlloc->pick(), [this](T* p){_dataAlloc->recycle(p); }, ref_count_alloc<void>(_refCountAlloc));
	}
private:
	obj_pool<T>* _dataAlloc;
	mem_alloc_base* _refCountAlloc;
};

template <typename T, typename CREATER, typename DESTROYER, typename MUTEX>
class SharedObjPool2_ : public shared_obj_pool<T>
{
public:
	template <typename Creater, typename Destroyer>
	SharedObjPool2_(size_t poolSize, Creater&& creater, Destroyer&& destroyer)
	{
		_refCountAlloc = make_ref_count_alloc<T, mem_alloc_mt2<void, MUTEX>>(poolSize, [this](T*){});
		_dataAlloc = create_pool_mt2<T, MUTEX>(poolSize, TRY_MOVE(creater), TRY_MOVE(destroyer));
	}
public:
	~SharedObjPool2_()
	{
		delete _dataAlloc;
		delete _refCountAlloc;
	}
public:
	std::shared_ptr<T> pick()
	{
		return std::shared_ptr<T>(_dataAlloc->pick(), [this](T* p){_dataAlloc->recycle(p); }, ref_count_alloc<void>(_refCountAlloc));
	}
private:
	obj_pool<T>* _dataAlloc;
	mem_alloc_base* _refCountAlloc;
};

class my_actor;
struct shared_bool
{
	friend my_actor;
	shared_bool();
	shared_bool(const shared_bool& s);
	shared_bool(shared_bool&& s);
	explicit shared_bool(const std::shared_ptr<bool>& pb);
	explicit shared_bool(std::shared_ptr<bool>&& pb);
	void operator=(const shared_bool& s);
	void operator=(shared_bool&& s);
	void operator=(bool b);
	operator bool() const;
	bool empty() const;
	void reset();
	static shared_bool new_(bool b = false);
private:
	std::shared_ptr<bool> _ptr;
	static mem_alloc_base* _sharedBoolAlloc;
};
#endif