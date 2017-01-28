#ifndef __GENERATOR_H
#define __GENERATOR_H

#include "msg_queue.h"
#include "actor_timer.h"
#include "async_timer.h"

//在generator函数体内，获取当前generator对象
#define co_self __coSelf
//作为generator函数体首个参数
#define co_generator generator& co_self
//在generator上下文初始化中(co_end_context_init)，设置当前不捕获外部变量
#define co_context_no_capture __noContextCapture
struct __co_context_no_capture{};

#if (_DEBUG || DEBUG)
#define _co_for_check_break_sign bool __coForCheckBreak = false
#define _co_for_break {__coForCheckBreak = true; break;}
#define _co_for \
	for (__forYieldSwitch = false, __coForCheckBreak = false;; __forYieldSwitch = true)\
	if (__forYieldSwitch) { assert(__coForCheckBreak); break; }\
	else for
#else
#define _co_for_check_break_sign
#define _co_for_break break
#define _co_for for
#endif

#define _co_counter (__COUNTER__ - __coBeginCount)

//开始定义generator函数体上下文
#define co_begin_context \
	enum { __coBeginCount = __COUNTER__+2 };\
	static_assert(__COUNTER__+1 == __COUNTER__, ""); __co_context_no_capture const co_context_no_capture = __co_context_no_capture();\
	struct co_context_tag {

#define _co_end_context(__ctx__) \
	DEBUG_OPERATION(co_self.__inside = true);}\
	_co_for_check_break_sign;\
	struct co_context_tag* const __pCoContext = static_cast<struct co_context_tag*>(co_self.__ctx);\
	struct co_context_tag& __coContext = *__pCoContext;\
	struct co_context_tag& __ctx__ = *__pCoContext;\
	size_t __coSwitchTempVal = 0;\
	bool __coSwitchFirstLoopSign = false;\
	bool __coSwitchDefaultSign = false;\
	bool __coSwitchPreSign = false;\
	bool __coSwitchSign = false;\
	bool __selectCaseDoSign = false;\
	bool __selectCaseTyiedIo = false;\
	bool __selectCaseTyiedIoFailed = false;\
	bool __forYieldSwitch = false;\
	bool __isBestCall = false;\
	int __selectCaseStep = 0;\
	int __selectStep = 0;\
	int __coNext = 0;

#define _co_end_no_context \
	if (!co_self.__ctx){co_self.__ctx = (void*)-1;\
	DEBUG_OPERATION(co_self.__inside = true);}\
	_co_for_check_break_sign;\
	size_t __coSwitchTempVal = 0;\
	bool __coSwitchFirstLoopSign = false;\
	bool __coSwitchDefaultSign = false;\
	bool __coSwitchPreSign = false;\
	bool __coSwitchSign = false;\
	bool __selectCaseDoSign = false;\
	bool __selectCaseTyiedIo = false;\
	bool __selectCaseTyiedIoFailed = false;\
	bool __forYieldSwitch = false;\
	bool __isBestCall = false;\
	int __selectCaseStep = 0;\
	int __selectStep = 0;\
	int __coNext = 0;\
	_co_stop_no_ctx(); if(0){

#define _co_stop() \
	auto __stop = [&co_self]{\
	DEBUG_OPERATION(co_self.__inside = false);\
	struct co_context_tag* const pCtx = static_cast<struct co_context_tag*>(co_self.__ctx);\
	if((void*)-1!=(void*)pCtx){delete pCtx;}\
	co_self.__ctx = NULL;}

#define _co_stop_dealloc(__dealloc__) \
	auto __stop = [&]{\
	DEBUG_OPERATION(co_self.__inside = false);\
	struct co_context_tag* const pCtx = static_cast<struct co_context_tag*>(co_self.__ctx);\
	if((void*)-1!=(void*)pCtx){pCtx->~co_context_tag(); (__dealloc__)(pCtx);}\
	co_self.__ctx = NULL;}

#define _co_stop_no_ctx() \
	auto __stop = [&co_self]{\
	DEBUG_OPERATION(co_self.__inside = false);\
	assert((void*)-1==(void*)co_self.__ctx);\
	co_self.__ctx = NULL;}

//结束generator函数体上下文定义
#define co_end_context(__ctx__) };\
	if (!co_self.__ctx){co_self.__ctx = -1==co_self.__coNext ? (void*)-1 : new co_context_tag();\
	_co_end_context(__ctx__); _co_stop(); if(0){

#define _cop(__p__) decltype(__p__)& __p__
#define _co_capture0() co_context_tag()
#define _co_capture1(p1) co_context_tag(_cop(p1))
#define _co_capture2(p1,p2) co_context_tag(_cop(p1),_cop(p2))
#define _co_capture3(p1,p2,p3) co_context_tag(_cop(p1),_cop(p2),_cop(p3))
#define _co_capture4(p1,p2,p3,p4) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4))
#define _co_capture5(p1,p2,p3,p4,p5) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5))
#define _co_capture6(p1,p2,p3,p4,p5,p6) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6))
#define _co_capture7(p1,p2,p3,p4,p5,p6,p7) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7))
#define _co_capture8(p1,p2,p3,p4,p5,p6,p7,p8) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8))
#define _co_capture9(p1,p2,p3,p4,p5,p6,p7,p8,p9) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8),_cop(p9))
#define _co_capture10(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8),_cop(p9),_cop(p10))
#define _co_capture(...) _BOND_LR__(_co_capture, MPL_ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__)

//结束generator函数体上下文定义，带内部变量初始化
#define co_end_context_init(__ctx__, __capture__, ...) _co_capture __capture__:__VA_ARGS__{}};\
	if (!co_self.__ctx){co_self.__ctx = -1==co_self.__coNext ? (void*)-1 : new co_context_tag __capture__;\
	_co_end_context(__ctx__); _co_stop(); if(0){

//在generator结束时，做最后状态清理，可以不用
#define co_context_destroy ~co_context_tag()
//没有上下文状态信息
#define co_no_context co_begin_context}; _co_end_no_context
//没有上下文状态，引用co_ref_xx为隐藏参数
#define co_end_ref_context(...) } __coContext = {__VA_ARGS__}; _co_end_no_context

#define co_ref_state co_async_state& __coState
#define co_ref_id gen_id& __coId
#define co_ref_timer overlap_timer::timer_handle& __coTimerHandle
#define co_ref_select co_select_sign& __selectSign

//
#define co_context_space_size sizeof(co_context_tag)
//
#define co_end_context_alloc(__alloc__, __dealloc__, __ctx__) };\
	if (!co_self.__ctx){co_self.__ctx = -1==co_self.__coNext ? (void*)-1 : new(__alloc__)co_context_tag();\
	_co_end_context(__ctx__); _co_stop_dealloc(__dealloc__); if(0){

#define co_end_context_alloc_init(__alloc__, __dealloc__, __ctx__, __capture__, ...) _co_capture __capture__:__VA_ARGS__{}};\
	if (!co_self.__ctx){co_self.__ctx = -1==co_self.__coNext ? (void*)-1 : new(__alloc__)co_context_tag __capture__;\
	_co_end_context(__ctx__); _co_stop_dealloc(__dealloc__); if(0){

#define co_end_context_stack(__lifo_alloc__, __ctx__) \
	co_end_context_alloc(__lifo_alloc__.allocate(co_context_space_size), [&](void* p){__lifo_alloc__.deallocate(p, co_context_space_size); }, __ctx__)

#define co_end_context_stack_init(__lifo_alloc__, __ctx__, __capture__, ...) \
	co_end_context_alloc_init(__lifo_alloc__.allocate(co_context_space_size), [&](void* p){__lifo_alloc__.deallocate(p, co_context_space_size); }, __ctx__, __capture__, __VA_ARGS__)

#define co_check_stop do{if(-1==co_self.__coNext) co_stop;}while(0)

//开始generator的代码区域
#define co_begin }\
	if (!co_self.__coNext) {co_self.__coNext = (_co_counter+1)/2;}else co_check_stop;\
	__coNext=co_self.__coNext; co_self.__coNext=0;\
	switch(co_self.__coNextEx ? co_self.__coNextEx : __coNext) { case _co_counter/2:;

//结束generator的代码区域
#define co_end break;default:assert(false);} co_stop;

#define _co_yield do{\
	assert(co_self.__inside);\
	assert(!co_self.__awaitSign && !co_self.__sharedAwaitSign);\
	co_check_stop;\
	DEBUG_OPERATION(co_self.__inside = false);\
	co_self.__coNext = (_co_counter+1)/2;\
	return; case _co_counter/2:;\
	}while (0)

//generator的yield原语
#define co_yield \
	assert(co_self.__inside);\
	_co_for(__forYieldSwitch = false;;__forYieldSwitch = true)\
	if (__forYieldSwitch) {_co_yield; _co_for_break;}\
	else

//不带参数的异步回调方法
#define co_async CoAsync_(std::move(co_async_this))
//不带参数的可共享异步回调方法
#define co_shared_async CoShardAsync_(co_shared_this, co_async_sign)
//不带参数的不加线程判断的异步回调方法
#define co_anext CoAnext_(std::move(co_shared_this))

//带参数的异步回调方法
#define co_async_result(...) _co_async_result(std::move(co_async_this), __VA_ARGS__)
#define co_asio_result(...) _co_asio_result(std::move(co_async_this), __VA_ARGS__)
#define co_anext_result(...) _co_anext_result(std::move(co_shared_this), __VA_ARGS__)
#define co_async_result_(...) _co_async_same_result(std::move(co_async_this), __VA_ARGS__)
#define co_anext_result_(...) _co_anext_same_result(std::move(co_shared_this), __VA_ARGS__)
#define co_async_safe_result(...) _co_async_safe_result(std::move(co_async_this), __VA_ARGS__)
#define co_anext_safe_result(...) _co_anext_safe_result(std::move(co_shared_this), __VA_ARGS__)
#define co_async_safe_result_(...) _co_async_same_safe_result(std::move(co_async_this), __VA_ARGS__)
#define co_anext_safe_result_(...) _co_anext_same_safe_result(std::move(co_shared_this), __VA_ARGS__)
//带参数的可共享异步回调方法
#define co_shared_async_result(...) _co_shared_async_result(co_shared_this, co_async_sign, __VA_ARGS__)
#define co_shared_async_result_(...) _co_shared_async_same_result(co_shared_this, co_async_sign, __VA_ARGS__)
#define co_shared_async_safe_result(...) _co_shared_async_safe_result(co_shared_this, co_async_sign, __VA_ARGS__)
#define co_shared_async_safe_result_(...) _co_shared_async_same_safe_result(co_shared_this, co_async_sign, __VA_ARGS__)
//忽略一个参数
#define co_ignore (generator::__anyAccept)
//忽略所有异步参数
#define co_async_ignore CoAsyncIgnoreResult_(std::move(co_async_this))
#define co_shared_async_ignore CoShardAsyncIgnoreResult_(co_shared_this, co_async_sign)
#define co_anext_ignore CoAnextIgnoreResult_(std::move(co_shared_this))
#define co_asio_ignore CoAsioIgnoreResult_(std::move(co_async_this))

//挂起generator，等待调度器下次触发
#define co_tick do{\
	assert(co_self.__inside);\
	co_strand->next_tick(std::bind([](generator_handle& host){\
	if(!host->__ctx)return; host->_revert_this(host)->_next(); }, std::move(co_shared_this))); _co_yield;}while (0)

//挂起generator，等待调度器下次IO后触发
#define co_io_tick do{\
	assert(co_self.__inside);\
	co_strand->post(std::bind([](generator_handle& host){\
	if(!host->__ctx)return; host->_revert_this(host)->_next(); }, std::move(co_shared_this))); _co_yield;}while (0)

//检测上次异步操作有没有发生切换，没有就切换一次
#define co_try_tick do{if(!co_last_yield){co_tick;}}while (0)
#define co_try_io_tick do{if(!co_last_yield){co_io_tick;}}while (0)

//检测上次异步操作有没有发生切换
#define co_last_yield ((const bool&)co_self.__yieldSign)

#define _co_await do{\
	assert(co_self.__inside);\
	assert(co_self.__awaitSign || co_self.__sharedAwaitSign);\
	DEBUG_OPERATION(co_self.__awaitSign = co_self.__sharedAwaitSign = false);\
	if (co_self.__asyncSign) {co_self.__asyncSign = false;}\
	else {co_self.__asyncSign = true; _co_yield;}\
	}while (0)

#define _co_timed_await(__ms__, __handler__) do{\
	assert(co_self.__inside);\
	co_timeout(__ms__, co_timer, [&]__handler__); _co_await; \
	co_cancel_timer(co_timer); \
	}while (0)

#define _co_timed_await2(__ms__) do{\
	assert(co_self.__inside);\
	co_timeout(__ms__, co_timer, wrap_bind_(std::bind([&](generator_handle& gen, shared_bool& sign){\
	co_last_state = co_async_state::co_async_overtime;\
	co_shared_async_next(gen, sign);}, co_shared_this, co_async_sign)));\
	_co_await; co_cancel_timer(co_timer); \
	}while (0)

//generator await原语，与co_async之类使用
#define co_await \
	assert(co_self.__inside);\
	_co_for(__forYieldSwitch = false;;__forYieldSwitch = true)\
	if (__forYieldSwitch) {_co_await; _co_for_break;}\
	else

#define co_timed_await(__ms__, __handler__) \
	assert(co_self.__inside);\
	_co_for(__forYieldSwitch = false;;__forYieldSwitch = true)\
	if (__forYieldSwitch) {_co_timed_await(__ms__, __handler__); _co_for_break;}\
	else

#define co_timed_await2(__ms__) \
	assert(co_self.__inside);\
	co_last_state = co_async_state::co_async_ok;\
	_co_for(__forYieldSwitch = false;;__forYieldSwitch = true)\
	if (__forYieldSwitch) {_co_timed_await2(__ms__); _co_for_break;}\
	else

#define co_next_(__host__) do{\
	__host__->_revert_this(__host__)->_co_next();\
	}while (0)

#define co_next(__host__) do{\
	generator_handle __host = __host__;\
	__host->_revert_this(__host)->_co_next();\
	}while (0)

#define co_tick_next_(__host__) do{\
	__host__->_revert_this(__host__)->_co_tick_next();\
	}while (0)

#define co_tick_next(__host__) do{\
	generator_handle __host = __host__;\
	__host->_revert_this(__host)->_co_tick_next();\
	}while (0)

#define co_async_next_(__host__) do{\
	__host__->_revert_this(__host__)->_co_async_next();\
	}while (0)

#define co_async_next(__host__) do{\
	generator_handle __host = __host__;\
	__host->_revert_this(__host)->_co_async_next();\
	}while (0)

#define co_shared_async_next(__host__, __sign__) do{\
	__host__->_co_shared_async_next(__sign__);\
	}while (0)

//递归调用另一个generator，直到执行完毕后接着下一行
#define co_call_of \
	assert(co_self.__inside);\
	co_check_stop;\
	_co_for(__forYieldSwitch = false;;__forYieldSwitch = true)\
	if (__forYieldSwitch) {co_self.__coNextEx=0; if (-1 != co_self.__coNext){co_self.__coNext = -2;}\
	return; case (_co_counter+1)/2:; _co_for_break;}\
	else CoCall_(co_self, _co_counter/2)-

#define co_call(...) co_call_of _co_call_bind(__VA_ARGS__)

//在指定strand中，递归调用另一个generator，直到执行完毕后接着下一行
#define co_st_call_of(__strand__) \
	assert(co_self.__inside);\
	co_check_stop; co_lock_stop;\
	_co_for(__forYieldSwitch = false;;__forYieldSwitch = true)\
	if (__forYieldSwitch) {_co_await; co_unlock_stop; _co_for_break;}\
	else CoStCall(__strand__, co_self)-

#define co_st_call(__strand__, ...) co_st_call_of(__strand__) _co_call_bind(__VA_ARGS__)

//判断strand属性，决定是用co_call_of还是co_st_call_of
#define co_best_call_of(__strand__) \
	assert(co_self.__inside);\
	co_check_stop;\
	_co_for(__forYieldSwitch = false;;__forYieldSwitch = true)\
	if (__forYieldSwitch) {if(!__isBestCall){co_lock_stop; _co_await; co_unlock_stop;}\
	else{co_self.__coNextEx=0; if (-1 != co_self.__coNext){co_self.__coNext = -2;}\
	return; case (_co_counter+1)/2:;}_co_for_break;}\
	else CoBestCall(__strand__, co_self, __isBestCall, _co_counter/2)-

#define co_best_call(__strand__, ...) co_best_call_of(__strand__) _co_call_bind(__VA_ARGS__)

//启动一个临时作用域
#define co_scope co_call_of [&](co_generator)->void

//sleep等待
#define co_sleep(__ms__) do{co_self._co_sleep(__ms__); _co_yield;}while (0)
#define co_usleep(__us__) do{co_self._co_usleep(__us__); _co_yield;}while (0)
#define co_sleep_(__ms__) do{co_timeout(__ms__, co_timer, co_anext); _co_yield;}while (0)
#define co_usleep_(__us__) do{co_utimeout(__us__, co_timer, co_anext); _co_yield;}while (0)

//各种异步延时
#define co_dead_sleep(__ms__) do{co_self._co_dead_sleep(__ms__); _co_yield;}while (0)
#define co_dead_usleep(__us__) do{co_self._co_dead_usleep(__us__); _co_yield;}while (0)
#define co_timeout_of(__ms__, __th__) CoTimeout_(__ms__, co_over_timer, __th__)-
#define co_utimeout_of(__us__, __th__) CouTimeout_(__us__, co_over_timer, __th__)-
#define co_deadline_of(__us__, __th__) CoDeadline_(__us__, co_over_timer, __th__)-
#define co_interval_of(__ms__, .../*__th__, [immed=false]*/) CoInterval_(__ms__, co_over_timer, __VA_ARGS__)-
#define co_uinterval_of(__us__, .../*__th__, [immed=false]*/) CouInterval_(__us__, co_over_timer, __VA_ARGS__)-
#define co_timeout co_over_timer->timeout
#define co_utimeout co_over_timer->utimeout
#define co_deadline co_over_timer->deadline
#define co_interval co_over_timer->interval
#define co_uinterval co_over_timer->uinterval
//取消延时
#define co_cancel_timer co_over_timer->cancel
//提前触发延时
#define co_advance_timer co_over_timer->advance
//重新开始计时
#define co_restart_timer co_over_timer->restart
//当前overlap_timer定时器
#define co_over_timer co_strand->over_timer()

//创建一个generator，立即运行
#define co_go(...) CoGo_(__VA_ARGS__)-
//创建一个generator，但不立即运行
#define co_create(...) CoCreate_(__VA_ARGS__)-
//结束当前generator的运行
#define co_stop do{__stop(); return;} while(0)
//锁定外部generator的stop操作
#define co_lock_stop do{\
	assert(co_self.__inside);\
	co_check_stop;\
	assert(co_self.__lockStop<255);\
	co_self.__lockStop++;\
	}while (0)
//解锁外部generator的stop操作
#define co_unlock_stop do{\
	assert(co_self.__inside);\
	assert(co_self.__lockStop);\
	assert(-1!=co_self.__coNext);\
	if (0==--co_self.__lockStop && co_self.__readyQuit) co_stop;\
	}while (0)
//当前generator调度器
#define co_strand co_self.self_strand()
#define co_ios co_strand->get_io_engine()
//单线回调时与co_yield配合使用，多线调度时与co_async_sign，co_await配合使用
#define co_shared_this co_self.shared_this()
//与co_await回调时配合使用
#define co_async_this co_self.async_this()
//多线回调时使用标记，与co_shared_this配合
#define co_async_sign co_self.shared_async_sign()

//发送一个任务到另一个strand中执行，执行完成后接着下一行
#define co_begin_send(__strand__) {if (co_self._co_send(__strand__, [&]{
#define co_end_send }))_co_yield;}
//发送一个任务到另一个strand中执行，执行完成后接着下一行
#define co_begin_async_send(__strand__) {co_self._co_async_send(__strand__, [&]{
#define co_end_async_send });_co_yield;}

