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
		char* const sp = (char*)info->stackTop - 2 * PAGE_SIZE;
		VirtualAlloc(sp, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD);
		VirtualFree(sb, (size_t)sp - (size_t)sb, MEM_DECOMMIT);
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
			assert((size_t)get_sp() > (size_t)ref->info->stackTop - PAGE_SIZE + 256);
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

	void decommit_context(context_yield::coro_info* info)
	{
		adjust_stack(info);
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
		void* stack = mmap(0, allocSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);//内存足够下可能失败，调整 /proc/sys/vm/max_map_count
		if (!stack)
		{
			throw size_t(allocSize);
		}
		context_yield::coro_info* info = new context_yield::coro_info;
		info->stackTop = (char*)stack + allocSize;
		info->stackSize = stackSize;
		info->reserveSize = allocSize - info->stackSize;
		bool ok = 0 == mprotect(stack, PAGE_SIZE, PROT_NONE);//设置哨兵，可能失败，调整 /proc/sys/vm/max_map_count
		assert(ok);
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

	void decommit_context(context_yield::coro_info* info)
	{
		madvise((char*)info->stackTop - info->stackSize - info->reserveSize, info->stackSize + info->reserveSize, MADV_DONTNEED);
	}
#endif
}