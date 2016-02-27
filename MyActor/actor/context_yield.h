#ifndef __CONTEXT_YIELD_H
#define __CONTEXT_YIELD_H
#include <stddef.h>

namespace context_yield
{
	struct coro_info
	{
		void* obj = 0;
		void* stackTop = 0;
		void* nc = 0;
		size_t stackSize = 0;
		size_t reserveSize = 0;
	};

	typedef void(*context_handler)(coro_info* info, void* p);
	coro_info* make_context(size_t stackSize, context_handler handler, void* p);
	void push_yield(coro_info* info);
	void pull_yield(coro_info* info);
	void delete_context(coro_info* info);
	void decommit_context(coro_info* info);
}

#endif