//提供该宏间接实现动态case的switch效果
#define co_begin_switch(__val__) for(__coSwitchPreSign=false,__coSwitchDefaultSign=false,__coSwitchFirstLoopSign=true,__coSwitchTempVal=(size_t)(__val__);\
	__coSwitchFirstLoopSign || (!__coSwitchPreSign && __coSwitchDefaultSign);__coSwitchFirstLoopSign=false){if(0){
#define co_switch_case_(__val__) __coSwitchPreSign=true;}if (__coSwitchPreSign || __coSwitchTempVal==(size_t)(__val__)){
#define co_switch_case(__val__) __coSwitchPreSign=true;}if (__coSwitchPreSign || (__coSwitchFirstLoopSign&&__coSwitchTempVal==(size_t)(__val__))){
#define co_switch_default __coSwitchPreSign=true;}if(!__coSwitchDefaultSign && !__coSwitchPreSign){__coSwitchDefaultSign=true;}else{
#define co_end_switch __coSwitchPreSign=true;}}

#define _switch_mark (((unsigned long long)1) << 32)
//generator无法reenter到switch-case内，替换原有switch关键字实现generator内switch-case功能，不支持嵌套
#define co_switch(__exp__) \
	_co_for(assert(co_self.__inside && !co_self.__coNextEx),\
		co_self.__coNextEx=(_co_counter+1)/2, __coSwitchSign=true, __forYieldSwitch=false;;__forYieldSwitch=true)\
	if (__forYieldSwitch) {co_self.__coNextEx=0; _co_for_break;}\
	else case _co_counter/2: switch(__coSwitchSign ? co_calc()->unsigned long long{\
	const auto val = (__exp__); static_assert(sizeof(val) <= 4, "switch value must le 32bit");\
	return ((unsigned long long)(unsigned)(val)) | _switch_mark;} : __coNext)

#define co_case(__num__) case (((unsigned long long)(unsigned)(__num__)) | _switch_mark):\
	static_assert(sizeof(decltype(__num__)) <= 4, "case number must le 32bit");

#define co_default() default:;

//push数据到channel/msg_buffer
#define co_chan_push(__chan__, ...) do{(__chan__).push(co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_copy_push(__chan__, ...) do{(__chan__).push(co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_push_void(__chan__) do{(__chan__).push(co_async_result(co_last_state)); _co_await;}while (0)
#define co_chan_try_push(__chan__, ...) do{(__chan__).try_push(co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_try_copy_push(__chan__, ...) do{(__chan__).try_push(co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_try_push_void(__chan__) do{(__chan__).try_push(co_async_result(co_last_state)); _co_await;}while (0)
#define co_chan_timed_push(__chan__, __ms__, ...) do{(__chan__).timed_push(co_timer, __ms__, co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_timed_copy_push(__chan__, __ms__, ...) do{(__chan__).timed_push(co_timer, __ms__, co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_timed_push_void(__chan__, __ms__) do{(__chan__).timed_push(co_timer, __ms__, co_async_result(co_last_state)); _co_await;}while (0)
#define co_csp_push(__chan__, __res__, ...) do{(__chan__).push(co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_copy_push(__chan__, __res__, ...) do{(__chan__).push(co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_push_void(__chan__, __res__) do{(__chan__).push(co_async_result_(co_last_state, __res__)); _co_await;}while (0)
#define co_csp_try_push(__chan__, __res__, ...) do{(__chan__).try_push(co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_try_copy_push(__chan__, __res__, ...) do{(__chan__).try_push(co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_try_push_void(__chan__, __res__) do{(__chan__).try_push(co_async_result_(co_last_state, __res__)); _co_await;}while (0)
#define co_csp_timed_push(__chan__, __ms__, __res__, ...) do{(__chan__).timed_push(co_timer, __ms__, co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_timed_copy_push(__chan__, __ms__, __res__, ...) do{(__chan__).timed_push(co_timer, __ms__, co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_timed_push_void(__chan__, __ms__, __res__) do{(__chan__).timed_push(co_timer, __ms__, co_async_result_(co_last_state, __res__)); _co_await;}while (0)
#define co_chan_tick_push(__chan__, ...) do{(__chan__).tick_push(co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_copy_tick_push(__chan__, ...) do{(__chan__).tick_push(co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_tick_push_void(__chan__) do{(__chan__).tick_push(co_async_result(co_last_state)); _co_await;}while (0)
#define co_chan_try_tick_push(__chan__, ...) do{(__chan__).try_tick_push(co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_try_copy_tick_push(__chan__, ...) do{(__chan__).try_tick_push(co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_try_tick_push_void(__chan__) do{(__chan__).try_tick_push(co_async_result(co_last_state)); _co_await;}while (0)
#define co_chan_timed_tick_push(__chan__, __ms__, ...) do{(__chan__).timed_tick_push(co_timer, __ms__, co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_timed_copy_tick_push(__chan__, __ms__, ...) do{(__chan__).timed_tick_push(co_timer, __ms__, co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_timed_tick_push_void(__chan__, __ms__) do{(__chan__).timed_tick_push(co_timer, __ms__, co_async_result(co_last_state)); _co_await;}while (0)
#define co_csp_tick_push(__chan__, __res__, ...) do{(__chan__).tick_push(co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_copy_tick_push(__chan__, __res__, ...) do{(__chan__).tick_push(co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_tick_push_void(__chan__, __res__) do{(__chan__).tick_push(co_async_result_(co_last_state, __res__)); _co_await;}while (0)
#define co_csp_try_tick_push(__chan__, __res__, ...) do{(__chan__).try_tick_push(co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_try_copy_tick_push(__chan__, __res__, ...) do{(__chan__).try_tick_push(co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_try_tick_push_void(__chan__, __res__) do{(__chan__).try_tick_push(co_async_result_(co_last_state, __res__)); _co_await;}while (0)
#define co_csp_timed_tick_push(__chan__, __ms__, __res__, ...) do{(__chan__).timed_tick_push(co_timer, __ms__, co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_timed_copy_tick_push(__chan__, __ms__, __res__, ...) do{(__chan__).timed_tick_push(co_timer, __ms__, co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_timed_tick_push_void(__chan__, __ms__, __res__) do{(__chan__).timed_tick_push(co_timer, __ms__, co_async_result_(co_last_state, __res__)); _co_await;}while (0)
//push数据到依赖同一个strand的channel/msg_buffer
#define co_chan_aff_push(__chan__, ...) do{(__chan__).aff_push(co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_aff_copy_push(__chan__, ...) do{(__chan__).aff_push(co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_push_void(__chan__) do{(__chan__).aff_push(co_async_result(co_last_state)); _co_await;}while (0)
#define co_chan_aff_try_push(__chan__, ...) do{(__chan__).aff_try_push(co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_aff_try_copy_push(__chan__, ...) do{(__chan__).aff_try_push(co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_try_push_void(__chan__) do{(__chan__).aff_try_push(co_async_result(co_last_state)); _co_await;}while (0)
#define co_chan_aff_timed_push(__chan__, __ms__, ...) do{(__chan__).aff_timed_push(co_timer, __ms__, co_async_result(co_last_state), __VA_ARGS__); _co_await;}while (0)
#define co_chan_aff_timed_copy_push(__chan__, __ms__, ...) do{(__chan__).aff_timed_push(co_timer, __ms__, co_async_result(co_last_state), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_timed_push_void(__chan__, __ms__) do{(__chan__).aff_timed_push(co_timer, __ms__, co_async_result(co_last_state)); _co_await;}while (0)
#define co_csp_aff_push(__chan__, __res__, ...) do{(__chan__).aff_push(co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_aff_copy_push(__chan__, __res__, ...) do{(__chan__).aff_push(co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_aff_push_void(__chan__, __res__) do{(__chan__).aff_push(co_async_result_(co_last_state, __res__)); _co_await;}while (0)
#define co_csp_aff_try_push(__chan__, __res__, ...) do{(__chan__).aff_try_push(co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_aff_try_copy_push(__chan__, __res__, ...) do{(__chan__).aff_try_push(co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_aff_try_push_void(__chan__, __res__) do{(__chan__).aff_try_push(co_async_result_(co_last_state, __res__)); _co_await;}while (0)
#define co_csp_aff_timed_push(__chan__, __ms__, __res__, ...) do{(__chan__).aff_timed_push(co_timer, __ms__, co_async_result_(co_last_state, __res__), __VA_ARGS__); _co_await;}while (0)
#define co_csp_aff_timed_copy_push(__chan__, __ms__, __res__, ...) do{(__chan__).aff_timed_push(co_timer, __ms__, co_async_result_(co_last_state, __res__), forward_copys(__VA_ARGS__)); _co_await;}while (0)
#define co_csp_aff_timed_push_void(__chan__, __ms__, __res__) do{(__chan__).aff_timed_push(co_timer, __ms__, co_async_result_(co_last_state, __res__)); _co_await;}while (0)
//从channel/msg_buffer中读取数据
#define co_chan_pop(__chan__, ...) do{(__chan__).pop(co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_safe_pop(__chan__, ...) do{(__chan__).pop(co_async_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_pop_void(__chan__) do{(__chan__).pop(co_async_result_(co_last_state)); _co_await;}while (0)
#define co_chan_try_pop(__chan__, ...) do{(__chan__).try_pop(co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_try_safe_pop(__chan__, ...) do{(__chan__).try_pop(co_async_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_try_pop_void(__chan__) do{(__chan__).try_pop(co_async_result_(co_last_state)); _co_await;}while (0)
#define co_chan_timed_pop(__chan__, __ms__, ...) do{(__chan__).timed_pop(co_timer, __ms__, co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_timed_safe_pop(__chan__, __ms__, ...) do{(__chan__).timed_pop(co_timer, __ms__, co_async_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_timed_pop_void(__chan__, __ms__) do{(__chan__).timed_pop(co_timer, __ms__, co_async_result_(co_last_state)); _co_await;}while (0)
#define co_chan_tick_pop(__chan__, ...) do{(__chan__).tick_pop(co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_safe_tick_pop(__chan__, ...) do{(__chan__).tick_pop(co_anext_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_tick_pop_void(__chan__) do{(__chan__).tick_pop(co_async_result_(co_last_state)); _co_await;}while (0)
#define co_chan_try_tick_pop(__chan__, ...) do{(__chan__).try_tick_pop(co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_try_safe_tick_pop(__chan__, ...) do{(__chan__).try_tick_pop(co_anext_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_try_tick_pop_void(__chan__) do{(__chan__).try_tick_pop(co_async_result_(co_last_state)); _co_await;}while (0)
#define co_chan_timed_tick_pop(__chan__, __ms__, ...) do{(__chan__).timed_tick_pop(co_timer, __ms__, co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_timed_safe_tick_pop(__chan__, __ms__, ...) do{(__chan__).timed_tick_pop(co_timer, __ms__, co_anext_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_timed_tick_pop_void(__chan__, __ms__) do{(__chan__).timed_tick_pop(co_timer, __ms__, co_async_result_(co_last_state)); _co_await;}while (0)
//从依赖同一个strand的channel/msg_buffer中读取数据
#define co_chan_aff_pop(__chan__, ...) do{(__chan__).aff_pop(co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_safe_pop(__chan__, ...) do{(__chan__).aff_pop(co_async_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_pop_void(__chan__) do{(__chan__).aff_pop(co_async_result_(co_last_state)); _co_await;}while (0)
#define co_chan_aff_try_pop(__chan__, ...) do{(__chan__).aff_try_pop(co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_try_safe_pop(__chan__, ...) do{(__chan__).aff_try_pop(co_async_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_try_pop_void(__chan__) do{(__chan__).aff_try_pop(co_async_result_(co_last_state)); _co_await;}while (0)
#define co_chan_aff_timed_pop(__chan__, __ms__, ...) do{(__chan__).aff_timed_pop(co_timer, __ms__, co_async_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_timed_safe_pop(__chan__, __ms__, ...) do{(__chan__).aff_timed_pop(co_timer, __ms__, co_async_safe_result_(co_last_state, __VA_ARGS__)); _co_await;}while (0)
#define co_chan_aff_timed_pop_void(__chan__, __ms__) do{(__chan__).aff_timed_pop(co_timer, __ms__, co_async_result_(co_last_state)); _co_await;}while (0)
//从一个channel中读取一次数据转接到另一个channel中
#define co_chan_relay(__src_chan__, __dst_chan__) do{(__src_chan__).pop(CoChanRelay_<decltype(__dst_chan__)>(std::move(co_async_this), co_last_state, __dst_chan__)); _co_await;}while (0)
#define co_chan_try_relay(__src_chan__, __dst_chan__) do{(__src_chan__).try_pop(CoChanTryRelay_<decltype(__dst_chan__)>(std::move(co_async_this), co_last_state, __dst_chan__)); _co_await;}while (0)
#define co_chan_tick_relay(__src_chan__, __dst_chan__) do{(__src_chan__).tick_pop(CoChanRelay_<decltype(__dst_chan__)>(std::move(co_async_this), co_last_state, __dst_chan__)); _co_await;}while (0)
#define co_chan_try_tick_relay(__src_chan__, __dst_chan__) do{(__src_chan__).try_tick_pop(CoChanTryRelay_<decltype(__dst_chan__)>(std::move(co_async_this), co_last_state, __dst_chan__)); _co_await;}while (0)
//关闭channel/msg_buffer
#define co_chan_close(__chan__) do{(__chan__).close(co_async); _co_await;}while (0)
//co_chan_io/co_chan_try_io多参数读写时打包
#define co_chan_multi(...) _co_chan_multi(__VA_ARGS__)
#define co_chan_copy_multi(...)  _co_chan_multi(forward_copys(__VA_ARGS__))
//push/pop数据到channel/msg_buffer
#define co_chan_io(__chan__) co_await _make_co_chan_io(__chan__, co_last_state, co_self)
#define co_chan_try_io(__chan__) co_await _make_co_chan_try_io(__chan__, co_last_state, co_self)
#define co_chan_timed_io(__chan__, __ms__) co_await _make_co_chan_timed_io(__ms__, __chan__, co_last_state, co_self, co_timer)
#define co_chan_safe_io(__chan__) co_await _make_co_chan_safe_io(__chan__, co_last_state, co_self)
#define co_chan_try_safe_io(__chan__) co_await _make_co_chan_try_safe_io(__chan__, co_last_state, co_self)
#define co_chan_timed_safe_io(__chan__, __ms__) co_await _make_co_chan_timed_safe_io(__ms__, __chan__, co_last_state, co_self, co_timer)
#define co_csp_io(__chan__, __res__) co_await _make_co_csp_io(__res__, __chan__, co_last_state, co_self)
#define co_csp_try_io(__chan__, __res__) co_await _make_co_csp_try_io(__res__, __chan__, co_last_state, co_self)
#define co_csp_timed_io(__chan__, __ms__, __res__) co_await _make_co_csp_timed_io(__ms__, __res__, __chan__, co_last_state, co_self, co_timer)
#define co_csp_send_void(__chan__, __res__) co_csp_io(__chan__, __res__) << void_type()
#define co_csp_try_send_void(__chan__, __res__) co_csp_try_io(__chan__, __res__) << void_type()
#define co_csp_timed_send_void(__chan__, __ms__, __res__) co_csp_timed_io(__chan__, __ms__, __res__) << void_type()
#define co_csp_wait_void(__chan__, __res__) co_csp_io(__chan__, __res__) >> void_type()
#define co_csp_try_wait_void(__chan__, __res__) co_csp_try_io(__chan__, __res__) >> void_type()
#define co_csp_timed_wait_void(__chan__, __ms__, __res__) co_csp_timed_io(__chan__, __ms__, __res__) >> void_type()
#define co_chan_tick_io(__chan__) co_await _make_co_chan_tick_io(__chan__, co_last_state, co_self)
#define co_chan_try_tick_io(__chan__) co_await _make_co_chan_try_tick_io(__chan__, co_last_state, co_self)
#define co_chan_timed_tick_io(__chan__, __ms__) co_await _make_co_chan_timed_tick_io(__ms__, __chan__, co_last_state, co_self, co_timer)
#define co_chan_safe_tick_io(__chan__) co_await _make_co_chan_safe_tick_io(__chan__, co_last_state, co_self)
#define co_chan_try_safe_tick_io(__chan__) co_await _make_co_chan_try_safe_tick_io(__chan__, co_last_state, co_self)
#define co_chan_timed_safe_tick_io(__chan__, __ms__) co_await _make_co_chan_timed_safe_tick_io(__ms__, __chan__, co_last_state, co_self, co_timer)
#define co_csp_tick_io(__chan__, __res__) co_await _make_co_csp_tick_io(__res__, __chan__, co_last_state, co_self)
#define co_csp_try_tick_io(__chan__, __res__) co_await _make_co_csp_try_tick_io(__res__, __chan__, co_last_state, co_self)
#define co_csp_timed_tick_io(__chan__, __ms__, __res__) co_await _make_co_csp_timed_tick_io(__ms__, __res__, __chan__, co_last_state, co_self, co_timer)
#define co_csp_tick_send_void(__chan__, __res__) co_csp_tick_io(__chan__, __res__) << void_type()
#define co_csp_try_tick_send_void(__chan__, __res__) co_csp_try_tick_io(__chan__, __res__) << void_type()
#define co_csp_timed_tick_send_void(__chan__, __ms__, __res__) co_csp_timed_tick_io(__chan__, __ms__, __res__) << void_type()
#define co_csp_tick_wait_void(__chan__, __res__) co_csp_tick_io(__chan__, __res__) >> void_type()
#define co_csp_try_tick_wait_void(__chan__, __res__) co_csp_try_tick_io(__chan__, __res__) >> void_type()
#define co_csp_timed_tick_wait_void(__chan__, __ms__, __res__) co_csp_timed_tick_io(__chan__, __ms__, __res__) >> void_type()
//push/pop数据到依赖同一个strand的channel/msg_buffer
#define co_chan_aff_io(__chan__) co_await _make_co_chan_aff_io(__chan__, co_last_state, co_self)
#define co_chan_aff_try_io(__chan__) co_await _make_co_chan_aff_try_io(__chan__, co_last_state, co_self)
#define co_chan_aff_timed_io(__chan__, __ms__) co_await _make_co_chan_aff_timed_io(__ms__, __chan__, co_last_state, co_self, co_timer)
#define co_csp_aff_io(__chan__, __res__) co_await _make_co_csp_aff_io(__res__, __chan__, co_last_state, co_self)
#define co_csp_aff_try_io(__chan__, __res__) co_await _make_co_csp_aff_try_io(__res__, __chan__, co_last_state, co_self)
#define co_csp_aff_timed_io(__chan__, __ms__, __res__) co_await _make_co_csp_aff_timed_io(__ms__, __res__, __chan__, co_last_state, co_self, co_timer)
#define co_csp_aff_send_void(__chan__, __res__) co_csp_aff_io(__chan__, __res__) << void_type()
#define co_csp_aff_try_send_void(__chan__, __res__) co_csp_aff_try_io(__chan__, __res__) << void_type()
#define co_csp_aff_timed_send_void(__chan__, __ms__, __res__) co_csp_aff_timed_io(__chan__, __ms__, __res__) << void_type()
#define co_csp_aff_wait_void(__chan__, __res__) co_csp_aff_io(__chan__, __res__) >> void_type()
#define co_csp_aff_try_wait_void(__chan__, __res__) co_csp_aff_try_io(__chan__, __res__) >> void_type()
#define co_csp_aff_timed_wait_void(__chan__, __ms__, __res__) co_csp_aff_timed_io(__chan__, __ms__, __res__) >> void_type()

#define co_use_state co_async_state __coState
#define co_use_id gen_id __coId
#define co_id (__coContext.__coId)
#define co_use_timer overlap_timer::timer_handle __coTimerHandle
#define co_timer (__coContext.__coTimerHandle)

//上一次异步结果状态
#define co_last_state (__coContext.__coState)
#define co_last_state_is_ok (co_async_state::co_async_ok == co_last_state)
#define co_last_state_is_fail (co_async_state::co_async_fail == co_last_state)
#define co_last_state_is_cancel (co_async_state::co_async_cancel == co_last_state)
#define co_last_state_is_closed (co_async_state::co_async_closed == co_last_state)
#define co_last_state_is_overtime (co_async_state::co_async_overtime == co_last_state)

#define co_mutex_lock(__mutex__) do{(__mutex__).lock(co_id, co_async); _co_await;}while (0)
#define co_mutex_try_lock(__mutex__) do{(__mutex__).try_lock(co_id, co_async_result(co_last_state)); _co_await;}while (0)
#define co_mutex_timed_lock(__mutex__, __ms__) do{(__mutex__).timed_lock(co_timer, co_id, __ms__, co_async_result(co_last_state)); _co_await;}while (0)
#define co_mutex_lock_shared(__mutex__) do{(__mutex__).lock_shared(co_id, co_async); _co_await;}while (0)
#define co_mutex_lock_pess_shared(__mutex__) do{(__mutex__).lock_pess_shared(co_id, co_async); _co_await;}while (0)
#define co_mutex_try_lock_shared(__mutex__) do{(__mutex__).try_lock_shared(co_id, co_async_result(co_last_state)); _co_await;}while (0)
#define co_mutex_timed_lock_shared(__mutex__, __ms__) do{(__mutex__).timed_lock_shared(co_timer, co_id, __ms__, co_async_result(co_last_state)); _co_await;}while (0)
#define co_mutex_lock_upgrade(__mutex__) do{(__mutex__).lock_upgrade(co_id, co_async); _co_await;}while (0)
#define co_mutex_try_lock_upgrade(__mutex__) do{(__mutex__).try_lock_upgrade(co_id, co_async_result(co_last_state)); _co_await;}while (0)
#define co_mutex_unlock(__mutex__) do{(__mutex__).unlock(co_id, co_async); _co_await;}while (0)
#define co_mutex_unlock_shared(__mutex__) do{(__mutex__).unlock_shared(co_id, co_async); _co_await;}while (0)
#define co_mutex_unlock_upgrade(__mutex__) do{(__mutex__).unlock_upgrade(co_id, co_async); _co_await;}while (0)

#define _co_mutex_opt_guard(__mutex__, __lock_, __unlock__) \
	co_lock_stop; __lock_(__mutex__);\
	_co_for(__forYieldSwitch = false;;__forYieldSwitch = true)\
	if (__forYieldSwitch) {__unlock__(__mutex__); co_unlock_stop; _co_for_break;}\
	else

#define _co_mutex_try_opt_guard(__mutex__, __lock_, __unlock__) \
	co_lock_stop; __lock_(__mutex__);\
	if (!co_last_state_is_ok) co_unlock_stop;\
	_co_for(__forYieldSwitch = false; co_last_state_is_ok; __forYieldSwitch = true, co_last_state = co_async_state::co_async_ok)\
	if (__forYieldSwitch) {__unlock__(__mutex__); co_unlock_stop; _co_for_break;}\
	else

#define _co_mutex_timed_opt_guard(__mutex__, __ms__, __lock_, __unlock__) \
	co_lock_stop; __lock_(__mutex__, __ms__);\
	if (!co_last_state_is_ok) co_unlock_stop;\
	_co_for(__forYieldSwitch = false; co_last_state_is_ok; __forYieldSwitch = true, co_last_state = co_async_state::co_async_ok)\
	if (__forYieldSwitch) {__unlock__(__mutex__); co_unlock_stop; _co_for_break;}\
	else

#define co_mutex_lock_guard(__mutex__) _co_mutex_opt_guard(__mutex__, co_mutex_lock, co_mutex_unlock)
#define co_mutex_lock_shared_guard(__mutex__) _co_mutex_opt_guard(__mutex__, co_mutex_lock_shared, co_mutex_unlock_shared)
#define co_mutex_lock_pess_shared_guard(__mutex__) _co_mutex_opt_guard(__mutex__, co_mutex_lock_pess_shared, co_mutex_unlock_shared)
#define co_mutex_lock_upgrade_guard(__mutex__) _co_mutex_opt_guard(__mutex__, co_mutex_lock_upgrade, co_mutex_unlock_upgrade)
#define co_mutex_try_lock_guard(__mutex__) _co_mutex_try_opt_guard(__mutex__, co_mutex_try_lock, co_mutex_unlock)
#define co_mutex_try_lock_shared_guard(__mutex__) _co_mutex_try_opt_guard(__mutex__, co_mutex_try_lock_shared, co_mutex_unlock_shared)
#define co_mutex_try_lock_upgrade_guard(__mutex__) _co_mutex_try_opt_guard(__mutex__, co_mutex_try_lock_upgrade, co_mutex_unlock_upgrade)
#define co_mutex_timed_lock_guard(__mutex__, __ms__) _co_mutex_timed_opt_guard(__mutex__, __ms__, co_mutex_timed_lock, co_mutex_unlock)
#define co_mutex_timed_lock_shared_guard(__mutex__, __ms__) _co_mutex_timed_opt_guard(__mutex__, __ms__, co_mutex_timed_lock_shared, co_mutex_unlock_shared)
#define co_convar_wait(__val__, __mutex__) do{(__val__).wait(co_id, __mutex__, co_async); _co_await;}while(0)
#define co_convar_timed_wait(__val__, __mutex__, __ms__) do{(__val__).timed_wait(co_timer, co_id, __ms__, __mutex__, co_async_result(co_last_state)); _co_await;}while(0)

#define co_use_select co_select_sign __selectSign
#define co_select (__coContext.__selectSign)
#define co_select_init __selectSign(co_self)
#define co_select_state (co_select._ntfState)
#define co_select_curr_id ((const size_t&)co_select._selectId)
#define co_select_state_is_ok (co_async_state::co_async_ok == co_select_state)
#define co_select_state_is_fail (co_async_state::co_async_fail == co_select_state)
#define co_select_state_is_cancel (co_async_state::co_async_cancel == co_select_state)
#define co_select_state_is_closed (co_async_state::co_async_closed == co_select_state)
#define co_select_state_is_overtime (co_async_state::co_async_overtime == co_select_state)

//开始从多个channel/msg_buffer中以select方式轮流读取数据(只能与co_end_select配合)
#define co_begin_select_(__label__) {\
	co_lock_stop; co_select._ntfPump.close(co_async); _co_await; co_select.reset();\
	DEBUG_OPERATION(co_select._labelId=__label__);\
	for (__selectStep=0, co_select._selectId=-1, __selectCaseTyiedIoFailed=false;;) {\
		if (1==__selectStep) {\
			do{\
				co_select._ntfPump.pop(co_async_result_(co_ignore, co_select._selectId, co_select_state)); _co_await;\
				co_select._currSign=&co_select._ntfSign[co_select_curr_id];\
				co_select._currSign->_appended=false;\
			}while(co_select._currSign->_disable);\
			__selectStep=1;\
		} else if (2==__selectStep) {\
			break;\
		}\
		if(0){goto __select_ ## __label__; __select_ ## __label__: __selectStep=2; co_select._selectId=-1;}\
		co_begin_switch(co_select_curr_id);\
		co_switch_default; if(1==__selectStep){assert(!"channel unexpected change");}if(0){do{

#define co_begin_select co_begin_select_(0)
#define co_begin_select1 co_begin_select_(1)
#define co_begin_select2 co_begin_select_(2)

//开始从多个channel/msg_buffer中以select方式读取一次数据(只能与co_end_select_once配合)
#define co_begin_select_once {{\
	co_lock_stop; co_select._ntfPump.close(co_async); _co_await; co_select.reset();\
	for (__selectStep=0, co_select._selectId=-1, __selectCaseTyiedIoFailed=false; __selectStep<=2; __selectStep++) {\
		if (1==__selectStep || __selectCaseTyiedIoFailed) {\
			co_select._ntfPump.pop(co_async_result_(co_ignore, co_select._selectId, co_select_state)); _co_await;\
			co_select._currSign=&co_select._ntfSign[co_select_curr_id];\
			assert(!co_select._currSign->_disable);\
			co_select._currSign->_appended=false;\
			__selectCaseTyiedIoFailed=false;\
			__selectStep=1;\
		} else if (2==__selectStep) {\
			co_select._selectId=-1;\
		}\
		co_begin_switch(co_select_curr_id);\
		co_switch_default; if(1==__selectStep){assert(!"channel unexpected change");}if(0){do{

#define co_begin_timed_select_once(__ms__) {{\
	co_lock_stop; co_select._ntfPump.close(co_async); _co_await; co_select.reset();\
	co_timeout(__ms__, co_timer, [&]{co_select._ntfPump.send(0, co_async_state::co_async_ok);});\
	for (__selectStep=0, co_select._selectId=-1, __selectCaseTyiedIoFailed=false; __selectStep<=2; __selectStep++) {\
		if (1==__selectStep || __selectCaseTyiedIoFailed) {\
			co_select._ntfPump.pop(co_async_result_(co_ignore, co_select._selectId, co_select_state)); _co_await;\
			if (0!=co_select_curr_id) {\
				co_select._currSign=&co_select._ntfSign[co_select_curr_id];\
				assert(!co_select._currSign->_disable);\
				co_select._currSign->_appended=false;\
			}\
			__selectCaseTyiedIoFailed=false;\
			__selectStep=1;\
		} else if (2==__selectStep) {\
			co_select._selectId=-1;\
			co_cancel_timer(co_timer);\
		}\
		co_begin_switch(co_select_curr_id);\
		co_switch_default; if(1==__selectStep && 0!=co_select_curr_id){assert(!"channel unexpected change");}if(0==co_select_curr_id) {do{

#define _co_select_case(__chan__) }while(0); __selectCaseStep=0;__selectStep=1;__selectCaseDoSign=true;}if(1==__selectStep) break;\
	co_switch_case((size_t)&(__chan__));\
	if (1!=__selectStep) {\
		co_select._selectId=(size_t)&(__chan__);\
		if (0==__selectStep) {\
			assert(co_select_curr_id && co_select._ntfSign.end()==co_select._ntfSign.find(co_select_curr_id));\
		}\
		co_select._currSign=&co_select._ntfSign[co_select_curr_id];\
		co_select._currSign->_isPush=false;\
	}\
	for(__selectCaseStep=0, __selectCaseDoSign=false; __selectCaseStep<2; __selectCaseStep++)\
	if (1==__selectCaseStep) {\
		if (0==__selectStep || __selectCaseDoSign) {\
			co_notify_sign* const currSign=co_select._currSign;\
			if (!currSign->_appended && !currSign->_disable) {\
				currSign->_appended=true;\
				const size_t chanId=co_select_curr_id;\
				(__chan__).append_pop_notify([&__coContext, chanId](co_async_state st) {\
					if (co_async_state::co_async_fail!=st) {\
						co_select._ntfPump.send(chanId, st);\
					}\
				}, *currSign);\
			}\
		} else if (2==__selectStep) {\
			if (co_select._currSign->_appended && !co_select._currSign->_disable) {\
				(__chan__).remove_pop_notify(co_async_result(co_ignore), *co_select._currSign); _co_await;\
				__selectCaseStep=1;\
				__selectStep=2;\
			}\
		}\
	} else if (1==__selectStep) {do{

#define _co_select_case_of(__chan__) }while(0); __selectCaseStep=0;__selectStep=1;__selectCaseDoSign=true;}if(1==__selectStep) break;\
	co_switch_case((size_t)&(__chan__));\
	if (1!=__selectStep) {\
		co_select._selectId=(size_t)&(__chan__);\
		if (0==__selectStep) {\
			assert(co_select_curr_id && co_select._ntfSign.end()==co_select._ntfSign.find(co_select_curr_id));\
		}\
		co_select._currSign=&co_select._ntfSign[co_select_curr_id];\
		co_select._currSign->_isPush=true;\
	}\
	for(__selectCaseStep=0, __selectCaseDoSign=false; __selectCaseStep<2; __selectCaseStep++)\
	if (1==__selectCaseStep) {\
		if (0==__selectStep || __selectCaseDoSign) {\
			co_notify_sign* const currSign=co_select._currSign;\
			if (!currSign->_appended && !currSign->_disable) {\
				currSign->_appended=true;\
				const size_t chanId=co_select_curr_id;\
				(__chan__).append_push_notify([&__coContext, chanId](co_async_state st) {\
					if (co_async_state::co_async_fail!=st) {\
						co_select._ntfPump.send(chanId, st);\
					}\
				}, *currSign);\
			}\
		} else if (2==__selectStep) {\
			if (co_select._currSign->_appended && !co_select._currSign->_disable) {\
				(__chan__).remove_push_notify(co_async_result(co_ignore), *co_select._currSign); _co_await;\
				__selectCaseStep=1;\
				__selectStep=2;\
			}\
		}\
	} else if (1==__selectStep) {do{

#define _co_select_case_once(__chan__) }while(0); __selectCaseStep=0;__selectStep=1;__selectCaseDoSign=true;}if(1==__selectStep) break;\
	co_switch_case((size_t)&(__chan__));\
	if (1!=__selectStep) {\
		co_select._selectId=(size_t)&(__chan__);\
		if (0==__selectStep) {\
			assert(co_select_curr_id && co_select._ntfSign.end()==co_select._ntfSign.find(co_select_curr_id));\
		};\
		co_select._currSign=&co_select._ntfSign[co_select_curr_id];\
		co_select._currSign->_isPush=false;\
	}\
	for(__selectCaseStep=0; __selectCaseStep<2; __selectCaseStep++)\
	if (1==__selectCaseStep) {\
		if (0==__selectStep || __selectCaseTyiedIoFailed) {\
			co_notify_sign* const currSign=co_select._currSign;\
			if (!currSign->_appended && !currSign->_disable) {\
				currSign->_appended=true;\
				const size_t chanId=co_select_curr_id;\
				(__chan__).append_pop_notify([&__coContext, chanId](co_async_state st) {\
					if (co_async_state::co_async_fail!=st) {\
						co_select._ntfPump.send(chanId, st);\
					}\
				}, *currSign);\
			}\
		} else if (2==__selectStep) {\
			if (co_select._currSign->_appended && !co_select._currSign->_disable) {\
				(__chan__).remove_pop_notify(co_async_result(co_ignore), *co_select._currSign); _co_await;\
				__selectCaseStep=1;\
				__selectStep=2;\
			}\
		}\
	} else if (1==__selectStep) {do{

#define _co_select_case_once_of(__chan__) }while(0); __selectCaseStep=0;__selectStep=1;__selectCaseDoSign=true;}if(1==__selectStep) break;\
	co_switch_case((size_t)&(__chan__));\
	if (1!=__selectStep) {\
		co_select._selectId=(size_t)&(__chan__);\
		if (0==__selectStep) {\
			assert(co_select_curr_id && co_select._ntfSign.end()==co_select._ntfSign.find(co_select_curr_id));\
		};\
		co_select._currSign=&co_select._ntfSign[co_select_curr_id];\
		co_select._currSign->_isPush=true;\
	}\
	for(__selectCaseStep=0; __selectCaseStep<2; __selectCaseStep++)\
	if (1==__selectCaseStep) {\
		if (0==__selectStep || __selectCaseTyiedIoFailed) {\
			co_notify_sign* const currSign=co_select._currSign;\
			if (!currSign->_appended && !currSign->_disable) {\
				currSign->_appended=true;\
				const size_t chanId=co_select_curr_id;\
				(__chan__).append_push_notify([&__coContext, chanId](co_async_state st) {\
					if (co_async_state::co_async_fail!=st) {\
						co_select._ntfPump.send(chanId, st);\
					}\
				}, *currSign);\
			}\
		} else if (2==__selectStep) {\
			if (co_select._currSign->_appended && !co_select._currSign->_disable) {\
				(__chan__).remove_push_notify(co_async_result(co_ignore), *co_select._currSign); _co_await;\
				__selectCaseStep=1;\
				__selectStep=2;\
			}\
		}\
	} else if (1==__selectStep) {do{

#define _co_switch_case_try_io_await \
	if(0){BOND_LINE(__try_io_fail):break;}\
	_co_for(__selectCaseTyiedIo=false, __forYieldSwitch=false;;__forYieldSwitch=true)\
	if (__forYieldSwitch) {\
		if (__selectCaseTyiedIo)\
			_co_await;\
		if (co_select_state_is_fail) {\
			__selectCaseTyiedIoFailed=true;\
			goto BOND_LINE(__try_io_fail);\
		} else {\
			_co_for_break;\
		}\
	} else

//从channel/msg_buffer中检测当前是否有数据，但不立即读取
#define co_select_case_once_(__chan__) _co_select_case_once(__chan__)
#define co_select_case_(__chan__) _co_select_case(__chan__)
//从channel/msg_buffer中读取数据，处理完成后开始下一轮侦听
#define co_select_slow_case_to(__chan__) _co_select_case(__chan__)_co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_chan_try_o(__chan__, co_select_state, co_self)
#define co_select_slow_case_csp_to(__chan__, __res__) _co_select_case(__chan__)_co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_csp_try_o(__res__, __chan__, co_select_state, co_self)
//从channel/msg_buffer中写入数据，处理完成后开始下一轮侦听
#define co_select_slow_case_of(__chan__) _co_select_case_of(__chan__)_co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_chan_try_i(__chan__, co_select_state, co_self)
#define co_select_slow_case_csp_of(__chan__, __res__) _co_select_case_of(__chan__)_co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_csp_try_i(__res__, __chan__, co_select_state, co_self)
//从channel/msg_buffer中读取数据时立即下一轮侦听
#define co_select_case_to(__chan__) _co_select_case(__chan__)_co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_chan_seamless_try_o(__chan__, co_select_state, co_self, co_select)
#define co_select_case_csp_to(__chan__, __res__) _co_select_case(__chan__)_co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_csp_seamless_try_o(__res__, __chan__, co_select_state, co_self, co_select)
//从channel/msg_buffer中写入数据时立即下一轮侦听
#define co_select_case_of(__chan__) _co_select_case_of(__chan__)_co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_chan_seamless_try_i(__chan__, co_select_state, co_self, co_select)
#define co_select_case_csp_of(__chan__, __res__) _co_select_case_of(__chan__)_co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_csp_seamless_try_i(__res__, __chan__, co_select_state, co_self, co_select)
//从channel/msg_buffer中读取一次数据
#define co_select_case_once_to(__chan__) _co_select_case_once(__chan__) _co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_chan_try_o(__chan__, co_select_state, co_self)
#define co_select_case_csp_once_to(__chan__, __res__) _co_select_case_once(__chan__) _co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_csp_try_o(__res__, __chan__, co_select_state, co_self)
//从channel/msg_buffer中写入一次数据
#define co_select_case_once_of(__chan__) _co_select_case_once_of(__chan__) _co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_chan_try_i(__chan__, co_select_state, co_self)
#define co_select_case_csp_once_of(__chan__, __res__) _co_select_case_once_of(__chan__) _co_switch_case_try_io_await if(co_select_state_is_ok)__selectCaseTyiedIo=true,_make_co_csp_try_i(__res__, __chan__, co_select_state, co_self)

//从channel/msg_buffer中读取数据，处理完成后开始下一轮侦听
#define co_select_slow_case(__chan__, ...) _co_select_case(__chan__)\
	if(co_select_state_is_ok){\
		(__chan__).try_pop(co_async_result_(co_select_state, __VA_ARGS__)); _co_await;\
		if(co_select_state_is_fail){\
			__selectCaseTyiedIoFailed=true;\
			break;\
		}\
	}

//从channel/msg_buffer中读取数据时立即下一轮侦听
#define co_select_case(__chan__, ...) _co_select_case(__chan__)\
	if(co_select_state_is_ok){\
		{\
			co_notify_sign* const currSign=co_select._currSign;\
			assert(!currSign->_appended && !currSign->_disable);\
			currSign->_appended=true;\
			const size_t chanId=co_select_curr_id;\
			(__chan__).try_pop_and_append_notify(co_async_result_(co_select_state, __VA_ARGS__), [&__coContext, chanId](co_async_state st) {\
				if (co_async_state::co_async_fail!=st) {\
					co_select._ntfPump.send(chanId, st);\
				}\
			}, *currSign);\
		} _co_await;\
		if(co_select_state_is_fail){\
			__selectCaseTyiedIoFailed=true;\
			break;\
		}\
	}

//从channel/msg_buffer中读取空数据，处理完成后开始下一轮侦听
#define co_select_slow_case_void(__chan__) _co_select_case(__chan__)\
	if(co_select_state_is_ok){\
		(__chan__).try_pop(co_async_result_(co_select_state)); _co_await;\
		if(co_select_state_is_fail){\
			__selectCaseTyiedIoFailed=true;\
			break;\
		}\
	}

//从channel/msg_buffer中读取数据时立即下一轮侦听
#define co_select_case_void(__chan__) _co_select_case(__chan__)\
	if(co_select_state_is_ok){\
		{\
			co_notify_sign* const currSign=co_select._currSign;\
			assert(!currSign->_appended && !currSign->_disable);\
			currSign->_appended=true;\
			const size_t chanId=co_select_curr_id;\
			(__chan__).try_pop_and_append_notify(co_async_result_(co_select_state), [&__coContext, chanId](co_async_state st) {\
				if (co_async_state::co_async_fail!=st) {\
					co_select._ntfPump.send(chanId, st);\
				}\
			}, *currSign);\
		} _co_await;\
		if(co_select_state_is_fail){\
			__selectCaseTyiedIoFailed=true;\
			break;\
		}\
	}

//从channel/msg_buffer中读取一次数据
#define co_select_case_once(__chan__, ...) _co_select_case_once(__chan__)\
	if(co_select_state_is_ok){\
		(__chan__).try_pop(co_async_result_(co_select_state, __VA_ARGS__)); _co_await;\
		if(co_select_state_is_fail){\
			__selectCaseTyiedIoFailed=true;\
			break;\
		}\
	}

//从channel/msg_buffer中读取一次空数据
#define co_select_case_void_once(__chan__) _co_select_case_once(__chan__)\
	if(co_select_state_is_ok){\
		(__chan__).try_pop(co_async_result_(co_select_state)); _co_await;\
		if(co_select_state_is_fail){\
			__selectCaseTyiedIoFailed=true;\
			break;\
		}\
	}

//结束从多个channel/msg_buffer中以select方式读取数据(只能与co_begin_select配合)
#define co_end_select }while(0); __selectCaseStep=0;__selectStep=1;__selectCaseDoSign=true;}\
	co_end_switch; if (0==__selectStep) {assert(!co_select._ntfSign.empty()); __selectStep=1;}}co_unlock_stop;}

//结束从多个channel/msg_buffer中以select方式读取数据(只能与co_begin_select_once配合)
#define co_end_select_once }while(0); __selectCaseStep=0;__selectStep=1;}\
	co_end_switch; if (0==__selectStep) {assert(!co_select._ntfSign.empty());}}co_unlock_stop;}}

//结束select读取(只能在co_begin_select(__label__)/co_end_select中使用)
#define co_select_exit_(__label__) do{assert(__label__==co_select._labelId);__selectCaseStep=3;__selectStep=1;__selectCaseDoSign=false;goto __select_ ## __label__;}while(0)
#define co_select_exit co_select_exit_(0)
#define co_select_exit1 co_select_exit_(1)
#define co_select_exit2 co_select_exit_(2)

//取消一个channel的case
#define co_select_cancel_case(__chan__) \
	do{\
		{\
			const size_t chanId=(size_t)&(__chan__);\
			assert(co_select._ntfSign.end()!=co_select._ntfSign.find(chanId));\
			co_notify_sign& sign=co_select._ntfSign[chanId];\
			if (sign._disable)\
				break;\
			sign._disable=true;\
			if (!sign._appended)\
				break;\
			if (sign._isPush) {\
				(__chan__).remove_push_notify(co_async_result(co_ignore), sign);\
			} else {\
				(__chan__).remove_pop_notify(co_async_result(co_ignore), sign);\
			}\
		} _co_await;\
	}while (0)

//恢复一个channel的case
#define co_select_resume_case(__chan__) \
	do{\
		const size_t chanId=(size_t)&(__chan__);\
		assert(co_select._ntfSign.end()!=co_select._ntfSign.find(chanId));\
		co_notify_sign& sign=co_select._ntfSign[chanId];\
		sign._disable=false;\
		if (chanId!=co_select_curr_id){\
			if (sign._appended)\
				break;\
			sign._appended=true;\
			if (sign._isPush) {\
				(__chan__).append_push_notify([&__coContext, chanId](co_async_state st){\
					if (co_async_state::co_async_fail!=st){\
						co_select._ntfPump.send(chanId, st);\
					}\
				}, sign);\
			} else {\
				(__chan__).append_pop_notify([&__coContext, chanId](co_async_state st){\
					if (co_async_state::co_async_fail!=st){\
						co_select._ntfPump.send(chanId, st);\
					}\
				}, sign);\
			}\
		} else {\
			assert(!sign._appended);\
		}\
	}while (0)

//锁定generator调度器
#define co_hold_work io_work __holdWork
#define co_hold_work_init __holdWork(co_ios)

//包装一个有局部变量的计算，然后返回一个结果
#define co_calc CoLocalWrapCalc_()*[&]
#define co_calc_of CoLocalWrapCalc_()*

class my_actor;
class generator;
class generator_done_sign;
struct CoGo_;
struct CoCreate_;
//generator 句柄
typedef std::shared_ptr<generator> generator_handle;
//generator function入口
typedef std::function<void(generator&)> co_function;

/*!
@brief 基于无栈协程(stackless coroutine)实现的generator
*/
class generator : public ActorTimerFace_
{
	friend my_actor;
	friend io_engine;
	FRIEND_SHARED_PTR(generator);
	struct call_stack_pck
	{
		call_stack_pck(int coNext, int coNextEx, void* ctx, co_function&& handler)
		:_handler(std::move(handler)), _ctx(ctx), _coNext(coNext), _coNextEx(coNextEx) {}

		operator co_function&()
		{
			return _handler;
		}

		co_function _handler;
		void* _ctx;
		int _coNext;
		int _coNextEx;
		RVALUE_CONSTRUCT4(call_stack_pck, _handler, _ctx, _coNext, _coNextEx);
	};
private:
	generator();
	~generator();
public:
	static generator_handle create(shared_strand strand, co_function handler, std::function<void()> notify = std::function<void()>());
	static generator_handle create(shared_strand strand, co_function handler, generator_done_sign& doneSign);
	void run();
	void stop();
	const shared_strand& self_strand();
	generator_handle& shared_this();
	generator_handle& async_this();
	const shared_bool& shared_async_sign();
	static long long alloc_id();
public:
	bool _next();
	generator* _revert_this(generator_handle&);
	void _co_next();
	void _co_tick_next();
	void _co_async_next();
	void _co_top_next();
	void _co_asio_next();
	void _co_shared_async_next(shared_bool& sign);
	bool _done();

	template <typename Handler>
	bool _co_send(const shared_strand& strand, Handler&& handler)
	{
		if (self_strand() != strand)
		{
			strand->post(std::bind([](generator_handle& gen, Handler& handler)
			{
				CHECK_EXCEPTION(handler);
				generator* const self = gen.get();
				self->self_strand()->post(std::bind([](generator_handle& gen)
				{
					gen->_revert_this(gen)->_co_next();
				}, std::move(gen)));
			}, std::move(shared_this()), std::forward<Handler>(handler)));
			return true;
		}
		CHECK_EXCEPTION(handler);
		return false;
	}

	template <typename Handler>
	void _co_async_send(const shared_strand& strand, Handler&& handler)
	{
		strand->try_tick(std::bind([](generator_handle& gen, Handler& handler)
		{
			CHECK_EXCEPTION(handler);
			generator* const self = gen.get();
			self->self_strand()->distribute(std::bind([](generator_handle& gen)
			{
				gen->_revert_this(gen)->_co_next();
			}, std::move(gen)));
		}, std::move(shared_this()), std::forward<Handler>(handler)));
	}

	void _co_sleep(int ms);
	void _co_usleep(long long us);
	void _co_dead_sleep(long long ms);
	void _co_dead_usleep(long long us);
	void _co_push_stack(int coNext, co_function&& handler);
private:
	void timeout_handler();
	static void install(std::atomic<long long>* id);
	static void uninstall();
	static void tls_init();
	static void tls_uninit();
private:
	std::weak_ptr<generator> _weakThis;
	std::shared_ptr<generator> _sharedThis;
	co_function _baseHandler;
	std::function<void()> _notify;
	msg_queue<call_stack_pck> _callStack;
	shared_strand _strand;
	ActorTimer_::timer_handle _timerHandle;
	shared_bool _sharedSign;
	DEBUG_OPERATION(bool _isRun);
	static mem_alloc_base* _genObjAlloc;
	static std::atomic<long long>* _id;
public:
	void* __ctx;
	int __coNext;
	int __coNextEx;
	unsigned char __lockStop;
	bool __readyQuit;
	bool __asyncSign;
	bool __yieldSign;
#if (_DEBUG || DEBUG)
	bool __inside;
	bool __awaitSign;
	bool __sharedAwaitSign;
#endif
	static any_accept __anyAccept;
	NONE_COPY(generator);
};

/*!
@brief generator done监听
*/
class generator_done_sign
{
	friend generator;
	friend CoGo_;
	friend CoCreate_;
public:
	generator_done_sign();
	generator_done_sign(shared_strand strand);
	~generator_done_sign();
public:
	void append_listen(std::function<void()> notify);
	const shared_strand& self_strand();
private:
	void notify_all();
private:
	shared_strand _strand;
	std::list<std::function<void()>> _waitList;
	bool _notified;
};

enum co_async_state : char
{
	co_async_undefined = (char)-1,
	co_async_ok = 0,
	co_async_fail,
	co_async_cancel,
	co_async_closed,
	co_async_overtime
};

class gen_id
{
public:
	gen_id() :_id(generator::alloc_id()) {}
	gen_id(long long id) :_id(id) {}
	operator long long() { return _id; }
	void operator =(long long id) { _id = id; }
private:
	long long _id;
};

struct CoGo_
{
	CoGo_(shared_strand strand, std::function<void()> ntf = std::function<void()>());
	CoGo_(io_engine& ios, std::function<void()> ntf = std::function<void()>());
	CoGo_(shared_strand strand, generator_done_sign& doneSign);
	CoGo_(io_engine& ios, generator_done_sign& doneSign);

	template <typename Handler>
	generator_handle operator-(Handler&& handler)
	{
		generator_handle res = generator::create(std::move(_strand), std::forward<Handler>(handler), std::move(_ntf));
		res->run();
		return res;
	}

	shared_strand _strand;
	std::function<void()> _ntf;
};

struct CoCreate_
{
	CoCreate_(shared_strand strand, std::function<void()> ntf = std::function<void()>());
	CoCreate_(io_engine& ios, std::function<void()> ntf = std::function<void()>());
	CoCreate_(shared_strand strand, generator_done_sign& doneSign);
	CoCreate_(io_engine& ios, generator_done_sign& doneSign);

	template <typename Handler>
	generator_handle operator-(Handler&& handler)
	{
		return generator::create(std::move(_strand), std::forward<Handler>(handler), std::move(_ntf));
	}

	shared_strand _strand;
	std::function<void()> _ntf;
};

struct CoTimeout_
{
	CoTimeout_(int ms, overlap_timer* tm, overlap_timer::timer_handle& th) :_ms(ms), _tm(tm), _th(th){}
	template <typename Handler> void operator-(Handler&& handler) { _tm->timeout(_ms, _th, std::forward<Handler>(handler)); }
	int _ms; overlap_timer* _tm; overlap_timer::timer_handle& _th;
};

struct CouTimeout_
{
	CouTimeout_(long long us, overlap_timer* tm, overlap_timer::timer_handle& th) :_us(us), _tm(tm), _th(th){}
	template <typename Handler> void operator-(Handler&& handler) { _tm->utimeout(_us, _th, std::forward<Handler>(handler)); }
	long long _us; overlap_timer* _tm; overlap_timer::timer_handle& _th;
};

struct CoDeadline_
{
	CoDeadline_(long long us, overlap_timer* tm, overlap_timer::timer_handle& th) :_us(us), _tm(tm), _th(th){}
	template <typename Handler> void operator-(Handler&& handler) { _tm->deadline(_us, _th, std::forward<Handler>(handler)); }
	long long _us; overlap_timer* _tm; overlap_timer::timer_handle& _th;
};

struct CoInterval_
{
	CoInterval_(int ms, overlap_timer* tm, overlap_timer::timer_handle& th, bool immed = false) :_immed(immed), _ms(ms), _tm(tm), _th(th){}
	template <typename Handler> void operator-(Handler&& handler) { _tm->interval(_ms, _th, std::forward<Handler>(handler), _immed); }
	bool _immed; int _ms; overlap_timer* _tm; overlap_timer::timer_handle& _th;
};

struct CouInterval_
{
	CouInterval_(long long us, overlap_timer* tm, overlap_timer::timer_handle& th, bool immed = false) :_immed(immed), _us(us), _tm(tm), _th(th){}
	template <typename Handler> void operator-(Handler&& handler) { _tm->uinterval(_us, _th, std::forward<Handler>(handler), _immed); }
	bool _immed; long long _us; overlap_timer* _tm; overlap_timer::timer_handle& _th;
};

struct CoAsyncIgnoreResult_
{
	CoAsyncIgnoreResult_(generator_handle&& gen)
	:_gen(std::move(gen)) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_gen->_revert_this(_gen)->_co_async_next();
	}

	generator_handle _gen;
	void operator=(const CoAsyncIgnoreResult_&) = delete;
	COPY_CONSTRUCT1(CoAsyncIgnoreResult_, _gen);
};

struct CoAsioIgnoreResult_
{
	CoAsioIgnoreResult_(generator_handle&& gen)
	:_gen(std::move(gen)) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_gen->_revert_this(_gen)->_co_asio_next();
	}

	generator_handle _gen;
	void operator=(const CoAsioIgnoreResult_&) = delete;
	COPY_CONSTRUCT1(CoAsioIgnoreResult_, _gen);
};

struct CoShardAsyncIgnoreResult_
{
	CoShardAsyncIgnoreResult_(generator_handle& gen, const shared_bool& sign)
	:_gen(gen), _sign(sign) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_gen->_co_shared_async_next(_sign);
	}

	generator_handle _gen;
	shared_bool _sign;
	void operator=(const CoShardAsyncIgnoreResult_&) = delete;
	COPY_CONSTRUCT2(CoShardAsyncIgnoreResult_, _gen, _sign);
};

struct CoAnextIgnoreResult_
{
	CoAnextIgnoreResult_(generator_handle&& gen)
	:_gen(std::move(gen)) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_gen->_revert_this(_gen)->_co_next();
	}

	generator_handle _gen;
	void operator=(const CoAnextIgnoreResult_&) = delete;
	COPY_CONSTRUCT1(CoAnextIgnoreResult_, _gen);
};

struct CoAsync_
{
	CoAsync_(generator_handle&& gen);
	void operator()();
	generator_handle _gen;
	void operator=(const CoAsync_&) = delete;
	COPY_CONSTRUCT1(CoAsync_, _gen);
};

struct CoShardAsync_
{
	CoShardAsync_(generator_handle& gen, const shared_bool& sign);
	void operator()();
	generator_handle _gen;
	shared_bool _sign;
	void operator=(const CoShardAsync_&) = delete;
	COPY_CONSTRUCT2(CoShardAsync_, _gen, _sign);
};

struct CoAnext_
{
	CoAnext_(generator_handle&& gen);
	void operator()();
	generator_handle _gen;
	void operator=(const CoAnext_&) = delete;
	COPY_CONSTRUCT1(CoAnext_, _gen);
};

template <typename... _Types>
struct CoAsyncResult_
{
	CoAsyncResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		_gen->_revert_this(_gen)->_co_async_next();
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAsyncResult_&) = delete;
	COPY_CONSTRUCT2(CoAsyncResult_, _gen, _result);
};

template <typename... _Types>
struct CoAsioResult_
{
	CoAsioResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		_gen->_revert_this(_gen)->_co_asio_next();
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAsioResult_&) = delete;
	COPY_CONSTRUCT2(CoAsioResult_, _gen, _result);
};

template <typename... _Types>
struct CoAsyncSafeResult_
{
	CoAsyncSafeResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		if (_gen->self_strand()->running_in_this_thread())
		{
			if (!_gen->_done())
			{
				_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
				_gen->_revert_this(_gen)->_co_top_next();
			}
		}
		else
		{
			typedef std::tuple<RM_CREF(Args)...> tuple_type;
			generator* gen = _gen.get();
			gen->self_strand()->post(std::bind([](generator_handle& _gen, std::tuple<_Types&...>& _result, tuple_type& args)
			{
				if (!_gen->_done())
				{
					_result = std::move(args);
					_gen->_revert_this(_gen)->_co_top_next();
				}
			}, std::move(_gen), _result, tuple_type(std::forward<Args>(args)...)));
		}
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAsyncSafeResult_&) = delete;
	COPY_CONSTRUCT2(CoAsyncSafeResult_, _gen, _result);
};

template <typename... _Types>
struct CoAsyncSameResult_
{
	CoAsyncSameResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		same_copy_to_tuple(_result, std::forward<Args>(args)...);
		_gen->_revert_this(_gen)->_co_async_next();
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAsyncSameResult_&) = delete;
	COPY_CONSTRUCT2(CoAsyncSameResult_, _gen, _result);
};

template <typename... _Types>
struct CoAsyncSameSafeResult_
{
	CoAsyncSameSafeResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		if (_gen->self_strand()->running_in_this_thread())
		{
			if (!_gen->_done())
			{
				same_copy_to_tuple(_result, std::forward<Args>(args)...);
				_gen->_revert_this(_gen)->_co_top_next();
			}
		}
		else
		{
			typedef std::tuple<RM_CREF(Args)...> tuple_type;
			generator* gen = _gen.get();
			gen->self_strand()->post(std::bind([](generator_handle& _gen, std::tuple<_Types&...>& _result, tuple_type& args)
			{
				if (!_gen->_done())
				{
					same_copy_tuple_to_tuple(_result, std::move(args));
					_gen->_revert_this(_gen)->_co_top_next();
				}
			}, std::move(_gen), _result, tuple_type(std::forward<Args>(args)...)));
		}
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAsyncSameSafeResult_&) = delete;
	COPY_CONSTRUCT2(CoAsyncSameSafeResult_, _gen, _result);
};

template <typename... _Types>
struct CoShardAsyncResult_
{
	CoShardAsyncResult_(generator_handle& gen, const shared_bool& sign, _Types&... result)
	:_gen(gen), _sign(sign), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		_gen->_co_shared_async_next(_sign);
	}

	generator_handle _gen;
	shared_bool _sign;
	std::tuple<_Types&...> _result;
	void operator=(const CoShardAsyncResult_&) = delete;
	COPY_CONSTRUCT3(CoShardAsyncResult_, _gen, _sign, _result);
};

template <typename... _Types>
struct CoShardAsyncSafeResult_
{
	CoShardAsyncSafeResult_(generator_handle& gen, const shared_bool& sign, _Types&... result)
	:_gen(gen), _sign(sign), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		if (_gen->self_strand()->running_in_this_thread())
		{
			if (!_gen->_done() && !_sign)
			{
				_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
				_gen->_co_top_next();
			}
		}
		else
		{
			typedef std::tuple<RM_CREF(Args)...> tuple_type;
			generator* gen = _gen.get();
			gen->self_strand()->post(std::bind([](generator_handle& _gen, shared_bool& _sign, std::tuple<_Types&...>& _result, tuple_type& args)
			{
				if (!_gen->_done() && !_sign)
				{
					_result = std::move(args);
					_gen->_co_top_next();
				}
			}, std::move(_gen), std::move(_sign), _result, tuple_type(std::forward<Args>(args)...)));
		}
	}

	generator_handle _gen;
	shared_bool _sign;
	std::tuple<_Types&...> _result;
	void operator=(const CoShardAsyncSafeResult_&) = delete;
	COPY_CONSTRUCT3(CoShardAsyncSafeResult_, _gen, _sign, _result);
};

template <typename... _Types>
struct CoShardAsyncSameResult_
{
	CoShardAsyncSameResult_(generator_handle& gen, const shared_bool& sign, _Types&... result)
	:_gen(gen), _sign(sign), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		same_copy_to_tuple(_result, std::forward<Args>(args)...);
		_gen->_co_shared_async_next(_sign);
	}

	generator_handle _gen;
	shared_bool _sign;
	std::tuple<_Types&...> _result;
	void operator=(const CoShardAsyncSameResult_&) = delete;
	COPY_CONSTRUCT3(CoShardAsyncSameResult_, _gen, _sign, _result);
};

template <typename... _Types>
struct CoShardAsyncSameSafeResult_
{
	CoShardAsyncSameSafeResult_(generator_handle& gen, const shared_bool& sign, _Types&... result)
	:_gen(gen), _sign(sign), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		if (_gen->self_strand()->running_in_this_thread())
		{
			if (!_gen->_done() && !_sign)
			{
				same_copy_to_tuple(_result, std::forward<Args>(args)...);
				_gen->_co_top_next();
			}
		}
		else
		{
			typedef std::tuple<RM_CREF(Args)...> tuple_type;
			generator* gen = _gen.get();
			gen->self_strand()->post(std::bind([](generator_handle& _gen, shared_bool& _sign, std::tuple<_Types&...>& _result, tuple_type& args)
			{
				if (!_gen->_done() && !_sign)
				{
					same_copy_tuple_to_tuple(_result, std::move(args));
					_gen->_co_top_next();
				}
			}, std::move(_gen), std::move(_sign), _result, tuple_type(std::forward<Args>(args)...)));
		}
	}

	generator_handle _gen;
	shared_bool _sign;
	std::tuple<_Types&...> _result;
	void operator=(const CoShardAsyncSameSafeResult_&) = delete;
	COPY_CONSTRUCT3(CoShardAsyncSameSafeResult_, _gen, _sign, _result);
};

template <typename... _Types>
struct CoAnextResult_
{
	CoAnextResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		_gen->_revert_this(_gen)->_co_next();
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAnextResult_&) = delete;
	COPY_CONSTRUCT2(CoAnextResult_, _gen, _result);
};

template <typename... _Types>
struct CoAnextSafeResult_
{
	CoAnextSafeResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		if (_gen->self_strand()->running_in_this_thread())
		{
			if (!_gen->_done())
			{
				_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
				_gen->_revert_this(_gen)->_next();
			}
		}
		else
		{
			typedef std::tuple<RM_CREF(Args)...> tuple_type;
			generator* gen = _gen.get();
			gen->self_strand()->post(std::bind([](generator_handle& _gen, std::tuple<_Types&...>& _result, tuple_type& args)
			{
				if (!_gen->_done())
				{
					_result = std::move(args);
					_gen->_revert_this(_gen)->_next();
				}
			}, std::move(_gen), _result, tuple_type(std::forward<Args>(args)...)));
		}
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAnextSafeResult_&) = delete;
	COPY_CONSTRUCT2(CoAnextSafeResult_, _gen, _result);
};

