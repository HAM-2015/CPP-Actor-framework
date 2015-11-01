#ifndef __TRY_MOVE_H
#define __TRY_MOVE_H

#include <type_traits>
#include <assert.h>

template <typename T>
struct try_move
{
	//不正确参数，无法检测是否可移动

	template <typename Arg>
	static inline Arg&& move(Arg&& p)
	{
		return (Arg&&)p;
	}
};

template <typename T>
struct try_move<const T&>
{
	enum { can_move = false };

	static inline const T& move(const T& p0)
	{
		return p0;
	}
};

template <typename T>
struct try_move<T&>
{
	enum { can_move = false };

	static inline T& move(T& p0)
	{
		return p0;
	}
};

template <typename T>
struct try_move<const T&&>
{
	enum { can_move = false };

	static inline const T& move(const T& p0)
	{
		return p0;
	}
};

template <typename T>
struct try_move<T&&>
{
	enum { can_move = true };

	static inline T&& move(T& p0)
	{
		return (T&&)p0;
	}
};

//移除引用属性
#define RM_REF(__T__) typename std::remove_reference<__T__>::type
//移除类型本身的 const 属性
#define RM_CONST(__T__) typename std::remove_const<__T__>::type
//移除类型本身的引用属性，然后移除 const
#define RM_CREF(__T__) RM_CONST(RM_REF(__T__))

//移除指针
#define RM_PTR(__T__) typename std::remove_pointer<__T__>::type
//移除类型本身的指针，然后移除 const
#define RM_CPTR(__T__) RM_CONST(RM_PTR(__T__))
//移除类型本身的引用属性，再移除指针，然后移除 const
#define RM_CPTR_REF(__T__) RM_CONST(RM_PTR(RM_REF(__T__)))


//检测一个参数是否是右值传递，是就继续进行右值传递
#define TRY_MOVE(__P__) (decltype(__P__))(__P__)
//检测一个参数是否是右值传递
#define CAN_MOVE(__P__) try_move<decltype(__P__)>::can_move
//const属性强制转换为右值
#define FORCE_MOVE(__P__) (RM_CREF(decltype(__P__))&&)__P__
//////////////////////////////////////////////////////////////////////////

/*!
@brief  检测一个参数是否是为引用，是就转为引用包装，否则尝试右值传递
*/
template <typename ARG>
struct try_ref_move
{
	template <typename Arg>
	static inline Arg&& move(Arg&& p)
	{
		return (Arg&&)p;
	}
};

template <typename ARG>
struct try_ref_move<ARG&>
{
	static inline std::reference_wrapper<ARG> move(ARG& p)
	{
		return std::reference_wrapper<ARG>(p);
	}
};

//////////////////////////////////////////////////////////////////////////
template <typename T>
struct type_move_pipe
{
	typedef T type;
};

template <typename T>
struct type_move_pipe<const T&>
{
	typedef std::reference_wrapper<const T> type;
};

template <typename T>
struct type_move_pipe<T&>
{
	typedef std::reference_wrapper<T> type;
};

template <typename T>
struct type_move_pipe<const T&&>
{
	typedef const T&& type;
};

template <typename T>
struct type_move_pipe<T&&>
{
	typedef T&& type;
};

#define TYPE_PIPE(__T__) typename type_move_pipe<__T__>::type

#endif