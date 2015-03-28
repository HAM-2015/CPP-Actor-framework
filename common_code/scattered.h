#ifndef __SCATTERED_H
#define __SCATTERED_H

#define   LIBPATH(p, f)   p##f 

#ifdef _WIN64
#pragma comment(lib, LIBPATH(__FILE__, "./../get_sp_x64.lib"))
#else
#pragma comment(lib, LIBPATH(__FILE__, "./../get_sp_x86.lib"))
#pragma comment(lib, LIBPATH(__FILE__, "./../64div32_x86.lib"))
#endif // _WIN64

#undef LIBPATH

/*!
@brief 获取当前esp/rsp栈顶寄存器值
*/
extern "C" void* __stdcall get_sp();

/*!
@brief 获取CPU计数
*/
extern "C" unsigned long long __fastcall cpu_tick();

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

/*!
@brief 清空std::function
*/
template <typename F>
inline void clear_function(F& f)
{
	f = F();
}

#endif