template <typename... _Types>
struct CoAnextSameResult_
{
	CoAnextSameResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		same_copy_to_tuple(_result, std::forward<Args>(args)...);
		_gen->_revert_this(_gen)->_co_next();
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAnextSameResult_&) = delete;
	COPY_CONSTRUCT2(CoAnextSameResult_, _gen, _result);
};

template <typename... _Types>
struct CoAnextSameSafeResult_
{
	CoAnextSameSafeResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		if (_gen->self_strand()->running_in_this_thread())
		{
			if (!_gen->_done())
			{
				same_copy_to_tuple(_result, std::forward<Args>(args)...);
				_gen->_revert_this(_gen)->_next();
			}
		}
		else
		{
			typedef std::tuple<RM_CREF(Args)...> tuple_type;
			generator* gen = _gen.get();
			gen->self_strand()->post(std::bind([](generator_handle& _gen, std::tuple<_Types&...>& _result, tuple_type& args)
			{
				if (!_gen->_done())
				{
					same_copy_tuple_to_tuple(_result, std::move(args));
					_gen->_revert_this(_gen)->_next();
				}
			}, std::move(_gen), _result, tuple_type(std::forward<Args>(args)...)));
		}
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAnextSameSafeResult_&) = delete;
	COPY_CONSTRUCT2(CoAnextSameSafeResult_, _gen, _result);
};

