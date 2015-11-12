#ifndef __SCATTERED_H
#define __SCATTERED_H

#include <algorithm>
#include <functional>
#include <memory>
#include <list>
#include "try_move.h"

using namespace std;

#define NAME_BOND(__NAMEL__, __NAMER__) __NAMEL__ ## __NAMER__

#ifdef _MSC_VER
#ifdef _WIN64
#pragma comment(lib, NAME_BOND(__FILE__, "./../asm_lib_x64.lib"))
#else
#pragma comment(lib, NAME_BOND(__FILE__, "./../asm_lib_x86.lib"))
#endif // _WIN64
#endif

#ifdef _MSC_VER
#define FRIEND_SHARED_PTR(__T__)\
	friend std::shared_ptr<__T__>;\
	friend std::_Ref_count<__T__>;
#elif __GNUG__
#define FRIEND_SHARED_PTR(__T__)\
	friend std::shared_ptr<__T__>; \
	friend std::__shared_count<__gnu_cxx::__default_lock_policy>;\
	friend std::_Sp_counted_ptr<__T__*, __gnu_cxx::__default_lock_policy>;
#endif

template <typename CL>
struct out_of_scope
{
	template <typename TC>
	out_of_scope(TC&& cl)
		:_cl(TRY_MOVE(cl)) {}

	~out_of_scope()
	{
		_cl();
	}

	CL _cl;
private:
	out_of_scope(const out_of_scope&){};
	void operator =(const out_of_scope&){}
};

#define BOND_LINE(__P__, __L__) NAME_BOND(__P__, __L__)

//作用域退出时自动调用lambda
#define OUT_OF_SCOPE(__CL__) \
	auto BOND_LINE(__t, __LINE__) = [&]__CL__; \
	out_of_scope<decltype(BOND_LINE(__t, __LINE__))> BOND_LINE(__cl, __LINE__)(BOND_LINE(__t, __LINE__))

#if (_DEBUG || DEBUG)
#define DEBUG_OPERATION(__exp__)	__exp__
#else
#define DEBUG_OPERATION(__exp__)
#endif

#if (_DEBUG || DEBUG)

#define CHECK_EXCEPTION(__h) try { (__h)(); } catch (...) { assert(false); }
#define CHECK_EXCEPTION1(__h, __p0) try { (__h)(__p0); } catch (...) { assert(false); }
#define CHECK_EXCEPTION2(__h, __p0, __p1) try { (__h)(__p0, __p1); } catch (...) { assert(false); }
#define CHECK_EXCEPTION3(__h, __p0, __p1, __p2) try { (__h)(__p0, __p1, __p2); } catch (...) { assert(false); }

#define BEGIN_CHECK_EXCEPTION try {
#define END_CHECK_EXCEPTION } catch (...) { assert(false); }

#else

#define CHECK_EXCEPTION(__h) (__h)()
#define CHECK_EXCEPTION1(__h, __p0) (__h)(__p0)
#define CHECK_EXCEPTION2(__h, __p0, __p1) (__h)(__p0, __p1)
#define CHECK_EXCEPTION3(__h, __p0, __p1, __p2) (__h)(__p0, __p1, __p2)

#define BEGIN_CHECK_EXCEPTION
#define END_CHECK_EXCEPTION

#endif

//内存边界对齐
#define MEM_ALIGN(__o, __a) (((__o) + ((__a)-1)) & (((__a)-1) ^ -1))

//////////////////////////////////////////////////////////////////////////

