#include "coro_choice.h"
#include "context_yield.h"

#ifdef BOOST_CORO

#include <boost/coroutine/asymmetric_coroutine.hpp>
typedef boost::coroutines::coroutine<void>::pull_type boost_pull_type;
typedef boost::coroutines::coroutine<void>::push_type boost_push_type;
typedef boost::coroutines::attributes coro_attributes;

#elif (defined LIB_CORO)

#ifdef _MSC_VER
#include "scattered.h"

#ifdef _MSC_VER
#ifdef _WIN64
#pragma comment(lib, NAME_BOND(__FILE__, "./../context_yield64.lib"))
#else
#pragma comment(lib, NAME_BOND(__FILE__, "./../context_yield.lib"))
#endif // _WIN64
#endif

struct coro_pull_interface;
struct coro_push_interface;

typedef void(__stdcall *coro_push_handler)(coro_push_interface&, void* param);

struct coro_push_interface
{
	virtual void operator()() = 0;
};

struct coro_pull_interface
{
	virtual void operator()() = 0;
	virtual void destory() = 0;
};

extern "C"
{
	coro_pull_interface* make_coro(coro_push_handler ch, void* stackTop, size_t size, void* param = 0);
}

#elif __GNUG__
#include <ucontext.h>
#include <malloc.h>
#endif

#elif (defined FIBER_CORO)

#include <Windows.h>

#endif

namespace context_yield
{
#ifdef BOOST_CORO

	struct actor_stack_allocate
	{
		actor_stack_allocate() {}

		actor_stack_allocate(void** sp, size_t* size)
		{
			_sp = sp;
			_size = size;
		}

		void allocate(boost::coroutines::stack_context & stackCon, size_t size)
		{
			boost::coroutines::stack_allocator all;
			all.allocate(stackCon, size);
			*_sp = stackCon.sp;
			*_size = stackCon.size;
		}

		void deallocate(boost::coroutines::stack_context & stackCon)
		{
			boost::coroutines::stack_allocator all;
			all.deallocate(stackCon);
		}

		void** _sp;
		size_t* _size;
	};

	context_yield::coro_info* make_context(size_t stackSize, context_yield::context_handler handler, void* p)
	{
		context_yield::coro_info* info = new context_yield::coro_info;
		try
		{
			info->obj = new boost_pull_type([info, handler, p](boost_push_type& push)
			{
				info->nc = &push;
				handler(info, p);
			}, boost::coroutines::attributes(stackSize), actor_stack_allocate(&info->stackTop, &info->stackSize));
			return info;
		}
		catch (...)
		{
			delete info;
			return NULL;
		}
	}

	void push_yield(context_yield::coro_info* info)
	{
		(*(boost_push_type*)info->nc)();
	}

	void pull_yield(context_yield::coro_info* info)
	{
		(*(boost_pull_type*)info->obj)();
	}

	void delete_context(context_yield::coro_info* info)
	{
		delete (boost_pull_type*)info->obj;
		delete info;
	}

#elif (defined LIB_CORO)

#ifdef _MSC_VER
	context_yield::coro_info* make_context(size_t stackSize, context_yield::context_handler handler, void* p)
	{
		void* stack = malloc(stackSize);
		if (!stack)
		{
			return NULL;
		}
		context_yield::coro_info* info = new context_yield::coro_info;
		info->stackTop = (char*)stack + stackSize;
		info->stackSize = stackSize;
		struct local_ref
		{
			context_yield::context_handler handler;
			void* p;
			context_yield::coro_info* info;
		} ref = { handler, p, info };
		info->obj = make_coro([](coro_push_interface& push, void* param)
		{
			local_ref* ref = (local_ref*)param;
			ref->info->nc = &push;
			ref->handler(ref->info, ref->p);
		}, info->stackTop, info->stackSize, &ref);
		return info;
	}

	void push_yield(context_yield::coro_info* info)
	{
		(*(coro_push_interface*)info->nc)();
	}

	void pull_yield(context_yield::coro_info* info)
	{
		(*(coro_pull_interface*)info->obj)();
	}

	void delete_context(context_yield::coro_info* info)
	{
		((coro_pull_interface*)info->obj)->destory();
		free((char*)info->stackTop - info->stackSize);
		delete info;
	}
#elif __GNUG__
	context_yield::coro_info* make_context(size_t stackSize, context_yield::context_handler handler, void* p)
	{
		void* stack = malloc(stackSize);
		if (!stack)
		{
			return NULL;
		}
		context_yield::coro_info* info = new context_yield::coro_info;
		info->stackTop = (char*)stack + stackSize;
		info->stackSize = stackSize;
		info->obj = new ucontext_t;
		info->nc = new ucontext_t;
		ucontext_t* returnCt = (ucontext_t*)info->nc;
		ucontext_t* callCt = (ucontext_t*)info->obj;
		getcontext(callCt);
		callCt->uc_link = returnCt;
		callCt->uc_stack.ss_sp = stack;
		callCt->uc_stack.ss_size = stackSize;
		makecontext(callCt, (void(*)())handler, 2, info, p);
		swapcontext(returnCt, callCt);
		return info;
	}

	void push_yield(context_yield::coro_info* info)
	{
		ucontext_t* returnCt = (ucontext_t*)info->nc;
		ucontext_t* callCt = (ucontext_t*)info->obj;
		swapcontext(callCt, returnCt);
	}

	void pull_yield(context_yield::coro_info* info)
	{
		ucontext_t* returnCt = (ucontext_t*)info->nc;
		ucontext_t* callCt = (ucontext_t*)info->obj;
		swapcontext(returnCt, callCt);
	}

	void delete_context(context_yield::coro_info* info)
	{
		ucontext_t* returnCt = (ucontext_t*)info->nc;
		ucontext_t* callCt = (ucontext_t*)info->obj;
		free((char*)info->stackTop - info->stackSize);
		delete info;
		delete callCt;
		delete returnCt;
	}
#endif

#elif (defined FIBER_CORO)

	struct fiber_struct
	{
		void* _param;
		void* _seh;
		void* _stackTop;
		void* _stackBottom;
	};

	context_yield::coro_info* make_context(size_t stackSize, context_yield::context_handler handler, void* p)
	{
		context_yield::coro_info* info = new context_yield::coro_info;
		info->stackSize = stackSize;
		struct local_ref
		{
			context_yield::context_handler handler;
			void* p;
			context_yield::coro_info* info;
		} ref = { handler, p, info };
#ifdef _WIN64
		info->obj = CreateFiberEx(stackSize, 64 * 1024, FIBER_FLAG_FLOAT_SWITCH, [](void* param)
#else
		info->obj = CreateFiberEx(stackSize, 0, FIBER_FLAG_FLOAT_SWITCH, [](void* param)
#endif
		{
			local_ref* ref = (local_ref*)param;
			ref->handler(ref->info, ref->p);
		}, &ref);
		if (!info->obj)
		{
			delete info;
			return NULL;
		}
		info->stackTop = ((fiber_struct*)info->obj)->_stackTop;
		pull_yield(info);
		return info;
	}

	void push_yield(context_yield::coro_info* info)
	{
		SwitchToFiber(info->nc);
	}

	void pull_yield(context_yield::coro_info* info)
	{
		info->nc = GetCurrentFiber();
		SwitchToFiber(info->obj);
	}

	void delete_context(context_yield::coro_info* info)
	{
		DeleteFiber(info->obj);
		delete info;
	}

#endif
}