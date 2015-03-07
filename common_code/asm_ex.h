#ifndef __GET_SP_H
#define __GET_SP_H

#define   LIBPATH(p, f)   p##f 

#ifdef _WIN64
#pragma comment(lib, LIBPATH(__FILE__, "./../get_sp_x64.lib"))
#else
#pragma comment(lib, LIBPATH(__FILE__, "./../get_sp_x86.lib"))
#pragma comment(lib, LIBPATH(__FILE__, "./../64div32_x86.lib"))
#endif

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
extern "C" unsigned __fastcall u64div32(unsigned long long a, unsigned b, unsigned* pys = NULL);
extern "C" int __fastcall i64div32(long long a, int b, int* pys = NULL);

#else


#endif

#endif