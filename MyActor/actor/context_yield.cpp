#include "context_yield.h"
#include "check_actor_stack.h"
#include "scattered.h"

namespace context_yield
{
#ifdef WIN32
#include <Windows.h>
	struct fiber_struct
	{
		void* _param;
		void* _seh;
		void* _stackTop;
		void* _stackBottom;
	};

	void adjust_stack(context_yield::coro_info* info)
	{
		char* const sb = (char*)info->stackTop - info->stackSize - info->reserveSize;
		char* sp = (char*)((size_t)get_sp() & (0 - PAGE_SIZE)) - 2 * PAGE_SIZE;
		while (sp >= sb)
		{
			MEMORY_BASIC_INFORMATION mbi;
			if (sizeof(mbi) != VirtualQuery(sp, &mbi, sizeof(mbi)) || MEM_RESERVE == mbi.State)
			{
				break;
			}
			VirtualFree(sp, PAGE_SIZE, MEM_DECOMMIT);
			sp -= PAGE_SIZE;
		}
	}

	context_yield::coro_info* make_context(size_t stackSize, context_yield::context_handler handler, void* p)
	{
		size_t allocSize = MEM_ALIGN(stackSize + STACK_RESERVED_SPACE_SIZE, STACK_BLOCK_SIZE);
		context_yield::coro_info* info = new context_yield::coro_info;
		info->stackSize = stackSize;
		info->reserveSize = allocSize - info->stackSize;
		struct local_ref
		{
			context_yield::context_handler handler;
			context_yield::coro_info* info;
			void* p;
		} ref = { handler, info, p };
		info->obj = CreateFiberEx(0, allocSize, FIBER_FLAG_FLOAT_SWITCH, [](void* param)
		{
			local_ref* ref = (local_ref*)param;
			adjust_stack(ref->info);
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

#elif __linux__
#include <sys/mman.h>
	extern "C"
	{
		void* makefcontext(void* sp, size_t size, void(*fn)(void*));
		void* jumpfcontext(void** ofc, void* nfc, void* vp, bool preserve_fpu);
	}

	context_yield::coro_info* make_context(size_t stackSize, context_yield::context_handler handler, void* p)
	{
		size_t allocSize = MEM_ALIGN(stackSize + STACK_RESERVED_SPACE_SIZE, STACK_BLOCK_SIZE);
		void* stack = mmap(0, allocSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (!stack)
		{
			throw size_t(allocSize);
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
		struct local_ref
		{
			context_yield::context_handler handler;
			context_yield::coro_info* info;
			void* p;
		} ref = { handler, info, p };
		info->obj = makefcontext(info->stackTop, stackSize, [](void* param)
		{
			local_ref* ref = (local_ref*)param;
			ref->handler(ref->info, ref->p);
		});
		jumpfcontext(&info->nc, info->obj, &ref, true);
		return info;
	}

	void push_yield(context_yield::coro_info* info)
	{
		jumpfcontext(&info->obj, info->nc, NULL, true);
	}

	void pull_yield(context_yield::coro_info* info)
	{
		jumpfcontext(&info->nc, info->obj, NULL, true);
	}

	void delete_context(context_yield::coro_info* info)
	{
		munmap((char*)info->stackTop - info->stackSize - info->reserveSize, info->stackSize + info->reserveSize);
		delete info;
	}
#endif
}