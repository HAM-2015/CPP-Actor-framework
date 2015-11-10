#ifndef __CONTEXT_YIELD_H
#define __CONTEXT_YIELD_H

#ifdef DISABLE_BOOST_CORO
#include "scattered.h"

#ifdef _MSC_VER
#ifdef _WIN64
#pragma comment(lib, NAME_BOND(__FILE__, "./../context_yield64.lib"))
#else
#pragma comment(lib, NAME_BOND(__FILE__, "./../context_yield.lib"))
#endif // _WIN64
#endif

namespace context_yield
{
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
		size_t obj_space_size();
		coro_pull_interface* make_coro(coro_push_handler ch, void* stackTop, size_t size, void* param = 0);
		coro_pull_interface* make_coro2(void* space, coro_push_handler ch, void* stackTop, size_t size, void* param = 0);
	}
}
#endif

#endif