#define BEGIN_TRY_ {\
	bool __catched = false; \
	try {

#define CATCH_FOR(__exp__) }\
	catch (__exp__&) { __catched = true; }\
	DEBUG_OPERATION(catch (...) { assert(false); })\
if (__catched) {

#define END_TRY_ }}

#define RUN_IN_STRAND(__host__, __strand__, __exp__) __host__->send(__strand__, [&] {__exp__;})

#define begin_RUN_IN_STRAND(__host__, __strand__) __host__->send(__strand__, [&] {

#define end_RUN_IN_STRAND() })

#define begin_ACTOR_RUN_IN_STRAND(__host__, __strand__) {\
	my_actor* ___host = __host__; \
	actor_handle ___actor = my_actor::create(__strand__, [&](my_actor* __host__) {

#define end_ACTOR_RUN_IN_STRAND() });\
	___actor->notify_run(); \
	___host->actor_wait_quit(___actor); \
}

#define RUN_IN_TRHEAD_STACK(__host__, __exp__) {\
my_actor::quit_guard qg(__host__); \
__host__->run_in_thread_stack([&] {__exp__; });}

#define begin_RUN_IN_TRHEAD_STACK(__host__){\
	my_actor::quit_guard qg(__host__); \
	__host__->run_in_thread_stack([&] {

#define end_RUN_IN_TRHEAD_STACK() });}


/*!
@brief 这个类在测试消息传递时使用
*/
struct move_test
{
	struct count 
	{
		int _id;
		size_t _copyCount;
		size_t _moveCount;
		std::function<void(std::shared_ptr<count>)> _cb;
	};

	explicit move_test(int id);
	move_test();
	~move_test();
	move_test(int id, const std::function<void(std::shared_ptr<count>)>& cb);
	move_test(const move_test& s);
	move_test(move_test&& s);
	void operator=(const move_test& s);
	void operator=(move_test&& s);
	friend std::ostream& operator <<(std::ostream& out, const move_test& s);
	std::shared_ptr<count> _count;
	size_t _generation;
};

/*!
@brief 启用高精度时钟
*/
void enable_high_resolution();

std::string get_time_string_us();
std::string get_time_string_ms();
std::string get_time_string_s();
std::string get_time_string_s_file();

long long get_tick_us();
long long get_tick_ms();
int get_tick_s();

#ifdef _MSC_VER
void* get_sp();
extern "C" void __fastcall get_bp_sp_ip(void** pbp, void** psp, void** pip);
extern "C" unsigned long long __fastcall cpu_tick();
#elif __GNUG__
void get_bp_sp_ip(void** pbp, void** psp, void** pip);
void* get_sp();
unsigned long long cpu_tick();
#endif

#ifdef CHECK_ACTOR_STACK
struct stack_line_info
{
	stack_line_info(){}

	stack_line_info(stack_line_info&& s)
		:line(s.line),
		file(std::move(s.file)),
		module(std::move(s.module)),
		symbolName(std::move(s.symbolName)) {}

	int line;
	std::string file;
	std::string module;
	std::string symbolName;
};

//记录当前Actor入口信息
#define ACTOR_POSITION(__host__) \
{\
	__host__->_checkStackFree = true; \
	stack_line_info __line; \
	__line.line = __LINE__; \
	__line.file = __FILE__; \
	__line.module = "actor position"; \
	__host__->_createStack->push_front(__line); \
}

//记录当前Actor入口信息
#define SELF_POSITION ACTOR_POSITION(self)

/*!
@brief 获取当前调用堆栈信息
@param maxDepth 获取当前堆栈向下最高层次，最大32层
*/
list<stack_line_info> get_stack_list(size_t maxDepth = 32, size_t offset = 0, bool module = false, bool symbolName = false);

/*!
@brief 堆栈溢出弹出消息
*/
void stack_overflow_format(int size, std::shared_ptr<list<stack_line_info>> createStack);

#else //CHECK_ACTOR_STACK

//记录当前Actor入口信息
#define ACTOR_POSITION(__host__)
#define SELF_POSITION

#endif //CHECK_ACTOR_STACK

/*!
@brief 清空std::function
*/
template <typename F>
inline void clear_function(F& f)
{
	f = F();
}

#ifdef _MSC_VER
#define snPrintf sprintf_s
#elif __GNUG__
#define snPrintf snprintf
#endif

#endif