template <typename Chan>
struct CoChanRelay_
{
	CoChanRelay_(generator_handle&& gen, co_async_state& stateRes, Chan& chan)
	:_gen(std::move(gen)), _stateRes(stateRes), _chan(chan) {}

	template <typename... Args>
	void operator()(co_async_state state, Args&&... args)
	{
		assert(_gen);
		if (co_async_state::co_async_ok == state)
		{
			_chan.push(CoAsyncResult_<co_async_state>(std::move(_gen), _stateRes), std::forward<Args>(args)...);
		}
		else
		{
			_stateRes = state;
			_gen->_revert_this(_gen)->_co_async_next();
		}
	}

	void operator()(co_async_state state)
	{
		assert(_gen);
		assert(co_async_state::co_async_ok != state);
		_stateRes = state;
		_gen->_revert_this(_gen)->_co_async_next();
	}

	Chan& _chan;
	co_async_state& _stateRes;
	generator_handle _gen;
	void operator=(const CoChanRelay_&) = delete;
	COPY_CONSTRUCT3(CoChanRelay_, _chan, _stateRes, _gen);
};

template <typename Chan>
struct CoChanTryRelay_
{
	CoChanTryRelay_(generator_handle&& gen, co_async_state& stateRes, Chan& chan)
	:_gen(std::move(gen)), _stateRes(stateRes), _chan(chan) {}

	template <typename... Args>
	void operator()(co_async_state state, Args&&... args)
	{
		assert(_gen);
		if (co_async_state::co_async_ok == state)
		{
			_chan.try_push(CoAsyncResult_<co_async_state>(std::move(_gen), _stateRes), std::forward<Args>(args)...);
		}
		else
		{
			_stateRes = state;
			_gen->_revert_this(_gen)->_co_async_next();
		}
	}

	void operator()(co_async_state state)
	{
		assert(_gen);
		assert(co_async_state::co_async_ok != state);
		_stateRes = state;
		_gen->_revert_this(_gen)->_co_async_next();
	}

	Chan& _chan;
	co_async_state& _stateRes;
	generator_handle _gen;
	void operator=(const CoChanTryRelay_&) = delete;
	COPY_CONSTRUCT3(CoChanTryRelay_, _chan, _stateRes, _gen);
};

