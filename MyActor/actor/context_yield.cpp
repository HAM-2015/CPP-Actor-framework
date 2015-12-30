#include "coro_choice.h"
#include "context_yield.h"
#include "check_actor_stack.h"
#include "scattered.h"

#ifdef BOOST_CORO

#include <boost/coroutine/asymmetric_coroutine.hpp>
typedef boost::coroutines::coroutine<void>::pull_type boost_pull_type;
typedef boost::coroutines::coroutine<void>::push_type boost_push_type;
typedef boost::coroutines::attributes coro_attributes;

#ifdef __linux__
#include <sys/mman.h>
#endif

#elif (defined LIB_CORO)

#ifdef WIN32

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

#elif __linux__
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

		actor_stack_allocate(context_yield::coro_info* info)
			:_info(info) {}

		void allocate(boost::coroutines::stack_context & stackCon, size_t size)
		{
			size_t allocSize = MEM_ALIGN(size + STACK_RESERVED_SPACE_SIZE, STACK_BLOCK_SIZE);
#ifdef WIN32
			void* stack = VirtualAlloc(NULL, allocSize, MEM_RESERVE, PAGE_READWRITE);
			if (!stack)
			{
				throw size_t(allocSize);
			}
			_info->stackSize = size;
			_info->reserveSize = allocSize - _info->stackSize;
			_info->stackTop = (char*)stack + allocSize;
			VirtualAlloc((char*)stack + _info->reserveSize, _info->stackSize, MEM_COMMIT, PAGE_READWRITE);
			stackCon.sp = _info->stackTop;
			stackCon.size = allocSize;
#else
			void* stack = mmap(0, allocSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			if (!stack)
			{
				throw size_t(allocSize);
			}
			_info->stackTop = (char*)stack + allocSize;
#if (_DEBUG || DEBUG)
			_info->stackSize = allocSize - PAGE_SIZE;
			_info->reserveSize = PAGE_SIZE;
#else
			_info->stackSize = size;
			_info->reserveSize = allocSize - _info->stackSize;
#endif
			mprotect((char*)stack + _info->reserveSize, _info->stackSize, PROT_READ | PROT_WRITE);
			stackCon.sp = _info->stackTop;
			stackCon.size = allocSize;
#endif
		}

		void deallocate(boost::coroutines::stack_context & stackCon)
		{
#ifdef WIN32
			VirtualFree((char*)stackCon.sp - stackCon.size, 0, MEM_RELEASE);
#else
			munmap((char*)stackCon.sp - stackCon.size, stackCon.size);
#endif
		}

		context_yield::coro_info* _info;
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
			}, coro_attributes(stackSize), actor_stack_allocate(info));
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

#ifdef WIN32
	context_yield::coro_info* make_context(size_t stackSize, context_yield::context_handler handler, void* p)
	{
		size_t allocSize = MEM_ALIGN(stackSize, STACK_BLOCK_SIZE);
		void* stack = VirtualAlloc(NULL, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!stack)
		{
			return NULL;
		}
		context_yield::coro_info* info = new context_yield::coro_info;
		info->stackSize = stackSize;
		info->reserveSize = allocSize - info->stackSize;
		info->stackTop = (char*)stack + allocSize;
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
		VirtualFree((char*)info->stackTop - info->stackSize - info->reserveSize, 0, MEM_RELEASE);
		delete info;
	}
#elif __linux__
	context_yield::coro_info* make_context(size_t stackSize, context_yield::context_handler handler, void* p)
	{
		size_t allocSize = MEM_ALIGN(stackSize + STACK_RESERVED_SPACE_SIZE, STACK_BLOCK_SIZE);
		void* stack = mmap(0, allocSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (!stack)
		{
			return NULL;
		}
		context_yield::coro_info* info = new context_yield::coro_info;
		info->stackTop = (char*)stack + allocSize;
#if (_DEBUG || DEBUG)
		info->stackSize = allocSize - PAGE_SIZE;
		info->reserveSize = PAGE_SIZE;
#else
		info->stackSize = stackSize;
		info->reserveSize = allocSize - info->stackSize;
#endif
		mprotect((char*)stack + info->reserveSize, info->stackSize, PROT_READ | PROT_WRITE);
		info->obj = new ucontext_t;
		info->nc = new ucontext_t;
		ucontext_t* returnCt = (ucontext_t*)info->nc;
		ucontext_t* callCt = (ucontext_t*)info->obj;
		getcontext(callCt);
		callCt->uc_link = returnCt;
		callCt->uc_stack.ss_sp = stack;
		callCt->uc_stack.ss_size = allocSize;
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
		munmap((char*)info->stackTop - info->stackSize - info->reserveSize, info->stackSize);
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
		size_t allocSize = MEM_ALIGN(stackSize + STACK_RESERVED_SPACE_SIZE, STACK_BLOCK_SIZE);
		context_yield::coro_info* info = new context_yield::coro_info;
		info->stackSize = stackSize;
		info->reserveSize = allocSize - info->stackSize;
		struct local_ref
		{
			context_yield::context_handler handler;
			void* p;
			context_yield::coro_info* info;
		} ref = { handler, p, info };
		info->obj = CreateFiberEx(0, allocSize, FIBER_FLAG_FLOAT_SWITCH, [](void* param)
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