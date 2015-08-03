#ifndef __SCATTERED_H
#define __SCATTERED_H

#include <functional>
#include <memory>
#include <list>
#include "try_move.h"

using namespace std;


#define NAME_BOND(__NAMEL__, __NAMER__) __NAMEL__ ## __NAMER__

#ifdef _WIN64
#pragma comment(lib, NAME_BOND(__FILE__, "./../asm_lib_x64.lib"))
#else
#pragma comment(lib, NAME_BOND(__FILE__, "./../asm_lib_x86.lib"))
#endif // _WIN64

/*!
@brief 获取当前esp/rsp栈顶寄存器值
*/
void* get_sp();

/*!
@brief 获取bp,sp,ip寄存器值
*/
extern "C" void __fastcall get_bp_sp_ip(void** pbp, void** psp, void** pip);

/*!
@brief 获取CPU计数
*/
extern "C" unsigned long long __fastcall cpu_tick();

/*!
@brief 获取时间字符串
*/
string get_time_string_us();
string get_time_string_ms();
string get_time_string_s();
string get_time_string_s_file();

#ifndef _WIN64

/*!
@brief 64位整数除以32位整数，注意确保商不会超过32bit，否则程序发生致命错误
@param a 被除数
@param b 除数
@param pys 返回余数
@return 商
*/
extern "C" unsigned __fastcall u64div32(unsigned long long a, unsigned b, unsigned* pys = 0);
extern "C" int __fastcall i64div32(long long a, int b, int* pys = 0);

#else // _WIN64


#endif // _WIN64

/*!
@brief 启用高精度时钟
*/
void enable_high_resolution();

/*!
@brief 程序优先权提升为“软实时”
*/
void enable_realtime_priority();

/*!
@brief 设置程序优先级
REALTIME_PRIORITY_CLASS
HIGH_PRIORITY_CLASS
ABOVE_NORMAL_PRIORITY_CLASS
NORMAL_PRIORITY_CLASS
BELOW_NORMAL_PRIORITY_CLASS
IDLE_PRIORITY_CLASS
*/
void set_priority(int p);

/*!
@brief 获取硬件时间戳
*/
long long get_tick_us();
long long get_tick_ms();
int get_tick_s();

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
	string file;
	string module;
	string symbolName;
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

template <typename CL>
struct auto_call 
{
	template <typename TC>
	auto_call(TC&& cl)
		:_cl(TRY_MOVE(cl)) {}

	~auto_call()
	{
		_cl();
	}

	CL _cl;
private:
	auto_call(const auto_call&){};
	void operator =(const auto_call&){}
};

#define BOND_LINE(__P__, __L__) NAME_BOND(__P__, __L__)

//作用域退出时自动调用某个函数
#define AUTO_CALL(__CL__) \
	auto BOND_LINE(__t, __LINE__) = [&]__CL__; \
	auto_call<decltype(BOND_LINE(__t, __LINE__))> BOND_LINE(__cl, __LINE__)(BOND_LINE(__t, __LINE__))


//内存边界对齐
#define MEM_ALIGN(__o, __a) (((__o) + ((__a)-1)) & (((__a)-1) ^ -1))

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
	std::shared_ptr<count> _count;
};

namespace ph
{
	static decltype(std::placeholders::_1) _1;
	static decltype(std::placeholders::_2) _2;
	static decltype(std::placeholders::_3) _3;
	static decltype(std::placeholders::_4) _4;
	static decltype(std::placeholders::_5) _5;
	static decltype(std::placeholders::_6) _6;
	static decltype(std::placeholders::_7) _7;
	static decltype(std::placeholders::_8) _8;
	static decltype(std::placeholders::_9) _9;
}

/*!
@brief 空锁
*/
class null_mutex
{
public:
	void lock() const{};
	void unlock() const{};
};

#ifdef _DEBUG
#define DEBUG_OPERATION(__exp__)	__exp__
#else
#define DEBUG_OPERATION(__exp__)
#endif

#ifdef _DEBUG

#define CHECK_EXCEPTION(__h) try { (__h)(); } catch (...) { assert(false); }
#define CHECK_EXCEPTION1(__h, __p0) try { (__h)(__p0); } catch (...) { assert(false); }
#define CHECK_EXCEPTION2(__h, __p0, __p1) try { (__h)(__p0, __p1); } catch (...) { assert(false); }
#define CHECK_EXCEPTION3(__h, __p0, __p1, __p2) try { (__h)(__p0, __p1, __p2); } catch (...) { assert(false); }

#else

#define CHECK_EXCEPTION(__h) (__h)()
#define CHECK_EXCEPTION1(__h, __p0) (__h)(__p0)
#define CHECK_EXCEPTION2(__h, __p0, __p1) (__h)(__p0, __p1)
#define CHECK_EXCEPTION3(__h, __p0, __p1, __p2) (__h)(__p0, __p1, __p2)

