#ifndef __CORO_STACK_H
#define __CORO_STACK_H

#include <boost/coroutine/stack_allocator.hpp>
#include <boost/coroutine/stack_context.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/atomic/atomic.hpp>
#include <list>
#include <vector>
#include <map>

using namespace std;

struct stack_pck 
{
	boost::coroutines::stack_context _stack;
	size_t _size;
};

/*!
@brief 协程栈池
*/
class coro_stack_pool
{
	struct stack_pool_pck 
	{
		boost::mutex _mutex;
		list<stack_pck> _pool;
	};
private:
	coro_stack_pool();
public:
	~coro_stack_pool();
	static void enable();
	static bool isEnable();
public:
	static stack_pck getStack(size_t size);
	static void recovery(stack_pck& stack);
private:
	boost::mutex _mutex;
	boost::coroutines::stack_allocator _all;
	boost::atomic<int> _stackCount;
	vector<stack_pool_pck*> _stackPool;
	static boost::shared_ptr<coro_stack_pool> _coroStackPool;
};

/*!
@brief 协程栈分配器
*/
struct coro_stack
{
	coro_stack(void** sp, size_t* size);
	coro_stack();

	~coro_stack();

	void allocate( boost::coroutines::stack_context & stackCon, size_t size);

	void deallocate( boost::coroutines::stack_context & stackCon);

	stack_pck _stack;
	void** _sp;
	size_t* _size;
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 协程中使用临时的缓存池
*/
class coro_buff_pool
{
	struct buff_pool_pck 
	{
		boost::mutex _mutex;
		list<void*> _pool;
	};
private:
	coro_buff_pool();
public:
	~coro_buff_pool();
	static void enable();
public:
	static void* getBuff(size_t size);
	static void recovery(void* ptr, size_t size);
private:
	boost::mutex _mutex;
	boost::atomic<int> _buffCount;
	map<size_t, buff_pool_pck*> _buffPool;
	static boost::shared_ptr<coro_buff_pool> _coroBuffPool;
};

template<class T>
struct allocator_base
{
	typedef T value_type;
};

template<class T>
struct allocator_base<const T>
{
	typedef T value_type;
};

/*!
@brief 协程缓存区分配器
*/
template<class T = void>
class buffer_alloc
	: public allocator_base<T>
{
public:
	typedef typename allocator_base<T>::value_type value_type;

	typedef T* pointer;

	typedef size_t size_type;

	template<class Other>
	struct rebind
	{
		typedef buffer_alloc<Other> other;
	};

	buffer_alloc()
	{

	}

	~buffer_alloc()
	{

	}

	buffer_alloc(const buffer_alloc&)
	{

	}

	template<class Other>
	buffer_alloc(const buffer_alloc<Other>&)
	{

	}

	template<class Other>
	buffer_alloc<T>& operator=(const buffer_alloc<Other>&)
	{
		return (*this);
	}

	buffer_alloc<T>& operator=(const buffer_alloc&)
	{
		return (*this);
	}

	static void deallocate(pointer ptr, size_type count)
	{
		assert(1 == count);
		coro_buff_pool::recovery(ptr, sizeof(T));
	}

	static pointer allocate(size_type count)
	{
		assert(1 == count);
		return (pointer)coro_buff_pool::getBuff(sizeof(T));
	}

	static pointer allocate(size_type count, const void)
	{
		return (allocate(count));
	}

	static void destroy(pointer ptr)
	{
		ptr->~T();
	}

	static void obj_delete(T* obj)
	{
		buffer_alloc<T> all;
		all.destroy(obj);
		all.deallocate(obj, 1);
	}

	static void obj_delete_size(size_t size, T* obj)
	{
		buffer_alloc<T> all;
		all.destroy(obj);
		all.deallocate_size(obj, size, 1);
	}

	static void obj_destroy(T* obj)
	{
		buffer_alloc<T> all;
		all.destroy(obj);
	}

	static void obj_deallocate(T* obj)
	{
		buffer_alloc<T> all;
		all.deallocate(obj, 1);
	}

	static void shared_obj_destroy(boost::shared_ptr<void> sharedPoolMem, T* obj)
	{
		buffer_alloc<T> all;
		all.destroy(obj);
	}
};

template<> class buffer_alloc<void>
{
public:
	template<class Other>
	struct rebind
	{
		typedef buffer_alloc<Other> other;
	};

	static void* obj_allocate_size(size_t size)
	{
		return coro_buff_pool::getBuff(size);
	}

	static void obj_deallocate_size(size_t size, void* obj)
	{
		coro_buff_pool::recovery(obj, size);
	}
};

#endif