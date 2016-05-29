#ifndef __ACTOR_CPP_H
#define __ACTOR_CPP_H

/*

CHECK_SELF 启用检测当前代码运行在哪个Actor下
ENABLE_QT_UI 启用QT-UI
ENABLE_QT_ACTOR 启用在QT-UI线程中运行Actor(在ENABLE_QT_UI下使用)
ENABLE_NEXT_TICK 启用next_tick加速
ENABLE_CHECK_LOST 启用通知句柄丢失检测
ENABLE_DUMP_STACK 启用栈溢出检测
PRINT_ACTOR_STACK 检测actor堆栈，打印日志
DISABLE_AUTO_STACK 禁用栈空间自动伸缩控制
DISABLE_HIGH_TIMER 禁用high_resolution_timer计时，将启用deadline_timer计时
DISABLE_BOOST_TIMER 禁用boost计时器，用waitable_timer计时
ENABLE_GLOBAL_TIMER 启用全局定时器(DISABLE_BOOST_TIMER下使用)
ENABLE_TLS_CHECK_SELF 启用TLS技术检测当前代码运行在哪个Actor下

*/

#include "actor_framework.cpp"
#include "actor_mutex.cpp"
#include "actor_timer.cpp"
#include "async_timer.cpp"
#include "bind_qt_run.cpp"
#include "context_pool.cpp"
#include "context_yield.cpp"
#include "io_engine.cpp"
#include "qt_strand.cpp"
#include "run_thread.cpp"
#include "scattered.cpp"
#include "shared_strand.cpp"
#include "strand_ex.cpp"
#include "trace_stack.cpp"
#include "waitable_timer.cpp"

#endif