#endif //end _DEBUG
#define REF_STRUCT_NAME(__NAME__) __ref_##__NAME__

//用户内嵌lambda的外部变量引用捕获，可以减小直接"&捕获"sizeof(lambda)的大小，提高调度效率
#define LAMBDA_REF1(__NAME__, __P0__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)(decltype(__P0__)& p0)\
	:__P0__(p0){}\
	decltype(__P0__)& __P0__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s) : __P0__(s.__P0__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__);

#define LAMBDA_REF2(__NAME__, __P0__, __P1__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)(decltype(__P0__)& p0, decltype(__P1__)& p1)\
	:__P0__(p0), __P1__(p1){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s) : __P0__(s.__P0__), __P1__(s.__P1__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__);

#define LAMBDA_REF3(__NAME__, __P0__, __P1__, __P2__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)(decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2)\
	:__P0__(p0), __P1__(p1), __P2__(p2) {}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s) : __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__, __P2__);

#define LAMBDA_REF4(__NAME__, __P0__, __P1__, __P2__, __P3__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)(decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3)\
	:__P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3) {}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__, __P2__, __P3__);

#define LAMBDA_REF5(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)\
	(decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4)\
	:__P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4) {}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__, __P2__, __P3__, __P4__);

#define LAMBDA_REF6(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)\
	(decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, decltype(__P5__)& p5)\
	:__P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), __P5__(s.__P5__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__, __P2__, __P3__, __P4__, __P5__);

#define LAMBDA_REF7(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)\
	(decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, \
	decltype(__P5__)& p5, decltype(__P6__)& p6)\
	:__P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5), __P6__(p6){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
	decltype(__P6__)& __P6__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), __P5__(s.__P5__), __P6__(s.__P6__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__);

#define LAMBDA_REF8(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)\
	(decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, \
	decltype(__P5__)& p5, decltype(__P6__)& p6, decltype(__P7__)& p7)\
	:__P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5), __P6__(p6), __P7__(p7){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
	decltype(__P6__)& __P6__; \
	decltype(__P7__)& __P7__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), __P5__(s.__P5__), __P6__(s.__P6__), __P7__(s.__P7__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__);

#define LAMBDA_REF9(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)\
	(decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, \
	decltype(__P5__)& p5, decltype(__P6__)& p6, decltype(__P7__)& p7, decltype(__P8__)& p8)\
	:__P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5), __P6__(p6), __P7__(p7), __P8__(p8){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
	decltype(__P6__)& __P6__; \
	decltype(__P7__)& __P7__; \
	decltype(__P8__)& __P8__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), \
	__P5__(s.__P5__), __P6__(s.__P6__), __P7__(s.__P7__), __P8__(s.__P8__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__);

#define LAMBDA_REF10(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	REF_STRUCT_NAME(__NAME__)\
	(decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, \
	decltype(__P5__)& p5, decltype(__P6__)& p6, decltype(__P7__)& p7, decltype(__P8__)& p8, decltype(__P9__)& p9)\
	:__P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5), __P6__(p6), __P7__(p7), __P8__(p8), __P9__(p9){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
	decltype(__P6__)& __P6__; \
	decltype(__P7__)& __P7__; \
	decltype(__P8__)& __P8__; \
	decltype(__P9__)& __P9__; \
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), \
	__P5__(s.__P5__), __P6__(s.__P6__), __P7__(s.__P7__), __P8__(s.__P8__), __P9__(s.__P9__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
} __NAME__(__P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__);
//////////////////////////////////////////////////////////////////////////

//用户内嵌lambda的外部变量引用捕获（外带当前this捕获，通过->访问），可以减小直接"&捕获"sizeof(lambda)的大小，提高调度效率
#define LAMBDA_THIS_REF1(__NAME__, __P0__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)(this_type ts, decltype(__P0__)& p0)\
	:__this(ts), __P0__(p0){}\
	decltype(__P0__)& __P0__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__);

#define LAMBDA_THIS_REF2(__NAME__, __P0__, __P1__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1)\
	:__this(ts), __P0__(p0), __P1__(p1){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__, __P1__);

#define LAMBDA_THIS_REF3(__NAME__, __P0__, __P1__, __P2__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2)\
	:__this(ts), __P0__(p0), __P1__(p1), __P2__(p2) {}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__, __P1__, __P2__);

#define LAMBDA_THIS_REF4(__NAME__, __P0__, __P1__, __P2__, __P3__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3)\
	:__this(ts), __P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3) {}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__, __P1__, __P2__, __P3__);

#define LAMBDA_THIS_REF5(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)\
	(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4)\
	:__this(ts), __P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4) {}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__, __P1__, __P2__, __P3__, __P4__);

#define LAMBDA_THIS_REF6(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)\
	(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, decltype(__P5__)& p5)\
	:__this(ts), __P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), __P5__(s.__P5__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__);