template <typename... Args>
CoAsyncResult_<Args...> _co_async_result(generator_handle&& gen, Args&... result)
{
	return CoAsyncResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoAsioResult_<Args...> _co_asio_result(generator_handle&& gen, Args&... result)
{
	return CoAsioResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoAsyncSafeResult_<Args...> _co_async_safe_result(generator_handle&& gen, Args&... result)
{
	return CoAsyncSafeResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoShardAsyncResult_<Args...> _co_shared_async_result(generator_handle& gen, const shared_bool& sign, Args&... result)
{
	return CoShardAsyncResult_<Args...>(gen, sign, result...);
}

template <typename... Args>
CoShardAsyncSafeResult_<Args...> _co_shared_async_safe_result(generator_handle& gen, const shared_bool& sign, Args&... result)
{
	return CoShardAsyncSafeResult_<Args...>(gen, sign, result...);
}

template <typename... Args>
CoAnextResult_<Args...> _co_anext_result(generator_handle&& gen, Args&... result)
{
	return CoAnextResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoAnextSafeResult_<Args...> _co_anext_safe_result(generator_handle&& gen, Args&... result)
{
	return CoAnextSafeResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoAsyncSameResult_<Args...> _co_async_same_result(generator_handle&& gen, Args&... result)
{
	return CoAsyncSameResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoAsyncSameSafeResult_<Args...> _co_async_same_safe_result(generator_handle&& gen, Args&... result)
{
	return CoAsyncSameSafeResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoShardAsyncSameResult_<Args...> _co_shared_async_same_result(generator_handle& gen, const shared_bool& sign, Args&... result)
{
	return CoShardAsyncSameResult_<Args...>(gen, sign, result...);
}

template <typename... Args>
CoShardAsyncSameSafeResult_<Args...> _co_shared_async_same_safe_result(generator_handle& gen, const shared_bool& sign, Args&... result)
{
	return CoShardAsyncSameSafeResult_<Args...>(gen, sign, result...);
}

template <typename... Args>
CoAnextSameResult_<Args...> _co_anext_same_result(generator_handle&& gen, Args&... result)
{
	return CoAnextSameResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoAnextSameSafeResult_<Args...> _co_anext_same_safe_result(generator_handle&& gen, Args&... result)
{
	return CoAnextSameSafeResult_<Args...>(std::move(gen), result...);
}

template <bool>
struct CoCallBind_
{
	template <typename Handler, typename... Args>
	static co_function bind(Handler&& handler, Args&&... args)
	{
		return std::bind(std::forward<Handler>(handler), __1, std::forward<Args>(args)...);
	}

	template <typename Handler>
	static co_function bind(Handler&& handler)
	{
		return std::forward<Handler>(handler);
	}
};

template <>
struct CoCallBind_<true>
{
	template <typename Func, typename Obj, typename... Args>
	static co_function bind(Func func, Obj&& obj, Args&&... args)
	{
		return std::bind(func, std::forward<Obj>(obj), __1, std::forward<Args>(args)...);
	}
};

template <typename Handler, typename Unknown, typename... Args>
static co_function _co_call_bind(Handler&& handler, Unknown&& unkown, Args&&... args)
{
	return CoCallBind_<CheckClassFunc_<RM_REF(Handler)>::value>::bind(std::forward<Handler>(handler), std::forward<Unknown>(unkown), std::forward<Args>(args)...);
}

template <typename Handler>
static co_function _co_call_bind(Handler&& handler)
{
	static_assert(!CheckClassFunc_<RM_REF(Handler)>::value, "");
	return CoCallBind_<false>::bind(std::forward<Handler>(handler));
}

struct CoCall_
{
	CoCall_(generator& gen, int coNext)
	:co_self(gen), _coNext(coNext) {}

	template <typename Handler>
	void operator-(Handler&& handler)
	{
		co_self._co_push_stack(_coNext, std::forward<Handler>(handler));
	}
private:
	generator& co_self;
	int _coNext;
};

struct CoStCall
{
	CoStCall(shared_strand strand, generator& gen)
	:_strand(std::move(strand)), co_self(gen) {}

	template <typename Handler>
	void operator-(Handler&& handler)
	{
		generator::create(std::move(_strand), std::forward<Handler>(handler), co_async)->run();
	}
private:
	generator& co_self;
	shared_strand _strand;
};

struct CoBestCall
{
	CoBestCall(shared_strand strand, generator& gen, bool& isBest, int coNext)
	:_strand(std::move(strand)), co_self(gen), _isBest(isBest), _coNext(coNext) {}

	template <typename Handler>
	void operator-(Handler&& handler)
	{
		_isBest = co_strand == _strand;
		if (_isBest)
		{
			co_self._co_push_stack(_coNext, std::forward<Handler>(handler));
		}
		else
		{
			generator::create(std::move(_strand), std::forward<Handler>(handler), co_async)->run();
		}
	}
private:
	generator& co_self;
	bool& _isBest;
	shared_strand _strand;
	int _coNext;
};

struct CoLocalWrapCalc_
{
	template <typename Handler>
	auto operator*(Handler&& handler)->decltype(handler())
	{
		return handler();
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Handler> struct CoNotifyHandler_;
template <typename Handler> struct CoNilStateNotifyHandler_;

struct CoNotifyHandlerFace_
{
	virtual void invoke(reusable_mem& alloc, co_async_state state = co_async_state::co_async_ok) = 0;
	virtual void destroy() = 0;

	template <typename Handler>
	static CoNotifyHandlerFace_* wrap_notify(reusable_mem& alloc, Handler&& handler)
	{
		typedef CoNotifyHandler_<Handler> Handler_;
		return new(alloc.allocate(sizeof(Handler_)))Handler_(handler);
	}

	template <typename Handler>
	static CoNotifyHandlerFace_* wrap_nil_state_notify(reusable_mem& alloc, Handler&& handler)
	{
		typedef CoNilStateNotifyHandler_<Handler> Handler_;
		return new(alloc.allocate(sizeof(Handler_)))Handler_(handler);
	}
};

template <typename Handler>
struct CoNotifyHandler_ : public CoNotifyHandlerFace_
{
	typedef RM_CREF(Handler) handler_type;

	CoNotifyHandler_(Handler& handler)
		:_handler(std::forward<Handler>(handler)) {}

	void invoke(reusable_mem& alloc, co_async_state state)
	{
		Handler handler(std::move(_handler));
		destroy();
		alloc.deallocate(this);
		CHECK_EXCEPTION(handler, state);
	}

	void destroy()
	{
		this->~CoNotifyHandler_();
	}

	handler_type _handler;
	NONE_COPY(CoNotifyHandler_);
	RVALUE_CONSTRUCT1(CoNotifyHandler_, _handler);
};

template <typename Handler>
struct CoNilStateNotifyHandler_ : public CoNotifyHandlerFace_
{
	typedef RM_CREF(Handler) handler_type;

	CoNilStateNotifyHandler_(Handler& handler)
		:_handler(std::forward<Handler>(handler)) {}

	void invoke(reusable_mem& alloc, co_async_state state)
	{
		assert(co_async_state::co_async_ok == state);
		Handler handler(std::move(_handler));
		destroy();
		alloc.deallocate(this);
		CHECK_EXCEPTION(handler);
	}

	void destroy()
	{
		this->~CoNilStateNotifyHandler_();
	}

	handler_type _handler;
	NONE_COPY(CoNilStateNotifyHandler_);
	RVALUE_CONSTRUCT1(CoNilStateNotifyHandler_, _handler);
};

template <typename... Types>
struct CoChanMulti_ : public std::tuple<Types...>
{
	template <typename... Args>
	CoChanMulti_(Args&&... args)
		:std::tuple<Types...>(std::forward<Args>(args)...){}
};

template <typename... Type>
CoChanMulti_<Type&&...> _co_chan_multi(Type&&... args)
{
	return CoChanMulti_<Type&&...>(std::forward<Type>(args)...);
}

template <typename Chan> struct CoChanIo_
{
	CoChanIo_(Chan& chan, co_async_state& state, generator& host):_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.push(co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.pop(co_async_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanIo_<Chan> _make_co_chan_io(Chan& chan, co_async_state& state, generator& host) { return CoChanIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanTickIo_
{
	CoChanTickIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.tick_push(co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.tick_pop(co_async_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanTickIo_<Chan> _make_co_chan_tick_io(Chan& chan, co_async_state& state, generator& host) { return CoChanTickIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanSafeIo_
{
	CoChanSafeIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanSafeIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanSafeIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanSafeIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.push(co_async_safe_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanSafeIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.pop(co_async_safe_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanSafeIo_<Chan> _make_co_chan_safe_io(Chan& chan, co_async_state& state, generator& host) { return CoChanSafeIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanSafeTickIo_
{
	CoChanSafeTickIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanSafeTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanSafeTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanSafeTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.tick_push(co_async_safe_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanSafeTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.tick_pop(co_async_safe_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanSafeTickIo_<Chan> _make_co_chan_safe_tick_io(Chan& chan, co_async_state& state, generator& host) { return CoChanSafeTickIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanTryIo_
{
	CoChanTryIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTryIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTryIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTryIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_push(co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTryIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_pop(co_async_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanTryIo_<Chan> _make_co_chan_try_io(Chan& chan, co_async_state& state, generator& host) { return CoChanTryIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanTryTickIo_
{
	CoChanTryTickIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTryTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTryTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTryTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_tick_push(co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTryTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_tick_pop(co_async_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanTryTickIo_<Chan> _make_co_chan_try_tick_io(Chan& chan, co_async_state& state, generator& host) { return CoChanTryTickIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanTryI_
{
	CoChanTryI_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTryI_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
private:
	template <typename... Args> static void push(CoChanTryI_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_push(co_async_result(this_->_state), std::forward<Args>(args)...); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanTryI_<Chan> _make_co_chan_try_i(Chan& chan, co_async_state& state, generator& host) { return CoChanTryI_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanTryO_
{
	CoChanTryO_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTryO_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void pop(CoChanTryO_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_pop(co_async_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanTryO_<Chan> _make_co_chan_try_o(Chan& chan, co_async_state& state, generator& host) { return CoChanTryO_<Chan>(chan, state, host); }

struct co_select_sign;
template <typename Chan> struct CoChanSeamlessTryO_
{
	CoChanSeamlessTryO_(Chan& chan, co_async_state& state, generator& host, co_select_sign& selectSign) :_chan(chan), _state(state), _host(host), _selectSign(selectSign) {}
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanSeamlessTryO_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void pop(CoChanSeamlessTryO_* this_, Args&&... args);
private:
	Chan& _chan; co_async_state& _state; generator& _host; co_select_sign& _selectSign;
};
template <typename Chan> CoChanSeamlessTryO_<Chan> _make_co_chan_seamless_try_o(Chan& chan, co_async_state& state, generator& host, co_select_sign& selectSign) { return CoChanSeamlessTryO_<Chan>(chan, state, host, selectSign); }

struct co_select_sign;
template <typename Chan> struct CoChanSeamlessTryI_
{
	CoChanSeamlessTryI_(Chan& chan, co_async_state& state, generator& host, co_select_sign& selectSign) :_chan(chan), _state(state), _host(host), _selectSign(selectSign) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanSeamlessTryI_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
private:
	template <typename... Args> static void push(CoChanSeamlessTryI_* this_, Args&&... args);
private:
	Chan& _chan; co_async_state& _state; generator& _host; co_select_sign& _selectSign;
};
template <typename Chan> CoChanSeamlessTryI_<Chan> _make_co_chan_seamless_try_i(Chan& chan, co_async_state& state, generator& host, co_select_sign& selectSign) { return CoChanSeamlessTryI_<Chan>(chan, state, host, selectSign); }

template <typename Chan> struct CoChanTrySafeIo_
{
	CoChanTrySafeIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTrySafeIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTrySafeIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTrySafeIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_push(co_async_safe_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTrySafeIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_pop(co_async_safe_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanTrySafeIo_<Chan> _make_co_chan_try_safe_io(Chan& chan, co_async_state& state, generator& host) { return CoChanTrySafeIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanTrySafeTickIo_
{
	CoChanTrySafeTickIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTrySafeTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTrySafeTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTrySafeTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_tick_push(co_async_safe_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTrySafeTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_tick_pop(co_async_safe_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanTrySafeTickIo_<Chan> _make_co_chan_try_safe_tick_io(Chan& chan, co_async_state& state, generator& host) { return CoChanTrySafeTickIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanTimedIo_
{
	CoChanTimedIo_(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) :_tm(ms), _chan(chan), _state(state), _host(host), _timer(timer) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTimedIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTimedIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTimedIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_push(this_->_timer, this_->_tm, co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTimedIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_pop(this_->_timer, this_->_tm, co_async_result_(this_->_state, args...)); }
private:
	int _tm; Chan& _chan; co_async_state& _state; generator& _host; overlap_timer::timer_handle& _timer;
};
template <typename Chan> CoChanTimedIo_<Chan> _make_co_chan_timed_io(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) { return CoChanTimedIo_<Chan>(ms, chan, state, host, timer); }

template <typename Chan> struct CoChanTimedTickIo_
{
	CoChanTimedTickIo_(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) :_tm(ms), _chan(chan), _state(state), _host(host), _timer(timer) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTimedTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTimedTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTimedTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_tick_push(this_->_timer, this_->_tm, co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTimedTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_tick_pop(this_->_timer, this_->_tm, co_async_result_(this_->_state, args...)); }
private:
	int _tm; Chan& _chan; co_async_state& _state; generator& _host; overlap_timer::timer_handle& _timer;
};
template <typename Chan> CoChanTimedTickIo_<Chan> _make_co_chan_timed_tick_io(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) { return CoChanTimedTickIo_<Chan>(ms, chan, state, host, timer); }

template <typename Chan> struct CoChanTimedSafeIo_
{
	CoChanTimedSafeIo_(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) :_tm(ms), _chan(chan), _state(state), _host(host), _timer(timer) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTimedSafeIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTimedSafeIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTimedSafeIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_push(this_->_timer, this_->_tm, co_async_safe_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTimedSafeIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_pop(this_->_timer, this_->_tm, co_async_safe_result_(this_->_state, args...)); }
private:
	int _tm; Chan& _chan; co_async_state& _state; generator& _host; overlap_timer::timer_handle& _timer;
};
template <typename Chan> CoChanTimedSafeIo_<Chan> _make_co_chan_timed_safe_io(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) { return CoChanTimedSafeIo_<Chan>(ms, chan, state, host, timer); }

template <typename Chan> struct CoChanTimedSafeTickIo_
{
	CoChanTimedSafeTickIo_(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) :_tm(ms), _chan(chan), _state(state), _host(host), _timer(timer) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTimedSafeTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanTimedSafeTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanTimedSafeTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_tick_push(this_->_timer, this_->_tm, co_async_safe_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanTimedSafeTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_tick_pop(this_->_timer, this_->_tm, co_async_safe_result_(this_->_state, args...)); }
private:
	int _tm; Chan& _chan; co_async_state& _state; generator& _host; overlap_timer::timer_handle& _timer;
};
template <typename Chan> CoChanTimedSafeTickIo_<Chan> _make_co_chan_timed_safe_tick_io(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) { return CoChanTimedSafeTickIo_<Chan>(ms, chan, state, host, timer); }

template <typename R, typename CspChan>
struct CoCspIo_
{
	CoCspIo_(R& res, CspChan& chan, co_async_state& state, generator& host) :_res(res), _chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.push(co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.pop(co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host;
};
template <typename R, typename CspChan> CoCspIo_<R, CspChan> _make_co_csp_io(R& res, CspChan& chan, co_async_state& state, generator& host) { return CoCspIo_<R, CspChan>(res, chan, state, host); };

template <typename R, typename CspChan>
struct CoCspTickIo_
{
	CoCspTickIo_(R& res, CspChan& chan, co_async_state& state, generator& host) :_res(res), _chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.tick_push(co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.tick_pop(co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host;
};
template <typename R, typename CspChan> CoCspTickIo_<R, CspChan> _make_co_csp_tick_io(R& res, CspChan& chan, co_async_state& state, generator& host) { return CoCspTickIo_<R, CspChan>(res, chan, state, host); };

template <typename R, typename CspChan>
struct CoCspTryIo_
{
	CoCspTryIo_(R& res, CspChan& chan, co_async_state& state, generator& host) :_res(res), _chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTryIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTryIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspTryIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_push(co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspTryIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_pop(co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host;
};
template <typename R, typename CspChan> CoCspTryIo_<R, CspChan> _make_co_csp_try_io(R& res, CspChan& chan, co_async_state& state, generator& host) { return CoCspTryIo_<R, CspChan>(res, chan, state, host); };

template <typename R, typename CspChan>
struct CoCspTryTickIo_
{
	CoCspTryTickIo_(R& res, CspChan& chan, co_async_state& state, generator& host) :_res(res), _chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTryTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTryTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspTryTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_tick_push(co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspTryTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_tick_pop(co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host;
};
template <typename R, typename CspChan> CoCspTryTickIo_<R, CspChan> _make_co_csp_try_tick_io(R& res, CspChan& chan, co_async_state& state, generator& host) { return CoCspTryTickIo_<R, CspChan>(res, chan, state, host); };

template <typename R, typename CspChan>
struct CoCspTryI_
{
	CoCspTryI_(R& res, CspChan& chan, co_async_state& state, generator& host) :_res(res), _chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTryI_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
private:
	template <typename... Args> static void push(CoCspTryI_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_push(co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host;
};
template <typename R, typename CspChan> CoCspTryI_<R, CspChan> _make_co_csp_try_i(R& res, CspChan& chan, co_async_state& state, generator& host) { return CoCspTryI_<R, CspChan>(res, chan, state, host); };

template <typename R, typename CspChan>
struct CoCspTryO_
{
	CoCspTryO_(R& res, CspChan& chan, co_async_state& state, generator& host) :_res(res), _chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTryO_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void pop(CoCspTryO_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.try_pop(co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host;
};
template <typename R, typename CspChan> CoCspTryO_<R, CspChan> _make_co_csp_try_o(R& res, CspChan& chan, co_async_state& state, generator& host) { return CoCspTryO_<R, CspChan>(res, chan, state, host); };

template <typename R, typename CspChan>
struct CoCspSeamlessTryO_
{
	CoCspSeamlessTryO_(R& res, CspChan& chan, co_async_state& state, generator& host, co_select_sign& selectSign) :_res(res), _chan(chan), _state(state), _host(host), _selectSign(selectSign) {}
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspSeamlessTryO_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void pop(CoCspSeamlessTryO_* this_, Args&&... args);
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host; co_select_sign& _selectSign;
};
template <typename R, typename CspChan> CoCspSeamlessTryO_<R, CspChan> _make_co_csp_seamless_try_o(R& res, CspChan& chan, co_async_state& state, generator& host, co_select_sign& selectSign)
{
	return CoCspSeamlessTryO_<R, CspChan>(res, chan, state, host, selectSign);
};

template <typename R, typename CspChan>
struct CoCspSeamlessTryI_
{
	CoCspSeamlessTryI_(R& res, CspChan& chan, co_async_state& state, generator& host, co_select_sign& selectSign) :_res(res), _chan(chan), _state(state), _host(host), _selectSign(selectSign) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspSeamlessTryI_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
private:
	template <typename... Args> static void push(CoCspSeamlessTryI_* this_, Args&&... args);
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host; co_select_sign& _selectSign;
};
template <typename R, typename CspChan> CoCspSeamlessTryI_<R, CspChan> _make_co_csp_seamless_try_i(R& res, CspChan& chan, co_async_state& state, generator& host, co_select_sign& selectSign)
{
	return CoCspSeamlessTryI_<R, CspChan>(res, chan, state, host, selectSign);
};

template <typename R, typename CspChan>
struct CoCspTimedIo_
{
	CoCspTimedIo_(int ms, R& res, CspChan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) :_tm(ms), _res(res), _chan(chan), _state(state), _host(host), _timer(timer) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTimedIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTimedIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspTimedIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_push(this_->_timer, this_->_tm, co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspTimedIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_pop(this_->_timer, this_->_tm, co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	int _tm; R& _res; CspChan& _chan; co_async_state& _state; generator& _host; overlap_timer::timer_handle& _timer;
};
template <typename R, typename CspChan> CoCspTimedIo_<R, CspChan> _make_co_csp_timed_io(int ms, R& res, CspChan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) { return CoCspTimedIo_<R, CspChan>(ms, res, chan, state, host, timer); };

template <typename R, typename CspChan>
struct CoCspTimedTickIo_
{
	CoCspTimedTickIo_(int ms, R& res, CspChan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) :_tm(ms), _res(res), _chan(chan), _state(state), _host(host), _timer(timer) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTimedTickIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspTimedTickIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspTimedTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_tick_push(this_->_timer, this_->_tm, co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspTimedTickIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.timed_tick_pop(this_->_timer, this_->_tm, co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	int _tm; R& _res; CspChan& _chan; co_async_state& _state; generator& _host; overlap_timer::timer_handle& _timer;
};
template <typename R, typename CspChan> CoCspTimedTickIo_<R, CspChan> _make_co_csp_timed_tick_io(int ms, R& res, CspChan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) { return CoCspTimedTickIo_<R, CspChan>(ms, res, chan, state, host, timer); };

template <typename Chan> struct CoChanAffIo_
{
	CoChanAffIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanAffIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanAffIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanAffIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_push(co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanAffIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_pop(co_async_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanAffIo_<Chan> _make_co_chan_aff_io(Chan& chan, co_async_state& state, generator& host) { return CoChanAffIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanAffTryIo_
{
	CoChanAffTryIo_(Chan& chan, co_async_state& state, generator& host) :_chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanAffTryIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanAffTryIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); }
	void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanAffTryIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_try_push(co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanAffTryIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_try_pop(co_async_result_(this_->_state, args...)); }
private:
	Chan& _chan; co_async_state& _state; generator& _host;
};
template <typename Chan> CoChanAffTryIo_<Chan> _make_co_chan_aff_try_io(Chan& chan, co_async_state& state, generator& host) { return CoChanAffTryIo_<Chan>(chan, state, host); }

template <typename Chan> struct CoChanAffTimedIo_
{
	CoChanAffTimedIo_(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) :_tm(ms), _chan(chan), _state(state), _host(host), _timer(timer) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanAffTimedIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoChanAffTimedIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoChanAffTimedIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_timed_push(this_->_timer, this_->_tm, co_async_result(this_->_state), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoChanAffTimedIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_timed_pop(this_->_timer, this_->_tm, co_async_result_(this_->_state, args...)); }
private:
	int _tm; Chan& _chan; co_async_state& _state; generator& _host; overlap_timer::timer_handle& _timer;
};
template <typename Chan> CoChanAffTimedIo_<Chan> _make_co_chan_aff_timed_io(int ms, Chan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) { return CoChanAffTimedIo_<Chan>(ms, chan, state, host, timer); }

template <typename R, typename CspChan>
struct CoCspAffIo_
{
	CoCspAffIo_(R& res, CspChan& chan, co_async_state& state, generator& host) :_res(res), _chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspAffIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspAffIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspAffIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_push(co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspAffIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_pop(co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host;
};
template <typename R, typename CspChan> CoCspAffIo_<R, CspChan> _make_co_csp_aff_io(R& res, CspChan& chan, co_async_state& state, generator& host) { return CoCspAffIo_<R, CspChan>(res, chan, state, host); };

template <typename R, typename CspChan>
struct CoCspAffTryIo_
{
	CoCspAffTryIo_(R& res, CspChan& chan, co_async_state& state, generator& host) :_res(res), _chan(chan), _state(state), _host(host) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspAffTryIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspAffTryIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspAffTryIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_try_push(co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspAffTryIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_try_pop(co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	R& _res; CspChan& _chan; co_async_state& _state; generator& _host;
};
template <typename R, typename CspChan> CoCspAffTryIo_<R, CspChan> _make_co_csp_aff_try_io(R& res, CspChan& chan, co_async_state& state, generator& host) { return CoCspAffTryIo_<R, CspChan>(res, chan, state, host); };

template <typename R, typename CspChan>
struct CoCspAffTimedIo_
{
	CoCspAffTimedIo_(int ms, R& res, CspChan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) :_tm(ms), _res(res), _chan(chan), _state(state), _host(host), _timer(timer) {}
	template <typename Arg> void operator<<(Arg&& arg) { push(this, std::forward<Arg>(arg)); }
	template <typename Arg> void operator>>(Arg&& arg) { static_assert(!std::is_rvalue_reference<Arg&&>::value, ""); pop(this, std::forward<Arg>(arg)); }
	template <typename... Args> void operator<<(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspAffTimedIo_::push<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	template <typename... Args> void operator>>(CoChanMulti_<Args...>&& args) { tuple_invoke(&CoCspAffTimedIo_::pop<Args...>, std::make_tuple(this), (std::tuple<Args...>&&)args); }
	void operator<<(void_type&&) { push(this); } void operator>>(void_type&&) { pop(this); }
private:
	template <typename... Args> static void push(CoCspAffTimedIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_timed_push(this_->_timer, this_->_tm, co_async_result_(this_->_state, this_->_res), std::forward<Args>(args)...); }
	template <typename... Args> static void pop(CoCspAffTimedIo_* this_, Args&&... args) { co_generator = this_->_host; this_->_chan.aff_timed_pop(this_->_timer, this_->_tm, co_async_safe_result_(this_->_state, this_->_res, args...)); }
private:
	int _tm; R& _res; CspChan& _chan; co_async_state& _state; generator& _host; overlap_timer::timer_handle& _timer;
};
template <typename R, typename CspChan> CoCspAffTimedIo_<R, CspChan> _make_co_csp_aff_timed_io(int ms, R& res, CspChan& chan, co_async_state& state, generator& host, overlap_timer::timer_handle& timer) { return CoCspAffTimedIo_<R, CspChan>(ms, res, chan, state, host, timer); };

template <typename... Types>
class co_msg_buffer;
template <typename... Types>
class co_channel;
template <typename... Types>
class co_nil_channel;
template <typename... Types>
class co_csp_channel;

typedef msg_list<CoNotifyHandlerFace_*>::iterator co_notify_node;

struct co_notify_sign
{
	co_notify_sign()
	:_isPush(false), _nodeEffect(false), _appended(false), _disable(false) {}

	co_notify_node _ntfNode;
	bool _isPush:1;
	bool _nodeEffect:1;
	bool _appended:1;
	bool _disable:1;
	NONE_COPY(co_notify_sign);
};

template <typename Chan>
struct CoOtherReceiver_
{
	template <typename... Args>
	void operator()(co_async_state state, Args&&... msg)
	{
		assert(co_async_state::co_async_ok == state);
		_chan.push(any_handler(), std::forward<Args>(msg)...);
	}

	void operator()(co_async_state state)
	{
		assert(co_async_state::co_async_ok != state);
	}

	Chan& _chan;
};

template <typename Chan>
struct CoWrapSend_
{
	template <typename... Args>
	void operator()(Args&&... msg)
	{
		_chan.send(std::forward<Args>(msg)...);
	}

	Chan& _chan;
};

template <typename Chan>
struct CoWrapPost_
{
	template <typename... Args>
	void operator()(Args&&... msg)
	{
		_chan.post(std::forward<Args>(msg)...);
	}

	Chan& _chan;
};

template <typename Chan>
struct CoWrapTrySend_
{
	template <typename... Args>
	void operator()(Args&&... msg)
	{
		_chan.try_send(std::forward<Args>(msg)...);
	}

	Chan& _chan;
};

template <typename Chan>
struct CoWrapTryPost_
{
	template <typename... Args>
	void operator()(Args&&... msg)
	{
		_chan.try_post(std::forward<Args>(msg)...);
	}

	Chan& _chan;
};

template <typename T>
struct CoChanMsgMove_
{
	typedef T type;
	static T&& forward(T& p) { return (T&&)p; }
	static T&& move(T& p) { return (T&&)p; }
};

template <typename T>
struct CoChanMsgMove_<T&>
{
	typedef T& type;
	static std::reference_wrapper<T> forward(T& p) { return std::reference_wrapper<T>(p); }
	static T& move(T& p) { return p; }
};

template <typename T>
struct CoChanMsgMove_<const T> : public CoChanMsgMove_<const T&> {};

template <typename T>
struct CoChanMsgMove_<T&&> {};

/*!
@brief 异步消息队列
*/
template <typename... Types>
class co_msg_buffer
{
	typedef std::tuple<TYPE_PIPE(Types)...> msg_type;
public:
	co_msg_buffer(const shared_strand& strand, size_t poolSize = sizeof(void*))
		:_closed(false), _strand(strand), _msgBuff(poolSize) {}

	~co_msg_buffer()
	{
		assert(_waitQueue.empty());
	}

	static std::shared_ptr<co_msg_buffer> make(const shared_strand& strand, size_t poolSize = sizeof(void*))
	{
		return std::make_shared<co_msg_buffer>(strand, poolSize);
	}
public:
	template <typename... Args>
	void send(Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_push(any_handler(), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this](RM_CREF(Args)&... msg)
			{
				_push(any_handler(), std::move(msg)...);
			}, std::forward<Args>(msg)...));
		}
	}

	template <typename... Args>
	void post(Args&&... msg)
	{
		_strand->try_tick(std::bind([this](RM_CREF(Args)&... msg)
		{
			_push(any_handler(), std::move(msg)...);
		}, std::forward<Args>(msg)...));
	}

	template <typename Notify, typename... Args>
	void push(Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void tick_push(Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void try_push(Notify&& ntf, Args&&... msg)
	{
		push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void aff_try_push(Notify&& ntf, Args&&... msg)
	{
		aff_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify>
	void pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_pop(std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_pop(CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void tick_pop(Notify&& ntf)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_pop(CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_pop(std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void try_pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_try_pop(std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_try_pop(CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void try_tick_pop(Notify&& ntf)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_try_pop(CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_try_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_try_pop(std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void timed_pop(int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_pop(ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_pop(ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_tick_pop(int ms, Notify&& ntf)
	{
		_strand->try_tick(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_timed_pop(ms, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_timed_pop(int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_timed_pop(ms, std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_pop(timer, ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_pop(timer, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_tick_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		_strand->try_tick(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_timed_pop(timer, ms, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_timed_pop(timer, ms, std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void append_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_append_pop_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_append_pop_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename CbNotify, typename MsgNotify>
	void try_pop_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_try_pop_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf)
			{
				_try_pop_and_append_notify(CoChanMsgMove_<CbNotify>::move(cb), CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign);
			}, CoChanMsgMove_<CbNotify>::forward(cb), std::forward<MsgNotify>(msgNtf)));
		}
	}

	template <typename Notify>
	void remove_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_remove_pop_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_remove_pop_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}


	template <typename Notify>
	void append_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_append_push_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_append_push_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename CbNotify, typename MsgNotify, typename... Args>
	void try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign, std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_try_push_and_append_notify(CoChanMsgMove_<CbNotify>::move(cb), CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign, CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<CbNotify>::forward(cb), std::forward<MsgNotify>(msgNtf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify>
	void remove_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_remove_push_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_remove_push_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void close()
	{
		_strand->distribute([this]()
		{
			_close();
		});
	}

	template <typename Notify>
	void close(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_close();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_close();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel()
	{
		_strand->distribute([this]()
		{
			_cancel();
		});
	}

	template <typename Notify>
	void cancel(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void reset()
	{
		assert(_closed);
		assert(_waitQueue.empty());
		assert(_msgBuff.empty());
		_closed = false;
	}

	const shared_strand& self_strand() const
	{
		return _strand;
	}

	CoOtherReceiver_<co_msg_buffer<Types...>> other_receiver()
	{
		return CoOtherReceiver_<co_msg_buffer<Types...>>{*this};
	}

	CoWrapSend_<co_msg_buffer<Types...>> wrap_send()
	{
		return CoWrapSend_<co_msg_buffer<Types...>>{*this};
	}

	CoWrapPost_<co_msg_buffer<Types...>> wrap_post()
	{
		return CoWrapPost_<co_msg_buffer<Types...>>{*this};
	}
private:
	template <typename Notify, typename... Args>
	void _push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		_msgBuff.push_back(std::forward<Args>(msg)...);
		if (!_waitQueue.empty())
		{
			assert(1 == _msgBuff.size());
			CoNotifyHandlerFace_* wtNtf = _waitQueue.front();
			_waitQueue.pop_front();
			wtNtf->invoke(_alloc);
		}
		CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
	}

	template <typename Notify>
	void _pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_msgBuff.empty())
		{
			msg_type msg(std::move(_msgBuff.front()));
			_msgBuff.pop_front();
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				if (co_async_state::co_async_ok == state)
				{
					assert(!_msgBuff.empty());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
		}
	}

	template <typename Notify>
	void _try_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_msgBuff.empty())
		{
			msg_type msg(std::move(_msgBuff.front()));
			_msgBuff.pop_front();
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _timed_pop(int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_msgBuff.empty())
		{
			msg_type msg(std::move(_msgBuff.front()));
			_msgBuff.pop_front();
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else if (ms > 0)
		{
			overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				_strand->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_alloc.deallocate(timer);
				if (co_async_state::co_async_ok == state)
				{
					assert(!_msgBuff.empty());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* popNtf = *it;
				_waitQueue.erase(it);
				popNtf->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_msgBuff.empty())
		{
			msg_type msg(std::move(_msgBuff.front()));
			_msgBuff.pop_front();
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else if (ms > 0)
		{
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				_strand->over_timer()->cancel(timer);
				if (co_async_state::co_async_ok == state)
				{
					assert(!_msgBuff.empty());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* popNtf = *it;
				_waitQueue.erase(it);
				popNtf->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _append_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_msgBuff.empty())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				assert(ntfSign._nodeEffect);
				ntfSign._nodeEffect = false;
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			ntfSign._ntfNode = --_waitQueue.end();
			ntfSign._nodeEffect = true;
		}
	}

	template <typename CbNotify, typename MsgNotify>
	void _try_pop_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(msgNtf, co_async_state::co_async_closed);
			CHECK_EXCEPTION(cb, co_async_state::co_async_closed);
			return;
		}
		if (!_msgBuff.empty())
		{
			msg_type msg(std::move(_msgBuff.front()));
			_msgBuff.pop_front();
			_append_pop_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(tuple_invoke, cb, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			_append_pop_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(cb, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _remove_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			assert(!ntfSign._nodeEffect);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		const bool effect = ntfSign._nodeEffect;
		ntfSign._nodeEffect = false;
		if (effect)
		{
			assert(ntfSign._appended);
			ntfSign._appended = false;
			CoNotifyHandlerFace_* popNtf = *ntfSign._ntfNode;
			_waitQueue.erase(ntfSign._ntfNode);
			popNtf->destroy();
			_alloc.deallocate(popNtf);
		}
		if (!_msgBuff.empty() && !_waitQueue.empty())
		{
			CoNotifyHandlerFace_* wtNtf = _waitQueue.front();
			_waitQueue.pop_front();
			wtNtf->invoke(_alloc);
		}
		CHECK_EXCEPTION(ntf, effect ? co_async_state::co_async_ok : co_async_state::co_async_fail);
	}

	template <typename Notify>
	void _append_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
	}

	template <typename CbNotify, typename MsgNotify, typename... Args>
	void _try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(msgNtf, co_async_state::co_async_closed);
			CHECK_EXCEPTION(cb, co_async_state::co_async_closed);
			return;
		}
		_msgBuff.push_back(std::forward<Args>(msg)...);
		if (!_waitQueue.empty())
		{
			assert(1 == _msgBuff.size());
			CoNotifyHandlerFace_* wtNtf = _waitQueue.front();
			_waitQueue.pop_front();
			wtNtf->invoke(_alloc);
		}
		_append_push_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
		CHECK_EXCEPTION(cb, co_async_state::co_async_ok);
	}

	template <typename Notify>
	void _remove_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
	}

	void _close()
	{
		assert(_strand->running_in_this_thread());
		_closed = true;
		_msgBuff.clear();
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_waitQueue.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _waitQueue.front();
			}
			else
			{
				ntfsEx.push_back(_waitQueue.front());
			}
			_waitQueue.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_closed);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_closed);
			ntfsEx.pop_front();
		}
	}

	void _cancel()
	{
		assert(_strand->running_in_this_thread());
		bool noEmpty = !_waitQueue.empty();
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_waitQueue.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _waitQueue.front();
			}
			else
			{
				ntfsEx.push_back(_waitQueue.front());
			}
			_waitQueue.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}
private:
	shared_strand _strand;
	msg_queue<msg_type> _msgBuff;
	reusable_mem _alloc;
	msg_list<CoNotifyHandlerFace_*> _waitQueue;
	bool _closed;
	NONE_COPY(co_msg_buffer);
};

template <>
class co_msg_buffer<void> : public co_msg_buffer<void_type>
{
public:
	co_msg_buffer(const shared_strand& strand, size_t = sizeof(void*))
		:co_msg_buffer<void_type>(strand) {}

	static std::shared_ptr<co_msg_buffer> make(const shared_strand& strand, size_t = sizeof(void*))
	{
		return std::make_shared<co_msg_buffer>(strand);
	}
public:
	template <typename Notify> void push(Notify&& ntf, void_type = void_type()){ co_msg_buffer<void_type>::push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_push(Notify&& ntf, void_type = void_type()){ co_msg_buffer<void_type>::aff_push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void try_push(Notify&& ntf, void_type = void_type()){ co_msg_buffer<void_type>::try_push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_try_push(Notify&& ntf, void_type = void_type()){ co_msg_buffer<void_type>::aff_try_push(std::forward<Notify>(ntf), void_type()); }
	template <typename CbNotify, typename MsgNotify, typename... Args> void try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg){
		co_msg_buffer<void_type>::try_push_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign, void_type());
	}
};

/*!
@brief 异步channel通信
*/
template <typename... Types>
class co_channel
{
	typedef std::tuple<TYPE_PIPE(Types)...> msg_type;
public:
	co_channel(const shared_strand& strand, size_t buffLength = 1)
		:_closed(false), _strand(strand), _buffer(buffLength) {}

	~co_channel()
	{
		assert(_pushWait.empty());
		assert(_popWait.empty());
	}

	static std::shared_ptr<co_channel> make(const shared_strand& strand, size_t buffLength = 1)
	{
		return std::make_shared<co_channel>(strand, buffLength);
	}
public:
	template <typename... Args>
	void try_send(Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push(any_handler(), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this](RM_CREF(Args)&... msg)
			{
				_try_push(any_handler(), std::move(msg)...);
			}, std::forward<Args>(msg)...));
		}
	}

	template <typename... Args>
	void try_post(Args&&... msg)
	{
		_strand->try_tick(std::bind([this](RM_CREF(Args)&... msg)
		{
			_try_push(any_handler(), std::move(msg)...);
		}, std::forward<Args>(msg)...));
	}

	template <typename Notify, typename... Args>
	void push(Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		} 
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void tick_push(Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void try_push(Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		} 
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_try_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void try_tick_push(Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_try_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_try_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_try_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_push(ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		} 
		else
		{
			_strand->post(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_timed_push(ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void timed_tick_push(int ms, Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_timed_push(ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_timed_push(ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_push(timer, ms, std::forward<Notify>(ntf), msg_type(std::forward<Args>(msg)...));
		}
		else
		{
			_strand->post(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_timed_push(timer, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void timed_tick_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_timed_push(timer, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_timed_push(timer, ms, std::forward<Notify>(ntf), msg_type(std::forward<Args>(msg)...));
	}

	template <typename Notify>
	void pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_pop(std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_pop(CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void tick_pop(Notify&& ntf)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_pop(CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_pop(std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void try_pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_try_pop(std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_try_pop(CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void try_tick_pop(Notify&& ntf)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_try_pop(CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_try_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_try_pop(std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void timed_pop(int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_pop(ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_pop(ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_tick_pop(int ms, Notify&& ntf)
	{
		_strand->try_tick(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_timed_pop(ms, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_timed_pop(int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_timed_pop(ms, std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_pop(timer, ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_pop(timer, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_tick_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		_strand->try_tick(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_timed_pop(timer, ms, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_timed_pop(timer, ms, std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void append_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_append_pop_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_append_pop_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename CbNotify, typename MsgNotify>
	void try_pop_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_try_pop_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf)
			{
				_try_pop_and_append_notify(CoChanMsgMove_<CbNotify>::move(cb), CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign);
			}, CoChanMsgMove_<CbNotify>::forward(cb), std::forward<MsgNotify>(msgNtf)));
		}
	}

	template <typename Notify>
	void remove_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_remove_pop_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_remove_pop_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void append_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_append_push_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_append_push_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename CbNotify, typename MsgNotify, typename... Args>
	void try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign, std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_try_push_and_append_notify(CoChanMsgMove_<CbNotify>::move(cb), CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign, CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<CbNotify>::forward(cb), std::forward<MsgNotify>(msgNtf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify>
	void remove_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_remove_push_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_remove_push_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void close()
	{
		_strand->distribute([this]()
		{
			_close();
		});
	}

	template <typename Notify>
	void close(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_close();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_close();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel()
	{
		_strand->distribute([this]()
		{
			_cancel();
		});
	}

	template <typename Notify>
	void cancel(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel_push()
	{
		_strand->distribute([this]()
		{
			_cancel_push();
		});
	}

	template <typename Notify>
	void cancel_push(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel_push();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel_push();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel_pop()
	{
		_strand->distribute([this]()
		{
			_cancel_pop();
		});
	}

	template <typename Notify>
	void cancel_pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel_pop();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel_pop();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void reset()
	{
		assert(_closed);
		assert(_pushWait.empty());
		assert(_popWait.empty());
		assert(_buffer.empty());
		_closed = false;
	}

	const shared_strand& self_strand() const
	{
		return _strand;
	}

	CoOtherReceiver_<co_channel<Types...>> other_receiver()
	{
		return CoOtherReceiver_<co_channel<Types...>>{*this};
	}

	CoWrapTrySend_<co_channel<Types...>> wrap_try_send()
	{
		return CoWrapTrySend_<co_channel<Types...>>{*this};
	}

	CoWrapTryPost_<co_channel<Types...>> wrap_try_post()
	{
		return CoWrapTryPost_<co_channel<Types...>>{*this};
	}
private:
	template <typename Notify, typename... Args>
	void _push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_buffer.full())
		{
			_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				if (co_async_state::co_async_ok == state)
				{
					assert(!_buffer.full());
					_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
		}
		else
		{
			_buffer.push_back(std::forward<Args>(msg)...);
			if (!_popWait.empty())
			{
				assert(1 == _buffer.size());
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
	}

	template <typename Notify, typename... Args>
	void _try_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_buffer.full())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
		else
		{
			_buffer.push_back(std::forward<Args>(msg)...);
			if (!_popWait.empty())
			{
				assert(1 == _buffer.size());
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
	}

	template <typename Notify, typename... Args>
	void _timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_buffer.full())
		{
			if (ms > 0)
			{
				overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
				_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
				{
					_strand->over_timer()->cancel(*timer);
					timer->~timer_handle();
					_alloc.deallocate(timer);
					if (co_async_state::co_async_ok == state)
					{
						assert(!_buffer.full());
						_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
					}
					else
					{
						CHECK_EXCEPTION(ntf, state);
					}
				}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
				_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
				{
					CoNotifyHandlerFace_* pushWait = *it;
					_pushWait.erase(it);
					pushWait->invoke(_alloc, co_async_state::co_async_overtime);
				}, --_pushWait.end())));
			}
			else
			{
				CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
			}
		}
		else
		{
			_buffer.push_back(std::forward<Args>(msg)...);
			if (!_popWait.empty())
			{
				assert(1 == _buffer.size());
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
	}

	template <typename Notify, typename... Args>
	void _timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_buffer.full())
		{
			if (ms > 0)
			{
				_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
				{
					_strand->over_timer()->cancel(timer);
					if (co_async_state::co_async_ok == state)
					{
						assert(!_buffer.full());
						_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
					}
					else
					{
						CHECK_EXCEPTION(ntf, state);
					}
				}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
				_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
				{
					CoNotifyHandlerFace_* pushWait = *it;
					_pushWait.erase(it);
					pushWait->invoke(_alloc, co_async_state::co_async_overtime);
				}, --_pushWait.end())));
			}
			else
			{
				CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
			}
		}
		else
		{
			_buffer.push_back(std::forward<Args>(msg)...);
			if (!_popWait.empty())
			{
				assert(1 == _buffer.size());
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
	}

	template <typename Notify>
	void _pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_buffer.empty())
		{
			msg_type msg(std::move(_buffer.front()));
			_buffer.pop_front();
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			_popWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				if (co_async_state::co_async_ok == state)
				{
					assert(!_buffer.empty());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
		}
	}

	template <typename Notify>
	void _try_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_buffer.empty())
		{
			msg_type msg(std::move(_buffer.front()));
			_buffer.pop_front();
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _timed_pop(int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_buffer.empty())
		{
			msg_type msg(std::move(_buffer.front()));
			_buffer.pop_front();
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else if (ms > 0)
		{
			overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_popWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				_strand->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_alloc.deallocate(timer);
				if (co_async_state::co_async_ok == state)
				{
					assert(!_buffer.empty());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* popWait = *it;
				_popWait.erase(it);
				popWait->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_popWait.end())));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_buffer.empty())
		{
			msg_type msg(std::move(_buffer.front()));
			_buffer.pop_front();
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else if (ms > 0)
		{
			_popWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				_strand->over_timer()->cancel(timer);
				if (co_async_state::co_async_ok == state)
				{
					assert(!_buffer.empty());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* popWait = *it;
				_popWait.erase(it);
				popWait->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_popWait.end())));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _append_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_buffer.empty())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			_popWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([&ntfSign](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				assert(ntfSign._nodeEffect);
				ntfSign._nodeEffect = false;
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			ntfSign._ntfNode = --_popWait.end();
			ntfSign._nodeEffect = true;
		}
	}

	template <typename CbNotify, typename MsgNotify>
	void _try_pop_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(msgNtf, co_async_state::co_async_closed);
			CHECK_EXCEPTION(cb, co_async_state::co_async_closed);
			return;
		}
		if (!_buffer.empty())
		{
			msg_type msg(std::move(_buffer.front()));
			_buffer.pop_front();
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
			_append_pop_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(tuple_invoke, cb, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			_append_pop_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(cb, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _remove_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			assert(!ntfSign._nodeEffect);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		const bool effect = ntfSign._nodeEffect;
		ntfSign._nodeEffect = false;
		if (effect)
		{
			assert(ntfSign._appended);
			ntfSign._appended = false;
			CoNotifyHandlerFace_* popNtf = *ntfSign._ntfNode;
			_popWait.erase(ntfSign._ntfNode);
			popNtf->destroy();
			_alloc.deallocate(popNtf);
		}
		if (!_buffer.empty() && !_popWait.empty())
		{
			CoNotifyHandlerFace_* popNtf = _popWait.front();
			_popWait.pop_front();
			popNtf->invoke(_alloc);
		}
		CHECK_EXCEPTION(ntf, effect ? co_async_state::co_async_ok : co_async_state::co_async_fail);
	}

	template <typename Notify>
	void _append_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_buffer.full())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([&ntfSign](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				assert(ntfSign._nodeEffect);
				ntfSign._nodeEffect = false;
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			ntfSign._ntfNode = --_pushWait.end();
			ntfSign._nodeEffect = true;
		}
	}

	template <typename CbNotify, typename MsgNotify, typename... Args>
	void _try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(msgNtf, co_async_state::co_async_closed);
			CHECK_EXCEPTION(cb, co_async_state::co_async_closed);
			return;
		}
		if (!_buffer.full())
		{
			_buffer.push_back(std::forward<Args>(msg)...);
			if (!_popWait.empty())
			{
				assert(1 == _buffer.size());
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
			_append_push_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(cb, co_async_state::co_async_ok);
		}
		else
		{
			_append_push_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(cb, co_async_state::co_async_fail);
		}
	}
	
	template <typename Notify>
	void _remove_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			assert(!ntfSign._nodeEffect);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		const bool effect = ntfSign._nodeEffect;
		ntfSign._nodeEffect = false;
		if (effect)
		{
			assert(ntfSign._appended);
			ntfSign._appended = false;
			CoNotifyHandlerFace_* pushNtf = *ntfSign._ntfNode;
			_pushWait.erase(ntfSign._ntfNode);
			pushNtf->destroy();
			_alloc.deallocate(pushNtf);
		}
		if (!_buffer.full() && !_pushWait.empty())
		{
			CoNotifyHandlerFace_* pushNtf = _pushWait.front();
			_pushWait.pop_front();
			pushNtf->invoke(_alloc);
		}
		CHECK_EXCEPTION(ntf, effect ? co_async_state::co_async_ok : co_async_state::co_async_fail);
	}

	void _close()
	{
		assert(_strand->running_in_this_thread());
		_closed = true;
		_buffer.clear();
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_pushWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _pushWait.front();
			}
			else
			{
				ntfsEx.push_back(_pushWait.front());
			}
			_pushWait.pop_front();
		}
		while (!_popWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _popWait.front();
			}
			else
			{
				ntfsEx.push_back(_popWait.front());
			}
			_popWait.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_closed);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_closed);
			ntfsEx.pop_front();
		}
	}

	void _cancel()
	{
		assert(_strand->running_in_this_thread());
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_pushWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _pushWait.front();
			}
			else
			{
				ntfsEx.push_back(_pushWait.front());
			}
			_pushWait.pop_front();
		}
		while (!_popWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _popWait.front();
			}
			else
			{
				ntfsEx.push_back(_popWait.front());
			}
			_popWait.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}

	void _cancel_push()
	{
		assert(_strand->running_in_this_thread());
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_pushWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _pushWait.front();
			}
			else
			{
				ntfsEx.push_back(_pushWait.front());
			}
			_pushWait.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}

	void _cancel_pop()
	{
		assert(_strand->running_in_this_thread());
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_popWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _popWait.front();
			}
			else
			{
				ntfsEx.push_back(_popWait.front());
			}
			_popWait.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}
private:
	shared_strand _strand;
	fixed_buffer<msg_type> _buffer;
	reusable_mem _alloc;
	msg_list<CoNotifyHandlerFace_*> _pushWait;
	msg_list<CoNotifyHandlerFace_*> _popWait;
	bool _closed;
	NONE_COPY(co_channel);
};

template <>
class co_channel<void> : public co_channel<void_type>
{
public:
	co_channel(const shared_strand& strand, size_t buffLength = 1)
		:co_channel<void_type>(strand, buffLength) {}

	static std::shared_ptr<co_channel> make(const shared_strand& strand, size_t buffLength = 1)
	{
		return std::make_shared<co_channel>(strand, buffLength);
	}
public:
	template <typename Notify> void push(Notify&& ntf, void_type = void_type()){ co_channel<void_type>::push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_push(Notify&& ntf, void_type = void_type()){ co_channel<void_type>::aff_push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void try_push(Notify&& ntf, void_type = void_type()){ co_channel<void_type>::try_push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_try_push(Notify&& ntf, void_type = void_type()){ co_channel<void_type>::aff_try_push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void timed_push(int ms, Notify&& ntf, void_type = void_type()){ co_channel<void_type>::timed_push(ms, std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_timed_push(int ms, Notify&& ntf, void_type = void_type()){ co_channel<void_type>::aff_timed_push(ms, std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, void_type = void_type()){ co_channel<void_type>::timed_push(timer, ms, std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, void_type = void_type()){ co_channel<void_type>::aff_timed_push(timer, ms, std::forward<Notify>(ntf), void_type()); }
	template <typename CbNotify, typename MsgNotify, typename... Args> void try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg){
		co_channel<void_type>::try_push_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign, void_type());
	}
};

/*!
@brief 异步无缓冲channel通信
*/
template <typename... Types>
class co_nil_channel
{
	typedef std::tuple<TYPE_PIPE(Types)...> msg_type;
public:
	co_nil_channel(const shared_strand& strand)
		:_closed(false), _msgIsTryPush(false), _strand(strand) {}

	~co_nil_channel()
	{
		assert(_pushWait.empty());
		assert(_popWait.empty());
	}

	static std::shared_ptr<co_nil_channel> make(const shared_strand& strand)
	{
		return std::make_shared<co_nil_channel>(strand);
	}
public:
	template <typename... Args>
	void try_send(Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push(any_handler(), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this](RM_CREF(Args)&... msg)
			{
				_try_push(any_handler(), std::move(msg)...);
			}, std::forward<Args>(msg)...));
		}
	}

	template <typename... Args>
	void try_post(Args&&... msg)
	{
		_strand->try_tick(std::bind([this](RM_CREF(Args)&... msg)
		{
			_try_push(any_handler(), std::move(msg)...);
		}, std::forward<Args>(msg)...));
	}

	template <typename Notify, typename... Args>
	void push(Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void tick_push(Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void try_push(Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_try_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void try_tick_push(Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_try_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_try_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_try_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_push(ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_timed_push(ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void timed_tick_push(int ms, Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_timed_push(ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_timed_push(ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_push(timer, ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_timed_push(timer, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void timed_tick_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_timed_push(timer, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_timed_push(timer, ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify>
	void pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_pop(std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_pop(CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void tick_pop(Notify&& ntf)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_pop(CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_pop(std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void try_pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_try_pop(std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_try_pop(CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void aff_try_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_try_pop(std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void timed_pop(int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_pop(ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_pop(ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_tick_pop(int ms, Notify&& ntf)
	{
		_strand->try_tick(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_timed_pop(ms, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_timed_pop(int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_timed_pop(ms, std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_pop(timer, ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_pop(timer, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_tick_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		_strand->try_tick(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_timed_pop(timer, ms, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_timed_pop(timer, ms, std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void append_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_append_pop_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_append_pop_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename CbNotify, typename MsgNotify>
	void try_pop_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_try_pop_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf)
			{
				_try_pop_and_append_notify(CoChanMsgMove_<CbNotify>::move(cb), CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign);
			}, CoChanMsgMove_<CbNotify>::forward(cb), std::forward<MsgNotify>(msgNtf)));
		}
	}

	template <typename Notify>
	void remove_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_remove_pop_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_remove_pop_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void append_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_append_push_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_append_push_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename CbNotify, typename MsgNotify, typename... Args>
	void try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign, std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_try_push_and_append_notify(CoChanMsgMove_<CbNotify>::move(cb), CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign, CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<CbNotify>::forward(cb), std::forward<MsgNotify>(msgNtf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify>
	void remove_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_remove_push_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_remove_push_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void close()
	{
		_strand->distribute([this]()
		{
			_close();
		});
	}

	template <typename Notify>
	void close(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_close();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_close();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel()
	{
		_strand->distribute([this]()
		{
			_cancel();
		});
	}

	template <typename Notify>
	void cancel(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel_push()
	{
		_strand->distribute([this]()
		{
			_cancel_push();
		});
	}

	template <typename Notify>
	void cancel_push(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel_push();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel_push();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel_pop()
	{
		_strand->distribute([this]()
		{
			_cancel_pop();
		});
	}

	template <typename Notify>
	void cancel_pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel_pop();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel_pop();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void reset()
	{
		assert(_closed);
		assert(_pushWait.empty());
		assert(_popWait.empty());
		assert(!_tempBuffer.has());
		_closed = false;
	}

	const shared_strand& self_strand() const
	{
		return _strand;
	}

	CoOtherReceiver_<co_nil_channel<Types...>> other_receiver()
	{
		return CoOtherReceiver_<co_nil_channel<Types...>>{*this};
	}

	CoWrapTrySend_<co_nil_channel<Types...>> wrap_try_send()
	{
		return CoWrapTrySend_<co_nil_channel<Types...>>{*this};
	}

	CoWrapTryPost_<co_nil_channel<Types...>> wrap_try_post()
	{
		return CoWrapTryPost_<co_nil_channel<Types...>>{*this};
	}
private:
	template <typename Notify, typename... Args>
	void _push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				if (co_async_state::co_async_ok == state)
				{
					assert(!_tempBuffer.has());
					_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
		}
		else
		{
			_msgIsTryPush = false;
			_tempBuffer.create(std::forward<Args>(msg)...);
			_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				if (co_async_state::co_async_closed != state && co_async_state::co_async_cancel != state && !_pushWait.empty())
				{
					CoNotifyHandlerFace_* pushNtf = _pushWait.front();
					_pushWait.pop_front();
					pushNtf->invoke(_alloc);
				}
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			if (!_popWait.empty())
			{
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
		}
	}

	template <typename Notify, typename... Args>
	void _try_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
		else
		{
			if (!_popWait.empty())
			{
				_msgIsTryPush = true;
				_tempBuffer.create(std::forward<Args>(msg)...);
				_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
				{
					if (co_async_state::co_async_closed != state && co_async_state::co_async_cancel != state && !_pushWait.empty())
					{
						CoNotifyHandlerFace_* pushNtf = _pushWait.front();
						_pushWait.pop_front();
						pushNtf->invoke(_alloc);
					}
					CHECK_EXCEPTION(ntf, state);
				}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
			else
			{
				CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
			}
		}
	}

	template <typename Notify, typename... Args>
	void _timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			if (ms > 0)
			{
				overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
				_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
				{
					_strand->over_timer()->cancel(*timer);
					timer->~timer_handle();
					_alloc.deallocate(timer);
					if (co_async_state::co_async_ok == state)
					{
						assert(!_tempBuffer.has());
						_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
					}
					else
					{
						CHECK_EXCEPTION(ntf, state);
					}
				}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
				_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
				{
					CoNotifyHandlerFace_* pushWait = *it;
					_pushWait.erase(it);
					pushWait->invoke(_alloc, co_async_state::co_async_overtime);
				}, --_pushWait.end())));
			}
			else
			{
				CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
			}
		}
		else
		{
			_msgIsTryPush = false;
			_tempBuffer.create(std::forward<Args>(msg)...);
			overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				_strand->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_alloc.deallocate(timer);
				if (co_async_state::co_async_closed != state && co_async_state::co_async_cancel != state && !_pushWait.empty())
				{
					CoNotifyHandlerFace_* pushNtf = _pushWait.front();
					_pushWait.pop_front();
					pushNtf->invoke(_alloc);
				}
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* pushWait = *it;
				_pushWait.erase(it);
				pushWait->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_pushWait.end())));
			if (!_popWait.empty())
			{
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
		}
	}

	template <typename Notify, typename... Args>
	void _timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			if (ms > 0)
			{
				_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
				{
					_strand->over_timer()->cancel(timer);
					if (co_async_state::co_async_ok == state)
					{
						assert(!_tempBuffer.has());
						_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
					}
					else
					{
						CHECK_EXCEPTION(ntf, state);
					}
				}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
				_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
				{
					CoNotifyHandlerFace_* pushWait = *it;
					_pushWait.erase(it);
					pushWait->invoke(_alloc, co_async_state::co_async_overtime);
				}, --_pushWait.end())));
			}
			else
			{
				CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
			}
		}
		else
		{
			_msgIsTryPush = false;
			_tempBuffer.create(std::forward<Args>(msg)...);
			_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				_strand->over_timer()->cancel(timer);
				if (co_async_state::co_async_closed != state && co_async_state::co_async_cancel != state && !_pushWait.empty())
				{
					CoNotifyHandlerFace_* pushNtf = _pushWait.front();
					_pushWait.pop_front();
					pushNtf->invoke(_alloc);
				}
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* pushWait = *it;
				_pushWait.erase(it);
				pushWait->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_pushWait.end())));
			if (!_popWait.empty())
			{
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
		}
	}

	template <typename Notify>
	void _pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			assert(!_pushWait.empty());
			msg_type msg(std::move(_tempBuffer.get()));
			_tempBuffer.destroy();
			CoNotifyHandlerFace_* pushNtf = _pushWait.front();
			_pushWait.pop_front();
			pushNtf->invoke(_alloc);
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			_popWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				if (co_async_state::co_async_ok == state)
				{
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
		}
	}
	
	template <typename Notify>
	void _try_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			assert(!_pushWait.empty());
			msg_type msg(std::move(_tempBuffer.get()));
			_tempBuffer.destroy();
			CoNotifyHandlerFace_* pushNtf = _pushWait.front();
			_pushWait.pop_front();
			pushNtf->invoke(_alloc);
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _timed_pop(int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			assert(!_pushWait.empty());
			msg_type msg(std::move(_tempBuffer.get()));
			_tempBuffer.destroy();
			CoNotifyHandlerFace_* pushNtf = _pushWait.front();
			_pushWait.pop_front();
			pushNtf->invoke(_alloc);
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else if (ms > 0)
		{
			overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_popWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				_strand->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_alloc.deallocate(timer);
				if (co_async_state::co_async_ok == state)
				{
					assert(_tempBuffer.has());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* popWait = *it;
				_popWait.erase(it);
				popWait->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_popWait.end())));
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			assert(!_pushWait.empty());
			msg_type msg(std::move(_tempBuffer.get()));
			_tempBuffer.destroy();
			CoNotifyHandlerFace_* pushNtf = _pushWait.front();
			_pushWait.pop_front();
			pushNtf->invoke(_alloc);
			CHECK_EXCEPTION(tuple_invoke, ntf, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else if (ms > 0)
		{
			_popWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				_strand->over_timer()->cancel(timer);
				if (co_async_state::co_async_ok == state)
				{
					assert(_tempBuffer.has());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* popWait = *it;
				_popWait.erase(it);
				popWait->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_popWait.end())));
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _append_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			_popWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([&ntfSign](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				assert(ntfSign._nodeEffect);
				ntfSign._nodeEffect = false;
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			ntfSign._ntfNode = --_popWait.end();
			ntfSign._nodeEffect = true;
			if (!_pushWait.empty())
			{
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc);
			}
		}
	}

	template <typename CbNotify, typename MsgNotify>
	void _try_pop_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(msgNtf, co_async_state::co_async_closed);
			CHECK_EXCEPTION(cb, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			assert(!_pushWait.empty());
			msg_type msg(std::move(_tempBuffer.get()));
			_tempBuffer.destroy();
			CoNotifyHandlerFace_* pushNtf = _pushWait.front();
			_pushWait.pop_front();
			pushNtf->invoke(_alloc);
			_append_pop_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(tuple_invoke, cb, std::tuple<co_async_state>(co_async_state::co_async_ok), std::move(msg));
		}
		else
		{
			_append_pop_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(cb, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _remove_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			assert(!ntfSign._nodeEffect);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		const bool effect = ntfSign._nodeEffect;
		ntfSign._nodeEffect = false;
		if (effect)
		{
			assert(ntfSign._appended);
			ntfSign._appended = false;
			CoNotifyHandlerFace_* popNtf = *ntfSign._ntfNode;
			_popWait.erase(ntfSign._ntfNode);
			popNtf->destroy();
			_alloc.deallocate(popNtf);
		}
		if (_tempBuffer.has())
		{
			if (!_popWait.empty())
			{
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
			}
			else if (_msgIsTryPush)
			{
				_tempBuffer.destroy();
				assert(!_pushWait.empty());
				CoNotifyHandlerFace_* pushNtf = _pushWait.front();
				_pushWait.pop_front();
				pushNtf->invoke(_alloc, co_async_state::co_async_fail);
			}
		}
		CHECK_EXCEPTION(ntf, effect ? co_async_state::co_async_ok : co_async_state::co_async_fail);
	}

	template <typename Notify>
	void _append_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_popWait.empty())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			assert(!_tempBuffer.has());
			_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([&ntfSign](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				assert(ntfSign._nodeEffect);
				ntfSign._nodeEffect = false;
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			ntfSign._ntfNode = --_pushWait.end();
			ntfSign._nodeEffect = true;
		}
	}

	template <typename CbNotify, typename MsgNotify, typename... Args>
	void _try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(msgNtf, co_async_state::co_async_closed);
			CHECK_EXCEPTION(cb, co_async_state::co_async_closed);
			return;
		}
		if (!_tempBuffer.has())
		{
			if (!_popWait.empty())
			{
				_msgIsTryPush = true;
				_tempBuffer.create(std::forward<Args>(msg)...);
				_pushWait.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf, co_async_state state)
				{
					if (co_async_state::co_async_closed != state && co_async_state::co_async_cancel != state && !_pushWait.empty())
					{
						CoNotifyHandlerFace_* pushNtf = _pushWait.front();
						_pushWait.pop_front();
						pushNtf->invoke(_alloc);
					}
					_append_push_notify(CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign);
					CHECK_EXCEPTION(cb, state);
				}, CoChanMsgMove_<CbNotify>::forward(cb), CoChanMsgMove_<MsgNotify>::forward(msgNtf), __1)));
				CoNotifyHandlerFace_* popNtf = _popWait.front();
				_popWait.pop_front();
				popNtf->invoke(_alloc);
				return;
			}
		}
		assert(_popWait.empty());
		_append_push_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
		CHECK_EXCEPTION(cb, co_async_state::co_async_fail);
	}

	template <typename Notify>
	void _remove_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			assert(!ntfSign._nodeEffect);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		const bool effect = ntfSign._nodeEffect;
		ntfSign._nodeEffect = false;
		if (effect)
		{
			assert(ntfSign._appended);
			ntfSign._appended = false;
			CoNotifyHandlerFace_* pushNtf = *ntfSign._ntfNode;
			_pushWait.erase(ntfSign._ntfNode);
			pushNtf->destroy();
			_alloc.deallocate(pushNtf);
		}
		if (!_tempBuffer.has() && !_pushWait.empty())
		{
			CoNotifyHandlerFace_* pushNtf = _pushWait.front();
			_pushWait.pop_front();
			pushNtf->invoke(_alloc);
		}
		CHECK_EXCEPTION(ntf, effect ? co_async_state::co_async_ok : co_async_state::co_async_fail);
	}

	void _close()
	{
		assert(_strand->running_in_this_thread());
		_closed = true;
		_tempBuffer.destroy();
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_pushWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _pushWait.front();
			}
			else
			{
				ntfsEx.push_back(_pushWait.front());
			}
			_pushWait.pop_front();
		}
		while (!_popWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _popWait.front();
			}
			else
			{
				ntfsEx.push_back(_popWait.front());
			}
			_popWait.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_closed);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_closed);
			ntfsEx.pop_front();
		}
	}

	void _cancel()
	{
		assert(_strand->running_in_this_thread());
		size_t ntfNum = 0;
		_tempBuffer.destroy();
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_pushWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _pushWait.front();
			}
			else
			{
				ntfsEx.push_back(_pushWait.front());
			}
			_pushWait.pop_front();
		}
		while (!_popWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _popWait.front();
			}
			else
			{
				ntfsEx.push_back(_popWait.front());
			}
			_popWait.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}

	void _cancel_push()
	{
		assert(_strand->running_in_this_thread());
		size_t ntfNum = 0;
		_tempBuffer.destroy();
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_pushWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _pushWait.front();
			}
			else
			{
				ntfsEx.push_back(_pushWait.front());
			}
			_pushWait.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}

	void _cancel_pop()
	{
		assert(_strand->running_in_this_thread());
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_popWait.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _popWait.front();
			}
			else
			{
				ntfsEx.push_back(_popWait.front());
			}
			_popWait.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}
private:
	shared_strand _strand;
	stack_obj<msg_type> _tempBuffer;
	reusable_mem _alloc;
	msg_list<CoNotifyHandlerFace_*> _pushWait;
	msg_list<CoNotifyHandlerFace_*> _popWait;
	bool _msgIsTryPush;
	bool _closed;
	NONE_COPY(co_nil_channel);
};

template <>
class co_nil_channel<void> : public co_nil_channel<void_type>
{
public:
	co_nil_channel(const shared_strand& strand)
		:co_nil_channel<void_type>(strand) {}

	static std::shared_ptr<co_nil_channel> make(const shared_strand& strand)
	{
		return std::make_shared<co_nil_channel>(strand);
	}
public:
	template <typename Notify> void push(Notify&& ntf, void_type = void_type()){ co_nil_channel<void_type>::push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_push(Notify&& ntf, void_type = void_type()){ co_nil_channel<void_type>::aff_push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void try_push(Notify&& ntf, void_type = void_type()){ co_nil_channel<void_type>::try_push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_try_push(Notify&& ntf, void_type = void_type()){ co_nil_channel<void_type>::aff_try_push(std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void timed_push(int ms, Notify&& ntf, void_type = void_type()){ co_nil_channel<void_type>::timed_push(ms, std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_timed_push(int ms, Notify&& ntf, void_type = void_type()){ co_nil_channel<void_type>::aff_timed_push(ms, std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, void_type = void_type()){ co_nil_channel<void_type>::timed_push(timer, ms, std::forward<Notify>(ntf), void_type()); }
	template <typename Notify> void aff_timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, void_type = void_type()){ co_nil_channel<void_type>::aff_timed_push(timer, ms, std::forward<Notify>(ntf), void_type()); }
	template <typename CbNotify, typename MsgNotify, typename... Args> void try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg){
		co_nil_channel<void_type>::try_push_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign, void_type());
	}
};

template <typename R>
struct ResultNotifyFace_
{
	virtual void invoke(co_async_state, R) = 0;
	virtual void invoke(co_async_state) = 0;
};

template <typename R, typename Notify>
struct ResultNotify_ : public ResultNotifyFace_<R>
{
	typedef RM_CREF(Notify) notify_type;

	ResultNotify_(reusable_mem& alloc, Notify& ntf)
		:_alloc(&alloc), _ntf(std::forward<Notify>(ntf)) {}

	void invoke(co_async_state state, R res)
	{
		reusable_mem* const alloc = _alloc;
		Notify ntf = std::move(_ntf);
		this->~ResultNotify_();
		alloc->deallocate(this);
		CHECK_EXCEPTION(ntf, state, std::forward<R>(res));
	}

	void invoke(co_async_state state)
	{
		reusable_mem* const alloc = _alloc;
		Notify ntf = std::move(_ntf);
		this->~ResultNotify_();
		alloc->deallocate(this);
		CHECK_EXCEPTION(ntf, state);
	}

	notify_type _ntf;
	reusable_mem* _alloc;
	RVALUE_CONSTRUCT2(ResultNotify_, _ntf, _alloc);
};

#if (_DEBUG || DEBUG)
struct CspResultSign_
{
	CspResultSign_() :sign(false){}
	~CspResultSign_(){ assert(sign); }
	bool exchange(bool b){ return sign.exchange(b); }
	operator bool(){ return sign; }
	std::atomic<bool> sign;
};
#endif

/*!
@brief co_csp_channel调用返回结果
*/
template <typename R>
struct csp_result
{
	csp_result()
	:_notify(NULL) {}

	template <typename Arg>
	void return_(Arg&& arg)
	{
		assert(_notify && _ntfSign && !_ntfSign->exchange(true));
		_notify->invoke(co_async_state::co_async_ok, std::forward<Arg>(arg));
		_notify = NULL;
	}

	void return_()
	{
		assert(_notify && _ntfSign && !_ntfSign->exchange(true));
		_notify->invoke(co_async_state::co_async_ok);
		_notify = NULL;
	}
private:
	template <typename...> friend class co_csp_channel;
	csp_result(ResultNotifyFace_<R>* notify)
		:_notify(notify)
	{
		DEBUG_OPERATION(_ntfSign = std::shared_ptr<CspResultSign_>(new(malloc(sizeof(CspResultSign_)))CspResultSign_(), [](CspResultSign_* sign){sign->~CspResultSign_(); free(sign); }));
	}
protected:
	ResultNotifyFace_<R>* _notify;
	DEBUG_OPERATION(std::shared_ptr<CspResultSign_> _ntfSign);
};

template <>
struct csp_result<void_type> : public csp_result<void_type1>
{
	csp_result() :csp_result<void_type1>() {}
	csp_result(const csp_result<void_type1>& s) :csp_result<void_type1>(s){}
	void operator=(const csp_result<void_type1>& s) { csp_result<void_type1>::operator =(s); }
};

template <>
struct csp_result<void> : public csp_result<void_type1>
{
	csp_result() :csp_result<void_type1>() {}
	csp_result(const csp_result<void_type1>& s) :csp_result<void_type1>(s){}
	void operator=(const csp_result<void_type1>& s) { csp_result<void_type1>::operator =(s); }
};

/*!
@brief 带返回值的channel
*/
template <typename... Types> class co_csp_channel{};
template <typename R, typename... Types>
class co_csp_channel<R(Types...)>
{
	typedef std::tuple<TYPE_PIPE(Types)...> msg_type;

	struct send_pck
	{
		send_pck(ResultNotifyFace_<R>* ntf, msg_type&& msg, overlap_timer::timer_handle* timer = NULL)
		:_ntf(ntf), _msg(std::move(msg)), _timer(timer) {}

		void cancel_timer(shared_strand& strand, reusable_mem& alloc)
		{
			if (_timer)
			{
				if (0 != ((size_t)_timer & 0x1))
				{
					strand->over_timer()->cancel(*(overlap_timer::timer_handle*)((size_t)_timer & -2));
				}
				else
				{
					strand->over_timer()->cancel(*_timer);
					_timer->~timer_handle();
					alloc.deallocate(_timer);
				}
			}
		}

		ResultNotifyFace_<R>* _ntf;
		msg_type _msg;
		overlap_timer::timer_handle* _timer;
		RVALUE_CONSTRUCT3(send_pck, _ntf, _msg, _timer);
	};
public:
	co_csp_channel(const shared_strand& strand)
		:_closed(false), _msgIsTryPush(false), _strand(strand) {}

	~co_csp_channel()
	{
		assert(!_tempBuffer.has());
		assert(_sendQueue.empty());
		assert(_waitQueue.empty());
	}

	static std::shared_ptr<co_csp_channel> make(const shared_strand& strand)
	{
		return std::make_shared<co_csp_channel>(strand);
	}
public:
	template <typename Notify, typename... Args>
	void push(Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void tick_push(Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void try_push(Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_try_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void try_tick_push(Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_try_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_try_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_try_push(std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_push(ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_timed_push(ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void timed_tick_push(int ms, Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_timed_push(ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_timed_push(ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify, typename... Args>
	void timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_push(timer, ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_timed_push(timer, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify, typename... Args>
	void timed_tick_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		_strand->try_tick(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
		{
			_timed_push(timer, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
		}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...));
	}

	template <typename Notify, typename... Args>
	void aff_timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		_timed_push(timer, ms, std::forward<Notify>(ntf), std::forward<Args>(msg)...);
	}

	template <typename Notify>
	void pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_pop(std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_pop(CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void tick_pop(Notify&& ntf)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_pop(CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_pop(std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void try_pop(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_try_pop(std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_try_pop(CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void try_tick_pop(Notify&& ntf)
	{
		_strand->try_tick(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_try_pop(CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_try_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_try_pop(std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void timed_pop(int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_pop(ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_pop(ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_tick_pop(int ms, Notify&& ntf)
	{
		_strand->try_tick(std::bind([this, ms](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_timed_pop(ms, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_timed_pop(int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_timed_pop(ms, std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_pop(timer, ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_pop(timer, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_tick_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		_strand->try_tick(std::bind([this, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_timed_pop(timer, ms, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void aff_timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_timed_pop(timer, ms, std::forward<Notify>(ntf));
	}

	template <typename Notify>
	void append_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_append_pop_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_append_pop_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename CbNotify, typename MsgNotify>
	void try_pop_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_try_pop_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf)
			{
				_try_pop_and_append_notify(CoChanMsgMove_<CbNotify>::move(cb), CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign);
			}, CoChanMsgMove_<CbNotify>::forward(cb), std::forward<MsgNotify>(msgNtf)));
		}
	}

	template <typename Notify>
	void remove_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_remove_pop_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_remove_pop_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void append_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_append_push_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_append_push_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename CbNotify, typename MsgNotify, typename... Args>
	void try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg)
	{
		if (_strand->running_in_this_thread())
		{
			_try_push_and_append_notify(std::forward<CbNotify>(cb), std::forward<MsgNotify>(msgNtf), ntfSign, std::forward<Args>(msg)...);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<CbNotify>::type& cb, typename CoChanMsgMove_<MsgNotify>::type& msgNtf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				_try_push_and_append_notify(CoChanMsgMove_<CbNotify>::move(cb), CoChanMsgMove_<MsgNotify>::move(msgNtf), ntfSign, CoChanMsgMove_<Args>::move(msg)...);
			}, CoChanMsgMove_<CbNotify>::forward(cb), std::forward<MsgNotify>(msgNtf), CoChanMsgMove_<Args>::forward(msg)...));
		}
	}

	template <typename Notify>
	void remove_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		if (_strand->running_in_this_thread())
		{
			_remove_push_notify(std::forward<Notify>(ntf), ntfSign);
		}
		else
		{
			_strand->post(std::bind([this, &ntfSign](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_remove_push_notify(CoChanMsgMove_<Notify>::move(ntf), ntfSign);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void close()
	{
		_strand->distribute([this]()
		{
			_close();
		});
	}

	template <typename Notify>
	void close(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_close();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_close();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel()
	{
		_strand->distribute([this]()
		{
			_cancel();
		});
	}

	template <typename Notify>
	void cancel(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel_wait()
	{
		_strand->distribute([this]()
		{
			_cancel_wait();
		});
	}

	template <typename Notify>
	void cancel_wait(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel_wait();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel_wait();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void cancel_send()
	{
		_strand->distribute([this]()
		{
			_cancel_send();
		});
	}

	template <typename Notify>
	void cancel_send(Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_cancel_send();
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_strand->post(std::bind([this](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_cancel_send();
				CHECK_EXCEPTION(ntf);
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void reset()
	{
		assert(_closed);
		assert(_sendQueue.empty());
		assert(_waitQueue.empty());
		_closed = false;
	}

	const shared_strand& self_strand() const
	{
		return _strand;
	}
private:
	template <typename Notify>
	ResultNotifyFace_<R>* wrap_result_notify(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		typedef ResultNotify_<R, Notify> Notify_;
		return new(_alloc.allocate(sizeof(Notify_)))Notify_(_alloc, ntf);
	}

	template <typename Notify, typename... Args>
	void _push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			_sendQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
			{
				if (co_async_state::co_async_ok == state)
				{
					assert(!_tempBuffer.has());
					_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
		}
		else
		{
			_msgIsTryPush = false;
			_tempBuffer.create(wrap_result_notify(CoChanMsgMove_<Notify>::forward(ntf)), msg_type(std::forward<Args>(msg)...));
			if (!_waitQueue.empty())
			{
				CoNotifyHandlerFace_* handler = _waitQueue.front();
				_waitQueue.pop_front();
				handler->invoke(_alloc);
			}
		}
	}

	template <typename Notify, typename... Args>
	void _try_push(Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
		else
		{
			if (!_waitQueue.empty())
			{
				_msgIsTryPush = true;
				_tempBuffer.create(wrap_result_notify(CoChanMsgMove_<Notify>::forward(ntf)), msg_type(std::forward<Args>(msg)...));
				CoNotifyHandlerFace_* handler = _waitQueue.front();
				_waitQueue.pop_front();
				handler->invoke(_alloc);
			}
			else
			{
				CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
			}
		}
	}

	template <typename Notify, typename... Args>
	void _timed_push(int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			if (ms > 0)
			{
				overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
				_sendQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
				{
					_strand->over_timer()->cancel(*timer);
					timer->~timer_handle();
					_alloc.deallocate(timer);
					if (co_async_state::co_async_ok == state)
					{
						assert(!_tempBuffer.has());
						_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
					}
					else
					{
						CHECK_EXCEPTION(ntf, state);
					}
				}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
				_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
				{
					CoNotifyHandlerFace_* sendWait = *it;
					_sendQueue.erase(it);
					sendWait->invoke(_alloc, co_async_state::co_async_overtime);
				}, --_sendQueue.end())));
			}
			else
			{
				CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
			}
		}
		else
		{
			_msgIsTryPush = false;
			overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_tempBuffer.create(wrap_result_notify(CoChanMsgMove_<Notify>::forward(ntf)), msg_type(std::forward<Args>(msg)...), timer);
			_strand->over_timer()->timeout(ms, *timer, [this]()
			{
				_tempBuffer->cancel_timer(_strand, _alloc);
				ResultNotifyFace_<R>* ntf_ = _tempBuffer->_ntf;
				_tempBuffer.destroy();
				ntf_->invoke(co_async_state::co_async_overtime);
			});
			if (!_waitQueue.empty())
			{
				CoNotifyHandlerFace_* handler = _waitQueue.front();
				_waitQueue.pop_front();
				handler->invoke(_alloc);
			}
		}
	}

	template <typename Notify, typename... Args>
	void _timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			if (ms > 0)
			{
				_sendQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<Args>::type&... msg)
				{
					_strand->over_timer()->cancel(timer);
					if (co_async_state::co_async_ok == state)
					{
						assert(!_tempBuffer.has());
						_push(CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<Args>::move(msg)...);
					}
					else
					{
						CHECK_EXCEPTION(ntf, state);
					}
				}, __1, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<Args>::forward(msg)...)));
				_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
				{
					CoNotifyHandlerFace_* sendWait = *it;
					_sendQueue.erase(it);
					sendWait->invoke(_alloc, co_async_state::co_async_overtime);
				}, --_sendQueue.end())));
			}
			else
			{
				CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
			}
		}
		else
		{
			_msgIsTryPush = false;
			_tempBuffer.create(wrap_result_notify(CoChanMsgMove_<Notify>::forward(ntf)), msg_type(std::forward<Args>(msg)...), (overlap_timer::timer_handle*)((size_t)&timer | 0x1));
			_strand->over_timer()->timeout(ms, timer, [this]()
			{
				_tempBuffer->cancel_timer(_strand, _alloc);
				ResultNotifyFace_<R>* ntf_ = _tempBuffer->_ntf;
				_tempBuffer.destroy();
				ntf_->invoke(co_async_state::co_async_overtime);
			});
			if (!_waitQueue.empty())
			{
				CoNotifyHandlerFace_* handler = _waitQueue.front();
				_waitQueue.pop_front();
				handler->invoke(_alloc);
			}
		}
	}

	template <typename Notify>
	void _pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			send_pck pck = std::move(_tempBuffer.get());
			pck.cancel_timer(_strand, _alloc);
			_tempBuffer.destroy();
			if (!_sendQueue.empty())
			{
				CoNotifyHandlerFace_* sendWait = _sendQueue.front();
				_sendQueue.pop_front();
				sendWait->invoke(_alloc);
			}
			CHECK_EXCEPTION(tuple_invoke, ntf, std::make_tuple(co_async_state::co_async_ok, csp_result<R>(pck._ntf)), std::move(pck._msg));
		}
		else
		{
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				if (co_async_state::co_async_ok == state)
				{
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))));
			if (!_sendQueue.empty())
			{
				CoNotifyHandlerFace_* sendWait = _sendQueue.front();
				_sendQueue.pop_front();
				sendWait->invoke(_alloc);
			}
		}
	}

	template <typename Notify>
	void _try_pop(Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			send_pck pck = std::move(_tempBuffer.get());
			pck.cancel_timer(_strand, _alloc);
			_tempBuffer.destroy();
			if (!_sendQueue.empty())
			{
				CoNotifyHandlerFace_* sendWait = _sendQueue.front();
				_sendQueue.pop_front();
				sendWait->invoke(_alloc);
			}
			CHECK_EXCEPTION(tuple_invoke, ntf, std::make_tuple(co_async_state::co_async_ok, csp_result<R>(pck._ntf)), std::move(pck._msg));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _timed_pop(int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			send_pck pck = std::move(_tempBuffer.get());
			pck.cancel_timer(_strand, _alloc);
			_tempBuffer.destroy();
			if (!_sendQueue.empty())
			{
				CoNotifyHandlerFace_* sendWait = _sendQueue.front();
				_sendQueue.pop_front();
				sendWait->invoke(_alloc);
			}
			CHECK_EXCEPTION(tuple_invoke, ntf, std::make_tuple(co_async_state::co_async_ok, csp_result<R>(pck._ntf)), std::move(pck._msg));
		}
		else if (ms > 0)
		{
			overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_strand->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_alloc.deallocate(timer);
				if (co_async_state::co_async_ok == state)
				{
					assert(!_sendQueue.empty());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))));
			_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* waitNtf = *it;
				_waitQueue.erase(it);
				waitNtf->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _timed_pop(overlap_timer::timer_handle& timer, int ms, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			send_pck pck = std::move(_tempBuffer.get());
			pck.cancel_timer(_strand, _alloc);
			_tempBuffer.destroy();
			if (!_sendQueue.empty())
			{
				CoNotifyHandlerFace_* sendWait = _sendQueue.front();
				_sendQueue.pop_front();
				sendWait->invoke(_alloc);
			}
			CHECK_EXCEPTION(tuple_invoke, ntf, std::make_tuple(co_async_state::co_async_ok, csp_result<R>(pck._ntf)), std::move(pck._msg));
		}
		else if (ms > 0)
		{
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_strand->over_timer()->cancel(timer);
				if (co_async_state::co_async_ok == state)
				{
					assert(!_sendQueue.empty());
					_pop(CoChanMsgMove_<Notify>::move(ntf));
				}
				else
				{
					CHECK_EXCEPTION(ntf, state);
				}
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))));
			_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* waitNtf = *it;
				_waitQueue.erase(it);
				waitNtf->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _append_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([&ntfSign](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				assert(ntfSign._nodeEffect);
				ntfSign._nodeEffect = false;
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			ntfSign._ntfNode = --_waitQueue.end();
			ntfSign._nodeEffect = true;
			if (!_sendQueue.empty())
			{
				CoNotifyHandlerFace_* sendNtf = _sendQueue.front();
				_sendQueue.pop_front();
				sendNtf->invoke(_alloc);
			}
		}
	}

	template <typename CbNotify, typename MsgNotify>
	void _try_pop_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(msgNtf, co_async_state::co_async_closed);
			CHECK_EXCEPTION(cb, co_async_state::co_async_closed);
			return;
		}
		if (_tempBuffer.has())
		{
			send_pck pck = std::move(_tempBuffer.get());
			pck.cancel_timer(_strand, _alloc);
			_tempBuffer.destroy();
			if (!_sendQueue.empty())
			{
				CoNotifyHandlerFace_* sendWait = _sendQueue.front();
				_sendQueue.pop_front();
				sendWait->invoke(_alloc);
			}
			_append_pop_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(tuple_invoke, cb, std::make_tuple(co_async_state::co_async_ok, csp_result<R>(pck._ntf)), std::move(pck._msg));
		}
		else
		{
			_append_pop_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
			CHECK_EXCEPTION(cb, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _remove_pop_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			assert(!ntfSign._nodeEffect);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		const bool effect = ntfSign._nodeEffect;
		ntfSign._nodeEffect = false;
		if (effect)
		{
			assert(ntfSign._appended);
			ntfSign._appended = false;
			CoNotifyHandlerFace_* waitNtf = *ntfSign._ntfNode;
			_waitQueue.erase(ntfSign._ntfNode);
			waitNtf->destroy();
			_alloc.deallocate(waitNtf);
		}
		if (_tempBuffer.has())
		{
			if (!_waitQueue.empty())
			{
				CoNotifyHandlerFace_* handler = _waitQueue.front();
				_waitQueue.pop_front();
				handler->invoke(_alloc);
			}
			else if (_msgIsTryPush)
			{
				_tempBuffer->cancel_timer(_strand, _alloc);
				ResultNotifyFace_<R>* ntf_ = _tempBuffer->_ntf;
				_tempBuffer.destroy();
				ntf_->invoke(co_async_state::co_async_fail);
				if (!_sendQueue.empty())
				{
					CoNotifyHandlerFace_* sendNtf = _sendQueue.front();
					_sendQueue.pop_front();
					sendNtf->invoke(_alloc);
				}
			}
		}
		CHECK_EXCEPTION(ntf, effect ? co_async_state::co_async_ok : co_async_state::co_async_fail);
	}

	template <typename Notify>
	void _append_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		if (!_waitQueue.empty())
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			assert(!_tempBuffer.has());
			_sendQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([&ntfSign](typename CoChanMsgMove_<Notify>::type& ntf, co_async_state state)
			{
				assert(ntfSign._nodeEffect);
				ntfSign._nodeEffect = false;
				CHECK_EXCEPTION(ntf, state);
			}, CoChanMsgMove_<Notify>::forward(ntf), __1)));
			ntfSign._ntfNode = --_sendQueue.end();
			ntfSign._nodeEffect = true;
		}
	}

	template <typename CbNotify, typename MsgNotify, typename... Args>
	void _try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg)
	{
		assert(_strand->running_in_this_thread());
		assert(!ntfSign._nodeEffect);
		if (_closed)
		{
			CHECK_EXCEPTION(msgNtf, co_async_state::co_async_closed);
			CHECK_EXCEPTION(cb, co_async_state::co_async_closed);
			return;
		}
		if (!_tempBuffer.has())
		{
			if (!_waitQueue.empty())
			{
				_msgIsTryPush = true;
				_tempBuffer.create(wrap_result_notify(CoChanMsgMove_<CbNotify>::forward(cb)), msg_type(std::forward<Args>(msg)...));
				_append_push_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
				CoNotifyHandlerFace_* handler = _waitQueue.front();
				_waitQueue.pop_front();
				handler->invoke(_alloc);
				return;
			}
		}
		assert(_waitQueue.empty());
		_append_push_notify(CoChanMsgMove_<MsgNotify>::forward(msgNtf), ntfSign);
		CHECK_EXCEPTION(cb, co_async_state::co_async_fail);
	}

	template <typename Notify>
	void _remove_push_notify(Notify&& ntf, co_notify_sign& ntfSign)
	{
		assert(_strand->running_in_this_thread());
		if (_closed)
		{
			assert(!ntfSign._nodeEffect);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_closed);
			return;
		}
		const bool effect = ntfSign._nodeEffect;
		ntfSign._nodeEffect = false;
		if (effect)
		{
			assert(ntfSign._appended);
			ntfSign._appended = false;
			CoNotifyHandlerFace_* sendNtf = *ntfSign._ntfNode;
			_sendQueue.erase(ntfSign._ntfNode);
			sendNtf->destroy();
			_alloc.deallocate(sendNtf);
		}
		if (!_tempBuffer.has() && !_sendQueue.empty())
		{
			CoNotifyHandlerFace_* sendNtf = _sendQueue.front();
			_sendQueue.pop_front();
			sendNtf->invoke(_alloc);
		}
		CHECK_EXCEPTION(ntf, effect ? co_async_state::co_async_ok : co_async_state::co_async_fail);
	}

	void _close()
	{
		assert(_strand->running_in_this_thread());
		_closed = true;
		if (_tempBuffer.has())
		{
			_tempBuffer->cancel_timer(_strand, _alloc);
			ResultNotifyFace_<R>* ntf_ = _tempBuffer->_ntf;
			_tempBuffer.destroy();
			ntf_->invoke(co_async_state::co_async_closed);
		}
		size_t ntfNum1 = 0;
		size_t ntfNum2 = 0;
		CoNotifyHandlerFace_* ntfs1[32];
		CoNotifyHandlerFace_* ntfs2[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx1;
		std::list<CoNotifyHandlerFace_*> ntfsEx2;
		while (!_waitQueue.empty())
		{
			if (ntfNum1 < fixed_array_length(ntfs1))
			{
				ntfs1[ntfNum1++] = _waitQueue.front();
			}
			else
			{
				ntfsEx1.push_back(_waitQueue.front());
			}
			_waitQueue.pop_front();
		}
		while (!_sendQueue.empty())
		{
			if (ntfNum2 < fixed_array_length(ntfs2))
			{
				ntfs2[ntfNum2++] = _sendQueue.front();
			}
			else
			{
				ntfsEx2.push_back(_sendQueue.front());
			}
			_sendQueue.pop_front();
		}
		for (size_t i = 0; i < ntfNum1; i++)
		{
			ntfs1[i]->invoke(_alloc, co_async_state::co_async_closed);
		}
		while (!ntfsEx1.empty())
		{
			ntfsEx1.front()->invoke(_alloc, co_async_state::co_async_closed);
			ntfsEx1.pop_front();
		}
		for (size_t i = 0; i < ntfNum2; i++)
		{
			ntfs2[i]->invoke(_alloc, co_async_state::co_async_closed);
		}
		while (!ntfsEx2.empty())
		{
			ntfsEx2.front()->invoke(_alloc, co_async_state::co_async_closed);
			ntfsEx2.pop_front();
		}
	}

	void _cancel()
	{
		assert(_strand->running_in_this_thread());
		if (_tempBuffer.has())
		{
			_tempBuffer->cancel_timer(_strand, _alloc);
			ResultNotifyFace_<R>* ntf_ = _tempBuffer->_ntf;
			_tempBuffer.destroy();
			ntf_->invoke(co_async_state::co_async_cancel);
		}
		size_t ntfNum1 = 0;
		size_t ntfNum2 = 0;
		CoNotifyHandlerFace_* ntfs1[32];
		CoNotifyHandlerFace_* ntfs2[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx1;
		std::list<CoNotifyHandlerFace_*> ntfsEx2;
		while (!_waitQueue.empty())
		{
			if (ntfNum1 < fixed_array_length(ntfs1))
			{
				ntfs1[ntfNum1++] = _waitQueue.front();
			}
			else
			{
				ntfsEx1.push_back(_waitQueue.front());
			}
			_waitQueue.pop_front();
		}
		while (!_sendQueue.empty())
		{
			if (ntfNum2 < fixed_array_length(ntfs2))
			{
				ntfs2[ntfNum2++] = _sendQueue.front();
			}
			else
			{
				ntfsEx2.push_back(_sendQueue.front());
			}
			_sendQueue.pop_front();
		}
		for (size_t i = 0; i < ntfNum1; i++)
		{
			ntfs1[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx1.empty())
		{
			ntfsEx1.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx1.pop_front();
		}
		for (size_t i = 0; i < ntfNum2; i++)
		{
			ntfs2[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx2.empty())
		{
			ntfsEx2.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx2.pop_front();
		}
	}

	void _cancel_send()
	{
		assert(_strand->running_in_this_thread());
		if (_tempBuffer.has())
		{
			_tempBuffer->cancel_timer(_strand, _alloc);
			ResultNotifyFace_<R>* ntf_ = _tempBuffer->_ntf;
			_tempBuffer.destroy();
			ntf_->invoke(co_async_state::co_async_cancel);
		}
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_sendQueue.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _sendQueue.front();
			}
			else
			{
				ntfsEx.push_back(_sendQueue.front());
			}
			_sendQueue.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}

	void _cancel_wait()
	{
		assert(_strand->running_in_this_thread());
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_waitQueue.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _waitQueue.front();
			}
			else
			{
				ntfsEx.push_back(_waitQueue.front());
			}
			_waitQueue.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc, co_async_state::co_async_cancel);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc, co_async_state::co_async_cancel);
			ntfsEx.pop_front();
		}
	}
private:
	shared_strand _strand;
	stack_obj<send_pck> _tempBuffer;
	reusable_mem _alloc;
	msg_list<CoNotifyHandlerFace_*> _sendQueue;
	msg_list<CoNotifyHandlerFace_*> _waitQueue;
	bool _msgIsTryPush;
	bool _closed;
	NONE_COPY(co_csp_channel);
};

template <typename... Types>
class co_csp_channel<void_type(Types...)> : public co_csp_channel<void_type1(Types...)>
{
	typedef co_csp_channel<void_type1(Types...)> parent;
	template <typename Notify> struct push_notify
	{
		template <typename Ntf>
		push_notify(bool, Ntf&& ntf) :_ntf(std::forward<Ntf>(ntf)) {}
		void operator()(co_async_state st, void_type1) { CHECK_EXCEPTION(_ntf, st); }
		void operator()(co_async_state st) { CHECK_EXCEPTION(_ntf, st); }
		Notify _ntf;
		NONE_COPY(push_notify);
		RVALUE_CONSTRUCT1(push_notify, _ntf);
	};
public:
	co_csp_channel(const shared_strand& strand) :parent(strand) {}
public:
	template <typename Notify, typename... Args> void push(Notify&& ntf, Args&&... msg){ parent::push(push_notify<RM_CREF(Notify)>(bool(), ntf), std::forward<Args>(msg)...); }
	template <typename Notify, typename... Args> void aff_push(Notify&& ntf, Args&&... msg){ parent::aff_push(push_notify<RM_CREF(Notify)>(bool(), ntf), std::forward<Args>(msg)...); }
	template <typename Notify, typename... Args> void try_push(Notify&& ntf, Args&&... msg){ parent::try_push(push_notify<RM_CREF(Notify)>(bool(), ntf), std::forward<Args>(msg)...); }
	template <typename Notify, typename... Args> void aff_try_push(Notify&& ntf, Args&&... msg){ parent::aff_try_push(push_notify<RM_CREF(Notify)>(bool(), ntf), std::forward<Args>(msg)...); }
	template <typename Notify, typename... Args> void timed_push(int ms, Notify&& ntf, Args&&... msg){ parent::timed_push(ms, push_notify<RM_CREF(Notify)>(bool(), ntf), std::forward<Args>(msg)...); }
	template <typename Notify, typename... Args> void aff_timed_push(int ms, Notify&& ntf, Args&&... msg){ parent::aff_timed_push(ms, push_notify<RM_CREF(Notify)>(bool(), ntf), std::forward<Args>(msg)...); }
	template <typename Notify, typename... Args> void timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg){ parent::timed_push(timer, ms, push_notify<RM_CREF(Notify)>(bool(), ntf), std::forward<Args>(msg)...); }
	template <typename Notify, typename... Args> void aff_timed_push(overlap_timer::timer_handle& timer, int ms, Notify&& ntf, Args&&... msg){ parent::aff_timed_push(timer, ms, push_notify<RM_CREF(Notify)>(bool(), ntf), std::forward<Args>(msg)...); }
	template <typename CbNotify, typename MsgNotify, typename... Args> void try_push_and_append_notify(CbNotify&& cb, MsgNotify&& msgNtf, co_notify_sign& ntfSign, Args&&... msg){
		co_nil_channel<void_type>::try_push_and_append_notify(push_notify<RM_CREF(CbNotify)>(bool(), cb), std::forward<MsgNotify>(msgNtf), ntfSign, std::forward<Args>(msg)...);
	}
};

template <typename... Types>
class co_csp_channel<void(Types...)> : public co_csp_channel<void_type(Types...)>
{
public:
	co_csp_channel(const shared_strand& strand) :co_csp_channel<void_type(Types...)>(strand) {}

	static std::shared_ptr<co_csp_channel> make(const shared_strand& strand)
	{
		return std::make_shared<co_csp_channel>(strand);
	}
};

struct co_select_sign
{
	co_select_sign(co_generator):co_select_sign(co_strand) {}	
	co_select_sign(const shared_strand& strand)
	:_ntfPump(strand, 16), _ntfSign(16), _currSign(NULL), _selectId(-1), _ntfState(co_async_state::co_async_undefined)
	{
		DEBUG_OPERATION(_labelId = -1);
	}

	void reset()
	{
		_ntfPump.reset();
		_ntfSign.clear();
		_currSign = NULL;
		_selectId = -1;
		_ntfState = co_async_state::co_async_undefined;
		DEBUG_OPERATION(_labelId = -1);
	}

	co_msg_buffer<size_t, co_async_state> _ntfPump;
	msg_map<size_t, co_notify_sign> _ntfSign;
	co_notify_sign* _currSign;
	size_t _selectId;
	co_async_state _ntfState;
	DEBUG_OPERATION(int _labelId);
	NONE_COPY(co_select_sign);
};

class co_shared_mutex;
/*!
@brief generator中逻辑mutex同步
*/
class co_mutex
{
	struct wait_node
	{
		CoNotifyHandlerFace_* _ntf;
		long long _waitHostID;
	};
	friend co_shared_mutex;
public:
	co_mutex(const shared_strand& strand)
		:_strand(strand), _waitQueue(4), _lockActorID(0), _recCount(0)
	{
	}

	~co_mutex()
	{
		assert(_waitQueue.empty());
	}
public:
	template <typename Notify>
	void lock(long long id, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_lock(id, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_lock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify, typename LockedNtf>
	void lock(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (_strand->running_in_this_thread())
		{
			_lock(id, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			_strand->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_lock(id, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify>
	void try_lock(long long id, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_try_lock(id, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_try_lock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_lock(long long id, int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_lock(id, ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, id, ms](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_lock(id, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_lock(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_lock(timer, id, ms, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, id, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_lock(timer, id, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify, typename LockedNtf>
	void timed_lock(long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_lock(id, ms, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			_strand->post(std::bind([this, id, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_timed_lock(id, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify, typename LockedNtf>
	void timed_lock(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_lock(timer, id, ms, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			_strand->post(std::bind([this, id, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_timed_lock(timer, id, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify>
	void unlock(long long id, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_unlock(id, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_unlock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	const shared_strand& self_strand() const
	{
		return _strand;
	}

	static long long alloc_id()
	{
		return generator::alloc_id();
	}
private:
	bool check_self_err_call(const long long id)
	{
		for (wait_node& ele : _waitQueue)
		{
			if (id == ele._waitHostID)
			{
				return false;
			}
		}
		return true;
	}

	template <typename Notify>
	void _lock(long long id, Notify&& ntf)
	{
		_lock(id, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify, typename LockedNtf>
	void _lock(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(_strand->running_in_this_thread());
		assert(check_self_err_call(id));
		if (!_lockActorID || id == _lockActorID)
		{
			_lockActorID = id;
			_recCount++;
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_nil_state_notify(_alloc, std::forward<Notify>(ntf)), id });
			CHECK_EXCEPTION(lockedNtf);
		}
	}

	template <typename Notify>
	void _try_lock(long long id, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		assert(check_self_err_call(id));
		if (!_lockActorID || id == _lockActorID)
		{
			_lockActorID = id;
			_recCount++;
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _timed_lock(long long id, int ms, Notify&& ntf)
	{
		_timed_lock(id, ms, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify>
	void _timed_lock(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf)
	{
		_timed_lock(timer, id, ms, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify, typename LockedNtf>
	void _timed_lock(long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(_strand->running_in_this_thread());
		assert(check_self_err_call(id));
		if (!_lockActorID || id == _lockActorID)
		{
			_lockActorID = id;
			_recCount++;
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else if (ms > 0)
		{
			overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_strand->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_alloc.deallocate(timer);
				CHECK_EXCEPTION(ntf, state);
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))), id });
			_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const msg_list<wait_node>::iterator& it)
			{
				CoNotifyHandlerFace_* waitNtf = it->_ntf;
				_waitQueue.erase(it);
				waitNtf->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
			CHECK_EXCEPTION(lockedNtf);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify, typename LockedNtf>
	void _timed_lock(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(_strand->running_in_this_thread());
		assert(check_self_err_call(id));
		if (!_lockActorID || id == _lockActorID)
		{
			_lockActorID = id;
			_recCount++;
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else if (ms > 0)
		{
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, &timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_strand->over_timer()->cancel(timer);
				CHECK_EXCEPTION(ntf, state);
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))), id });
			_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const msg_list<wait_node>::iterator& it)
			{
				CoNotifyHandlerFace_* waitNtf = it->_ntf;
				_waitQueue.erase(it);
				waitNtf->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
			CHECK_EXCEPTION(lockedNtf);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	bool _unlock1(long long id)
	{
		if (0 == --_recCount)
		{
			if (!_waitQueue.empty())
			{
				_recCount = 1;
				wait_node queueFront = _waitQueue.front();
				_waitQueue.pop_front();
				_lockActorID = queueFront._waitHostID;
				queueFront._ntf->invoke(_alloc);
			}
			else
			{
				_lockActorID = 0;
			}
			return true;
		}
		return false;
	}

	template <typename Notify>
	void _unlock(long long id, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		assert(check_self_err_call(id));
		assert(id == _lockActorID);
		_unlock1(id);
		CHECK_EXCEPTION(ntf);
	}
private:
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
	reusable_mem _alloc;
	long long _lockActorID;
	size_t _recCount;
	NONE_COPY(co_mutex);
};

/*!
@brief generator中逻辑shared_mutex同步
*/
class co_shared_mutex
{
	enum lock_status
	{
		st_shared,
		st_unique,
		st_upgrade
	};

	struct wait_node
	{
		CoNotifyHandlerFace_* _ntf;
		long long _waitHostID;
		lock_status _status;
	};

	struct shared_count
	{
		size_t _count = 0;
	};
public:
	co_shared_mutex(const shared_strand& strand)
		:_upgradeMutex(strand), _waitQueue(4), _sharedMap(4)
	{
	}

	~co_shared_mutex()
	{
		assert(_waitQueue.empty());
	}
public:
	template <typename Notify>
	void lock(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_lock(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_lock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify, typename LockedNtf>
	void lock(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_lock(id, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_lock(id, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify>
	void try_lock(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_try_lock(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_try_lock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_lock(long long id, int ms, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_timed_lock(id, ms, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id, ms](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_lock(id, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_lock(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_timed_lock(timer, id, ms, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_lock(timer, id, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}
	
	template <typename Notify, typename LockedNtf>
	void timed_lock(long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_timed_lock(id, ms, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			self_strand()->post(std::bind([this, id, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_timed_lock(id, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify, typename LockedNtf>
	void timed_lock(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_timed_lock(timer, id, ms, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			self_strand()->post(std::bind([this, id, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_timed_lock(timer, id, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify>
	void lock_shared(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_lock_shared(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_lock_shared(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}
	
	template <typename Notify, typename LockedNtf>
	void lock_shared(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_lock_shared(id, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_lock_shared(id, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify>
	void lock_pess_shared(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_lock_pess_shared(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_lock_pess_shared(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify, typename LockedNtf>
	void lock_pess_shared(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_lock_pess_shared(id, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_lock_pess_shared(id, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify>
	void try_lock_shared(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_try_lock_shared(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_try_lock_shared(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_lock_shared(long long id, int ms, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_timed_lock_shared(id, ms, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id, ms](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_lock_shared(id, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_lock_shared(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_timed_lock_shared(timer, id, ms, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_lock_shared(timer, id, ms, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify, typename LockedNtf>
	void timed_lock_shared(long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_timed_lock_shared(id, ms, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			self_strand()->post(std::bind([this, id, ms](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_timed_lock_shared(id, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify, typename LockedNtf>
	void timed_lock_shared(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_timed_lock_shared(timer, id, ms, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			self_strand()->post(std::bind([this, id, ms, &timer](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_timed_lock_shared(timer, id, ms, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify>
	void lock_upgrade(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_lock_upgrade(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_lock_upgrade(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}
	
	template <typename Notify, typename LockedNtf>
	void lock_upgrade(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_lock_upgrade(id, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf, typename CoChanMsgMove_<LockedNtf>::type& lockedNtf)
			{
				_lock_upgrade(id, CoChanMsgMove_<Notify>::move(ntf), CoChanMsgMove_<LockedNtf>::move(lockedNtf));
			}, CoChanMsgMove_<Notify>::forward(ntf), CoChanMsgMove_<LockedNtf>::forward(lockedNtf)));
		}
	}

	template <typename Notify>
	void try_lock_upgrade(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_try_lock_upgrade(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_try_lock_upgrade(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void unlock(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_unlock(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_unlock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void unlock_shared(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_unlock_shared(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_unlock_shared(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void unlock_upgrade(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_unlock_upgrade(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_unlock_upgrade(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void unlock_and_lock_shared(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_unlock_and_lock_shared(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_unlock_and_lock_shared(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void unlock_and_lock_upgrade(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_unlock_and_lock_upgrade(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_unlock_and_lock_upgrade(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void unlock_upgrade_and_lock(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_unlock_upgrade_and_lock(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_unlock_upgrade_and_lock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void unlock_shared_and_lock(long long id, Notify&& ntf)
	{
		if (self_strand()->running_in_this_thread())
		{
			_unlock_shared_and_lock(id, std::forward<Notify>(ntf));
		}
		else
		{
			self_strand()->post(std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_unlock_shared_and_lock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	const shared_strand& self_strand() const
	{
		return _upgradeMutex.self_strand();
	}

	static long long alloc_id()
	{
		return generator::alloc_id();
	}
private:
	bool check_self_err_call(const long long id)
	{
		for (wait_node& ele : _waitQueue)
		{
			if (id == ele._waitHostID)
			{
				return false;
			}
		}
		return true;
	}

	template <typename Notify>
	void _lock(long long id, Notify&& ntf)
	{
		_lock(id, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify, typename LockedNtf>
	void _lock(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		long long& lockActorID = _upgradeMutex._lockActorID;
		if (_sharedMap.empty() && (!lockActorID || id == lockActorID))
		{
			lockActorID = id;
			_upgradeMutex._recCount++;
			DEBUG_OPERATION(_inSet[id] = st_unique);
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_nil_state_notify(_upgradeMutex._alloc, std::forward<Notify>(ntf)), id, st_unique });
			CHECK_EXCEPTION(lockedNtf);
		}
	}

	template <typename Notify>
	void _try_lock(long long id, Notify&& ntf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		long long& lockActorID = _upgradeMutex._lockActorID;
		if (_sharedMap.empty() && (!lockActorID || id == lockActorID))
		{
			lockActorID = id;
			_upgradeMutex._recCount++;
			DEBUG_OPERATION(_inSet[id] = st_unique);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _timed_lock(long long id, int ms, Notify&& ntf)
	{
		_timed_lock(id, ms, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify>
	void _timed_lock(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf)
	{
		_timed_lock(timer, id, ms, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify, typename LockedNtf>
	void _timed_lock(long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		long long& lockActorID = _upgradeMutex._lockActorID;
		if (_sharedMap.empty() && (!lockActorID || id == lockActorID))
		{
			lockActorID = id;
			_upgradeMutex._recCount++;
			DEBUG_OPERATION(_inSet[id] = st_unique);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else if (ms > 0)
		{
			overlap_timer::timer_handle* timer = new(_upgradeMutex._alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_notify(_upgradeMutex._alloc, std::bind([this, timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				self_strand()->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_upgradeMutex._alloc.deallocate(timer);
				CHECK_EXCEPTION(ntf, state);
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))), id, st_unique });
			self_strand()->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const msg_list<wait_node>::iterator& it)
			{
				CoNotifyHandlerFace_* waitNtf = it->_ntf;
				_waitQueue.erase(it);
				waitNtf->invoke(_upgradeMutex._alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
			CHECK_EXCEPTION(lockedNtf);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify, typename LockedNtf>
	void _timed_lock(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		long long& lockActorID = _upgradeMutex._lockActorID;
		if (_sharedMap.empty() && (!lockActorID || id == lockActorID))
		{
			lockActorID = id;
			_upgradeMutex._recCount++;
			DEBUG_OPERATION(_inSet[id] = st_unique);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else if (ms > 0)
		{
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_notify(_upgradeMutex._alloc, std::bind([this, &timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				self_strand()->over_timer()->cancel(timer);
				CHECK_EXCEPTION(ntf, state);
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))), id, st_unique });
			self_strand()->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const msg_list<wait_node>::iterator& it)
			{
				CoNotifyHandlerFace_* waitNtf = it->_ntf;
				_waitQueue.erase(it);
				waitNtf->invoke(_upgradeMutex._alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
			CHECK_EXCEPTION(lockedNtf);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _lock_shared(long long id, Notify&& ntf)
	{
		_lock_shared(id, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify, typename LockedNtf>
	void _lock_shared(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		if (!_sharedMap.empty() || !_upgradeMutex._lockActorID)
		{
			_sharedMap[id]._count++;
			DEBUG_OPERATION(_inSet[id] = st_shared);
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_nil_state_notify(_upgradeMutex._alloc, std::forward<Notify>(ntf)), id, st_shared });
			CHECK_EXCEPTION(lockedNtf);
		}
	}

	template <typename Notify>
	void _lock_pess_shared(long long id, Notify&& ntf)
	{
		_lock_pess_shared(id, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify, typename LockedNtf>
	void _lock_pess_shared(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		if (_waitQueue.empty() && (!_sharedMap.empty() || !_upgradeMutex._lockActorID))
		{
			_sharedMap[id]._count++;
			DEBUG_OPERATION(_inSet[id] = st_shared);
			CHECK_EXCEPTION(ntf);
		}
		else
		{
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_nil_state_notify(_upgradeMutex._alloc, std::forward<Notify>(ntf)), id, st_shared });
			CHECK_EXCEPTION(lockedNtf);
		}
	}

	template <typename Notify>
	void _try_lock_shared(long long id, Notify&& ntf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		if (!_sharedMap.empty() || !_upgradeMutex._lockActorID)
		{
			_sharedMap[id]._count++;
			DEBUG_OPERATION(_inSet[id] = st_shared);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_fail);
		}
	}

	template <typename Notify>
	void _timed_lock_shared(long long id, int ms, Notify&& ntf)
	{
		_timed_lock_shared(id, ms, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify>
	void _timed_lock_shared(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf)
	{
		_timed_lock_shared(timer, id, ms, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify, typename LockedNtf>
	void _timed_lock_shared(long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		if (!_sharedMap.empty() || !_upgradeMutex._lockActorID)
		{
			_sharedMap[id]._count++;
			DEBUG_OPERATION(_inSet[id] = st_shared);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else if (ms > 0)
		{
			overlap_timer::timer_handle* timer = new(_upgradeMutex._alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_notify(_upgradeMutex._alloc, std::bind([this, timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				self_strand()->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_upgradeMutex._alloc.deallocate(timer);
				CHECK_EXCEPTION(ntf, state);
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))), id, st_shared });
			self_strand()->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const msg_list<wait_node>::iterator& it)
			{
				CoNotifyHandlerFace_* waitNtf = it->_ntf;
				_waitQueue.erase(it);
				waitNtf->invoke(_upgradeMutex._alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
			CHECK_EXCEPTION(lockedNtf);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify, typename LockedNtf>
	void _timed_lock_shared(overlap_timer::timer_handle& timer, long long id, int ms, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		if (!_sharedMap.empty() || !_upgradeMutex._lockActorID)
		{
			_sharedMap[id]._count++;
			DEBUG_OPERATION(_inSet[id] = st_shared);
			CHECK_EXCEPTION(ntf, co_async_state::co_async_ok);
		}
		else if (ms > 0)
		{
			_waitQueue.push_back(wait_node{ CoNotifyHandlerFace_::wrap_notify(_upgradeMutex._alloc, std::bind([this, &timer](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				self_strand()->over_timer()->cancel(timer);
				CHECK_EXCEPTION(ntf, state);
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))), id, st_shared });
			self_strand()->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const msg_list<wait_node>::iterator& it)
			{
				CoNotifyHandlerFace_* waitNtf = it->_ntf;
				_waitQueue.erase(it);
				waitNtf->invoke(_upgradeMutex._alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
			CHECK_EXCEPTION(lockedNtf);
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _lock_upgrade(long long id, Notify&& ntf)
	{
		_lock_upgrade(id, std::forward<Notify>(ntf), []{});
	}

	template <typename Notify, typename LockedNtf>
	void _lock_upgrade(long long id, Notify&& ntf, LockedNtf&& lockedNtf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		assert(_inSet.find(id) != _inSet.end());
		assert(st_unique != _inSet.find(id)->second);
		assert(_sharedMap.find(id) != _sharedMap.end());
#if (_DEBUG || DEBUG)
		_upgradeMutex._lock(id, std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			DEBUG_OPERATION(_inSet[id] = st_upgrade);
			CHECK_EXCEPTION(ntf);
		}, CoChanMsgMove_<Notify>::forward(ntf)), std::forward<LockedNtf>(lockedNtf));
#else
		_upgradeMutex._lock(id, std::forward<Notify>(ntf), std::forward<LockedNtf>(lockedNtf));
#endif
	}
	
	template <typename Notify>
	void _try_lock_upgrade(long long id, Notify&& ntf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		assert(_inSet.find(id) != _inSet.end());
		assert(st_unique != _inSet.find(id)->second);
		assert(_sharedMap.find(id) != _sharedMap.end());
#if (_DEBUG || DEBUG)
		_upgradeMutex._try_lock(id, std::bind([this, id](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
		{
			DEBUG_OPERATION(if (co_async_state::co_async_ok == state) _inSet[id] = st_upgrade);
			CHECK_EXCEPTION(ntf, state);
		}, __1, CoChanMsgMove_<Notify>::forward(ntf)));
#else
		_upgradeMutex._try_lock(id, std::forward<Notify>(ntf));
#endif
	}

	void _unlock1(long long id)
	{
		if (!--_upgradeMutex._recCount && !_waitQueue.empty())
		{
			size_t ntfNum = 0;
			CoNotifyHandlerFace_* ntfs[32];
			std::list<CoNotifyHandlerFace_*> ntfsEx;
			wait_node queueFront = _waitQueue.front();
			_waitQueue.pop_front();
			ntfs[ntfNum++] = queueFront._ntf;
			DEBUG_OPERATION(_inSet.erase(id));
			DEBUG_OPERATION(_inSet[queueFront._waitHostID] = queueFront._status);
			if (st_shared == queueFront._status)
			{
				_upgradeMutex._lockActorID = 0;
				_sharedMap[queueFront._waitHostID]._count++;
				for (auto it = _waitQueue.begin(); it != _waitQueue.end();)
				{
					if (st_shared == it->_status)
					{
						assert(_inSet.find(it->_waitHostID) == _inSet.end());
						DEBUG_OPERATION(_inSet[it->_waitHostID] = st_shared);
						_sharedMap[it->_waitHostID]._count++;
						if (ntfNum < fixed_array_length(ntfs))
						{
							ntfs[ntfNum++] = it->_ntf;
						}
						else
						{
							ntfsEx.push_back(it->_ntf);
						}
						_waitQueue.erase(it++);
					}
					else
					{
						++it;
					}
				}
			}
			else
			{
				_upgradeMutex._lockActorID = queueFront._waitHostID;
				_upgradeMutex._recCount++;
			}
			for (size_t i = 0; i < ntfNum; i++)
			{
				ntfs[i]->invoke(_upgradeMutex._alloc);
			}
			while (!ntfsEx.empty())
			{
				ntfsEx.front()->invoke(_upgradeMutex._alloc);
				ntfsEx.pop_front();
			}
		}
	}

	template <typename Notify>
	void _unlock(long long id, Notify&& ntf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		assert(_inSet.find(id) != _inSet.end());
		assert(st_unique == _inSet.find(id)->second);
		assert(_sharedMap.empty());
		assert(id == _upgradeMutex._lockActorID && _upgradeMutex._recCount);
		_unlock1(id);
		CHECK_EXCEPTION(ntf);
	}

	void _unlock_shared1(long long id)
	{
		auto sit = _sharedMap.find(id);
		if (!--sit->second._count)
		{
			assert(id != _upgradeMutex._lockActorID);
			DEBUG_OPERATION(_inSet.erase(id));
			_sharedMap.erase(sit);
			if (_sharedMap.empty() && !_waitQueue.empty())
			{
				size_t ntfNum = 0;
				CoNotifyHandlerFace_* ntfs[32];
				std::list<CoNotifyHandlerFace_*> ntfsEx;
				wait_node queueFront = _waitQueue.front();
				_waitQueue.pop_front();
				ntfs[ntfNum++] = queueFront._ntf;
				DEBUG_OPERATION(_inSet[queueFront._waitHostID] = queueFront._status);
				if (st_shared == queueFront._status)
				{
					_upgradeMutex._lockActorID = 0;
					_sharedMap[queueFront._waitHostID]._count++;
					for (auto it = _waitQueue.begin(); it != _waitQueue.end();)
					{
						if (st_shared == it->_status)
						{
							assert(_inSet.find(it->_waitHostID) == _inSet.end());
							DEBUG_OPERATION(_inSet[it->_waitHostID] = st_shared);
							_sharedMap[it->_waitHostID]._count++;
							if (ntfNum < fixed_array_length(ntfs))
							{
								ntfs[ntfNum++] = it->_ntf;
							}
							else
							{
								ntfsEx.push_back(it->_ntf);
							}
							_waitQueue.erase(it++);
						}
						else
						{
							++it;
						}
					}
				}
				else
				{
					assert(!_upgradeMutex._lockActorID);
					_upgradeMutex._lockActorID = queueFront._waitHostID;
					_upgradeMutex._recCount++;
				}
				for (size_t i = 0; i < ntfNum; i++)
				{
					ntfs[i]->invoke(_upgradeMutex._alloc);
				}
				while (!ntfsEx.empty())
				{
					ntfsEx.front()->invoke(_upgradeMutex._alloc);
					ntfsEx.pop_front();
				}
			}
		}
	}

	template <typename Notify>
	void _unlock_shared(long long id, Notify&& ntf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		assert(_inSet.find(id) != _inSet.end());
		assert(st_shared == _inSet.find(id)->second);
		assert(_sharedMap.find(id) != _sharedMap.end());
		assert(_sharedMap[id]._count);
		_unlock_shared1(id);
		CHECK_EXCEPTION(ntf);
	}

	template <typename Notify>
	void _unlock_upgrade(long long id, Notify&& ntf)
	{
		assert(self_strand()->running_in_this_thread());
		assert(check_self_err_call(id));
		assert(_inSet.find(id) != _inSet.end());
		assert(st_upgrade == _inSet.find(id)->second);
		assert(_sharedMap.find(id) != _sharedMap.end());
		if (_upgradeMutex._unlock1(id))
		{
			DEBUG_OPERATION(_inSet[id] = st_shared);
		}
		CHECK_EXCEPTION(ntf);
	}

	template <typename Notify>
	void _unlock_and_lock_shared(long long id, Notify&& ntf)
	{
		_unlock(id, std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_lock_shared(id, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void _unlock_and_lock_upgrade(long long id, Notify&& ntf)
	{
		_unlock(id, std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_lock_shared(id, std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_lock_upgrade(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::move(ntf)));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void _unlock_upgrade_and_lock(long long id, Notify&& ntf)
	{
		_unlock_upgrade(id, std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_unlock_shared(id, std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_lock(id, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::move(ntf)));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}

	template <typename Notify>
	void _unlock_shared_and_lock(long long id, Notify&& ntf)
	{
		_unlock_shared(id, std::bind([this, id](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			_lock(id, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf)));
	}
private:
	co_mutex _upgradeMutex;
	msg_list<wait_node> _waitQueue;
	msg_map<long long, shared_count> _sharedMap;
#if (_DEBUG || DEBUG)
	msg_map<long long, lock_status> _inSet;
#endif
	NONE_COPY(co_shared_mutex);
};

/*!
@brief 条件变量
*/
class co_condition_variable
{
public:
	co_condition_variable(const shared_strand& strand)
		:_strand(strand) {}
public:
	template <typename Notify>
	void wait(long long id, co_mutex& mtx, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_wait(id, mtx, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, id, &mtx](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_wait(id, mtx, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_wait(long long id, int ms, co_mutex& mtx, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_wait(id, ms, mtx, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, id, ms, &mtx](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_wait(id, ms, mtx, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	template <typename Notify>
	void timed_wait(overlap_timer::timer_handle& timer, long long id, int ms, co_mutex& mtx, Notify&& ntf)
	{
		if (_strand->running_in_this_thread())
		{
			_timed_wait(timer, id, ms, mtx, std::forward<Notify>(ntf));
		}
		else
		{
			_strand->post(std::bind([this, id, ms, &timer, &mtx](typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_timed_wait(timer, id, ms, mtx, CoChanMsgMove_<Notify>::move(ntf));
			}, CoChanMsgMove_<Notify>::forward(ntf)));
		}
	}

	void notify_one()
	{
		if (_strand->running_in_this_thread())
		{
			_notify_one();
		}
		else
		{
			_strand->post([this]()
			{
				_notify_one();
			});
		}
	}

	void notify_all()
	{
		if (_strand->running_in_this_thread())
		{
			_notify_all();
		}
		else
		{
			_strand->post([this]()
			{
				_notify_all();
			});
		}
	}

	const shared_strand& self_strand()
	{
		return _strand;
	}
private:
	template <typename Notify>
	void _wait(long long id, co_mutex& mtx, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		_waitQueue.push_back(CoNotifyHandlerFace_::wrap_nil_state_notify(_alloc, std::bind([this, id, &mtx](typename CoChanMsgMove_<Notify>::type& ntf)
		{
			mtx.lock(id, CoChanMsgMove_<Notify>::move(ntf));
		}, CoChanMsgMove_<Notify>::forward(ntf))));
		mtx.unlock(id, any_handler());
	}

	template <typename Notify>
	void _timed_wait(long long id, int ms, co_mutex& mtx, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (ms > 0)
		{
			overlap_timer::timer_handle* timer = new(_alloc.allocate(sizeof(overlap_timer::timer_handle)))overlap_timer::timer_handle;
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, timer, id, &mtx](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_strand->over_timer()->cancel(*timer);
				timer->~timer_handle();
				_alloc.deallocate(timer);
				mtx.lock(id, wrap_bind_(std::bind(CoChanMsgMove_<Notify>::move(ntf), state)));
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))));
			_strand->over_timer()->timeout(ms, *timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* conWait = *it;
				_waitQueue.erase(it);
				conWait->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
			mtx.unlock(id, any_handler());
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	template <typename Notify>
	void _timed_wait(overlap_timer::timer_handle& timer, long long id, int ms, co_mutex& mtx, Notify&& ntf)
	{
		assert(_strand->running_in_this_thread());
		if (ms > 0)
		{
			_waitQueue.push_back(CoNotifyHandlerFace_::wrap_notify(_alloc, std::bind([this, id, &timer, &mtx](co_async_state state, typename CoChanMsgMove_<Notify>::type& ntf)
			{
				_strand->over_timer()->cancel(timer);
				mtx.lock(id, wrap_bind_(std::bind(CoChanMsgMove_<Notify>::move(ntf), state)));
			}, __1, CoChanMsgMove_<Notify>::forward(ntf))));
			_strand->over_timer()->timeout(ms, timer, wrap_bind_(std::bind([this](const co_notify_node& it)
			{
				CoNotifyHandlerFace_* conWait = *it;
				_waitQueue.erase(it);
				conWait->invoke(_alloc, co_async_state::co_async_overtime);
			}, --_waitQueue.end())));
			mtx.unlock(id, any_handler());
		}
		else
		{
			CHECK_EXCEPTION(ntf, co_async_state::co_async_overtime);
		}
	}

	void _notify_one()
	{
		assert(_strand->running_in_this_thread());
		if (!_waitQueue.empty())
		{
			CoNotifyHandlerFace_* waitNtf = _waitQueue.front();
			_waitQueue.pop_front();
			waitNtf->invoke(_alloc);
		}
	}

	void _notify_all()
	{
		assert(_strand->running_in_this_thread());
		size_t ntfNum = 0;
		CoNotifyHandlerFace_* ntfs[32];
		std::list<CoNotifyHandlerFace_*> ntfsEx;
		while (!_waitQueue.empty())
		{
			if (ntfNum < fixed_array_length(ntfs))
			{
				ntfs[ntfNum++] = _waitQueue.front();
			}
			else
			{
				ntfsEx.push_back(_waitQueue.front());
			}
			_waitQueue.pop_front();
		}
		for (size_t i = 0; i < ntfNum; i++)
		{
			ntfs[i]->invoke(_alloc);
		}
		while (!ntfsEx.empty())
		{
			ntfsEx.front()->invoke(_alloc);
			ntfsEx.pop_front();
		}
	}
private:
	shared_strand _strand;
	reusable_mem _alloc;
	msg_list<CoNotifyHandlerFace_*> _waitQueue;
	NONE_COPY(co_condition_variable);
};
//////////////////////////////////////////////////////////////////////////

template <typename Chan> template <typename... Args>
void CoChanSeamlessTryO_<Chan>::pop(CoChanSeamlessTryO_* this_, Args&&... args)
{
	co_generator = this_->_host;
	co_select_sign& selectSign = this_->_selectSign;
	co_notify_sign* const currSign = selectSign._currSign;
	const size_t chanId = selectSign._selectId;
	assert(chanId == (size_t)&this_->_chan && !currSign->_appended && !currSign->_disable);
	currSign->_appended = true;
	this_->_chan.try_pop_and_append_notify(co_async_result_(this_->_state, args...), [&, chanId](co_async_state st)
	{
		if (co_async_state::co_async_fail != st)
		{
			selectSign._ntfPump.send(chanId, st);
		}
	}, *currSign);
}

template <typename Chan> template <typename... Args>
void CoChanSeamlessTryI_<Chan>::push(CoChanSeamlessTryI_* this_, Args&&... args)
{
	co_generator = this_->_host;
	co_select_sign& selectSign = this_->_selectSign;
	co_notify_sign* const currSign = selectSign._currSign;
	const size_t chanId = selectSign._selectId;
	assert(chanId == (size_t)&this_->_chan && !currSign->_appended && !currSign->_disable);
	currSign->_appended = true;
	this_->_chan.try_push_and_append_notify(co_async_result(this_->_state), [&, chanId](co_async_state st)
	{
		if (co_async_state::co_async_fail != st)
		{
			selectSign._ntfPump.send(chanId, st);
		}
	}, *currSign, std::forward<Args>(args)...);
}

template <typename R, typename CspChan> template <typename... Args>
void CoCspSeamlessTryO_<R, CspChan>::pop(CoCspSeamlessTryO_* this_, Args&&... args)
{
	co_generator = this_->_host;
	co_select_sign& selectSign = this_->_selectSign;
	co_notify_sign* const currSign = selectSign._currSign;
	const size_t chanId = selectSign._selectId;
	assert(chanId == (size_t)&this_->_chan && !currSign->_appended && !currSign->_disable);
	currSign->_appended = true;
	this_->_chan.try_pop_and_append_notify(co_async_result_(this_->_state, this_->_res, args...), [&, chanId](co_async_state st)
	{
		if (co_async_state::co_async_fail != st)
		{
			selectSign._ntfPump.send(chanId, st);
		}
	}, *currSign);
}

template <typename R, typename CspChan> template <typename... Args>
void CoCspSeamlessTryI_<R, CspChan>::push(CoCspSeamlessTryI_* this_, Args&&... args)
{
	co_generator = this_->_host;
	co_select_sign& selectSign = this_->_selectSign;
	co_notify_sign* const currSign = selectSign._currSign;
	const size_t chanId = selectSign._selectId;
	assert(chanId == (size_t)&this_->_chan && !currSign->_appended && !currSign->_disable);
	currSign->_appended = true;
	this_->_chan.try_push_and_append_notify(co_async_result_(this_->_state, this_->_res), [&, chanId](co_async_state st)
	{
		if (co_async_state::co_async_fail != st)
		{
			selectSign._ntfPump.send(chanId, st);
		}
	}, *currSign, std::forward<Args>(args)...);
}

#endif