#ifndef __CONTEXT_YIELD_H
#define __CONTEXT_YIELD_H
#include <stddef.h>

namespace context_yield
{
	struct context_info
	{
		void* obj = 0;
		void* stackTop = 0;
		void* nc = 0;
		size_t stackSize = 0;
		size_t reserveSize = 0;
	};

	bool is_thread_a_fiber();
	void convert_thread_to_fiber();
	void convert_fiber_to_thread();
	typedef void(*context_handler)(context_info* info, void* p);
	context_info* make_context(size_t stackSize, context_handler handler, void* p);
	void push_yield(context_info* info);
	void pull_yield(context_info* info);
	void delete_context(context_info* info);
	void decommit_context(context_info* info);
}

#endif