#define LAMBDA_THIS_REF7(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)\
	(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, decltype(__P5__)& p5, decltype(__P6__)& p6)\
	:__this(ts), __P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5), __P6__(p6){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
	decltype(__P6__)& __P6__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), __P5__(s.__P5__), __P6__(s.__P6__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__);

#define LAMBDA_THIS_REF8(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)\
	(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, \
	decltype(__P5__)& p5, decltype(__P6__)& p6, decltype(__P7__)& p7)\
	:__this(ts), __P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5), __P6__(p6), __P7__(p7){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
	decltype(__P6__)& __P6__; \
	decltype(__P7__)& __P7__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), \
	__P5__(s.__P5__), __P6__(s.__P6__), __P7__(s.__P7__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__);


#define LAMBDA_THIS_REF9(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)\
	(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4, \
	decltype(__P5__)& p5, decltype(__P6__)& p6, decltype(__P7__)& p7, decltype(__P8__)& p8)\
	:__this(ts), __P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5), __P6__(p6), __P7__(p7), __P8__(p8){}\
	decltype(__P0__)& __P0__; \
	decltype(__P1__)& __P1__; \
	decltype(__P2__)& __P2__; \
	decltype(__P3__)& __P3__; \
	decltype(__P4__)& __P4__; \
	decltype(__P5__)& __P5__; \
	decltype(__P6__)& __P6__; \
	decltype(__P7__)& __P7__; \
	decltype(__P8__)& __P8__; \
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
private:\
	REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
	: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__), \
	__P5__(s.__P5__), __P6__(s.__P6__), __P7__(s.__P7__), __P8__(s.__P8__){}\
	void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
	this_type __this; \
} __NAME__(this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__);

#define LAMBDA_THIS_REF10(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__)\
struct REF_STRUCT_NAME(__NAME__)\
{\
	typedef decltype(this) this_type; \
	REF_STRUCT_NAME(__NAME__)\
	(this_type ts, decltype(__P0__)& p0, decltype(__P1__)& p1, decltype(__P2__)& p2, decltype(__P3__)& p3, decltype(__P4__)& p4,\
decltype(__P5__)& p5, decltype(__P6__)& p6, decltype(__P7__)& p7, decltype(__P8__)& p8, decltype(__P9__)& p9)\
:__this(ts), __P0__(p0), __P1__(p1), __P2__(p2), __P3__(p3), __P4__(p4), __P5__(p5), __P6__(p6), __P7__(p7), __P8__(p8), __P9__(p9){}\
decltype(__P0__)& __P0__; \
decltype(__P1__)& __P1__; \
decltype(__P2__)& __P2__; \
decltype(__P3__)& __P3__; \
decltype(__P4__)& __P4__; \
decltype(__P5__)& __P5__; \
decltype(__P6__)& __P6__; \
decltype(__P7__)& __P7__; \
decltype(__P8__)& __P8__; \
decltype(__P9__)& __P9__; \
this_type get(){ return __this; }\
this_type operator->(){ return __this; }\
private:\
		REF_STRUCT_NAME(__NAME__)(const REF_STRUCT_NAME(__NAME__)& s)\
		: __this(s.__this), __P0__(s.__P0__), __P1__(s.__P1__), __P2__(s.__P2__), __P3__(s.__P3__), __P4__(s.__P4__),\
		__P5__(s.__P5__), __P6__(s.__P6__), __P7__(s.__P7__), __P8__(s.__P8__), __P9__(s.__P9__){}\
		void operator=(const REF_STRUCT_NAME(__NAME__)&){}\
private:\
		this_type __this; \
} __NAME__(this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__);

#define BEGIN_TRY_ {\
bool __catched = false; \
try {

#define CATCH_FOR(__exp__) }\
catch (__exp__&)\
{\
	__catched = true; \
}\
if (__catched){

#define END_TRY_ }}

#define RUN_IN_STRAND(__host__, __strand__, __exp__) __host__->send(__strand__, [&] {__exp__;})

#define begin_RUN_IN_STRAND(__host__, __strand__) __host__->send(__strand__, [&] {

#define end_RUN_IN_STRAND() })

#define begin_ACTOR_RUN_IN_STRAND(__host__, __strand__) {\
	my_actor* ___host = __host__; \
	actor_handle ___actor = my_actor::create(__strand__, [&](my_actor* __host__) {

#define end_ACTOR_RUN_IN_STRAND() });\
	___actor->notify_run();\
	___host->actor_wait_quit(___actor); \
}

#define RUN_IN_TRHEAD_STACK(__host__, __exp__) __host__->run_in_thread_stack([&] {__exp__;})

#define begin_RUN_IN_TRHEAD_STACK(__host__)  __host__->run_in_thread_stack([&] {

#define end_RUN_IN_TRHEAD_STACK() })

#endif