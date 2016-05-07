#ifndef __LAMBDA_REF_H
#define __LAMBDA_REF_H

//用户内嵌lambda的外部变量引用捕获，可以减小直接"&捕获"sizeof(lambda)的大小，提高调度效率
#define LAMBDA_REF1(__NAME__, __P0__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
} __NAME__ = { __P0__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF2(__NAME__, __P0__, __P1__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
} __NAME__ = { __P0__, __P1__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF3(__NAME__, __P0__, __P1__, __P2__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
} __NAME__ = { __P0__, __P1__, __P2__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF4(__NAME__, __P0__, __P1__, __P2__, __P3__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF5(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF6(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF7(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
	typedef decltype(__P6__) _BOND_LR_(type6, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
	_BOND_LR_(type6, __LINE__)& __P6__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF8(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
	typedef decltype(__P6__) _BOND_LR_(type6, __LINE__); \
	typedef decltype(__P7__) _BOND_LR_(type7, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
	_BOND_LR_(type6, __LINE__)& __P6__; \
	_BOND_LR_(type7, __LINE__)& __P7__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF9(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
	typedef decltype(__P6__) _BOND_LR_(type6, __LINE__); \
	typedef decltype(__P7__) _BOND_LR_(type7, __LINE__); \
	typedef decltype(__P8__) _BOND_LR_(type8, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
	_BOND_LR_(type6, __LINE__)& __P6__; \
	_BOND_LR_(type7, __LINE__)& __P7__; \
	_BOND_LR_(type8, __LINE__)& __P8__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF10(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__)\
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
	typedef decltype(__P6__) _BOND_LR_(type6, __LINE__); \
	typedef decltype(__P7__) _BOND_LR_(type7, __LINE__); \
	typedef decltype(__P8__) _BOND_LR_(type8, __LINE__); \
	typedef decltype(__P9__) _BOND_LR_(type9, __LINE__); \
struct\
{\
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
	_BOND_LR_(type6, __LINE__)& __P6__; \
	_BOND_LR_(type7, __LINE__)& __P7__; \
	_BOND_LR_(type8, __LINE__)& __P8__; \
	_BOND_LR_(type9, __LINE__)& __P9__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__ };
//////////////////////////////////////////////////////////////////////////

//用户内嵌lambda的外部变量引用捕获（外带当前this捕获，通过->访问），可以减小直接"&捕获"sizeof(lambda)的大小，提高调度效率
#define LAMBDA_THIS_REF1(__NAME__, __P0__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
} __NAME__ = { this, __P0__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF2(__NAME__, __P0__, __P1__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
} __NAME__ = { this, __P0__, __P1__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF3(__NAME__, __P0__, __P1__, __P2__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
} __NAME__ = { this, __P0__, __P1__, __P2__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF4(__NAME__, __P0__, __P1__, __P2__, __P3__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF5(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF6(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF7(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
	typedef decltype(__P6__) _BOND_LR_(type6, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
	_BOND_LR_(type6, __LINE__)& __P6__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF8(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
	typedef decltype(__P6__) _BOND_LR_(type6, __LINE__); \
	typedef decltype(__P7__) _BOND_LR_(type7, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
	_BOND_LR_(type6, __LINE__)& __P6__; \
	_BOND_LR_(type7, __LINE__)& __P7__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF9(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
	typedef decltype(__P6__) _BOND_LR_(type6, __LINE__); \
	typedef decltype(__P7__) _BOND_LR_(type7, __LINE__); \
	typedef decltype(__P8__) _BOND_LR_(type8, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
	_BOND_LR_(type6, __LINE__)& __P6__; \
	_BOND_LR_(type7, __LINE__)& __P7__; \
	_BOND_LR_(type8, __LINE__)& __P8__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF10(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) _BOND_LR_(type0, __LINE__); \
	typedef decltype(__P1__) _BOND_LR_(type1, __LINE__); \
	typedef decltype(__P2__) _BOND_LR_(type2, __LINE__); \
	typedef decltype(__P3__) _BOND_LR_(type3, __LINE__); \
	typedef decltype(__P4__) _BOND_LR_(type4, __LINE__); \
	typedef decltype(__P5__) _BOND_LR_(type5, __LINE__); \
	typedef decltype(__P6__) _BOND_LR_(type6, __LINE__); \
	typedef decltype(__P7__) _BOND_LR_(type7, __LINE__); \
	typedef decltype(__P8__) _BOND_LR_(type8, __LINE__); \
	typedef decltype(__P9__) _BOND_LR_(type9, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	_BOND_LR_(type0, __LINE__)& __P0__; \
	_BOND_LR_(type1, __LINE__)& __P1__; \
	_BOND_LR_(type2, __LINE__)& __P2__; \
	_BOND_LR_(type3, __LINE__)& __P3__; \
	_BOND_LR_(type4, __LINE__)& __P4__; \
	_BOND_LR_(type5, __LINE__)& __P5__; \
	_BOND_LR_(type6, __LINE__)& __P6__; \
	_BOND_LR_(type7, __LINE__)& __P7__; \
	_BOND_LR_(type8, __LINE__)& __P8__; \
	_BOND_LR_(type9, __LINE__)& __P9__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__ };

#define _PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, N, ...) N
#define _PP_NARG(...) _BOND_LR(_PP_ARG_N(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),)

#define _BOND_LR(a, b) a##b
#define _BOND_LR_(a, b) _BOND_LR(a, b)
#define _BOND_LR__(a, b) _BOND_LR_(a, b)
//用户内嵌lambda的外部变量引用捕获，可以减小直接"&捕获"sizeof(lambda)的大小，提高调度效率
#define LAMBDA_REF(__NAME__, ...) _BOND_LR__(LAMBDA_REF, _PP_NARG(__VA_ARGS__))(__NAME__, __VA_ARGS__)
//用户内嵌lambda的外部变量引用捕获（外带当前this捕获，通过->访问），可以减小直接"&捕获"sizeof(lambda)的大小，提高调度效率
#define LAMBDA_THIS_REF(__NAME__, ...) _BOND_LR__(LAMBDA_THIS_REF, _PP_NARG(__VA_ARGS__))(__NAME__, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

template <typename... _Types>
struct LocalRecursive_;
template <typename... _Types>
struct LocalRecursiveFace_;
template <typename Handler, typename _Rt, typename... _Types>
struct LocalRecursiveInvoker_;

template <typename _Rt, typename... _Types>
struct LocalRecursiveFace_<_Rt(_Types...)>
{
	virtual _Rt invoke(_Types...) = 0;
	virtual void destroy() = 0;
};

template <typename Handler, typename _Rt, typename... _Types>
struct LocalRecursiveInvoker_<Handler, _Rt(_Types...)> : public LocalRecursiveFace_<_Rt(_Types...)>
{
	LocalRecursiveInvoker_(Handler& handler)
		:_handler(handler) {}

	_Rt invoke(_Types... args)
	{
		return _handler(std::forward<_Types>(args)...);
	}

	void destroy()
	{
		this->~LocalRecursiveInvoker_();
		DEBUG_OPERATION(memset(this, 0xcf, sizeof(*this)));
	}

	Handler& _handler;
};

template <typename _Rt, typename... _Types>
struct LocalRecursive_<_Rt(_Types...)>
{
	typedef LocalRecursiveFace_<_Rt(_Types...)> func_type;
	LocalRecursive_() {}
	~LocalRecursive_() { assert(!_depth && !_has); }

	template <typename... Args>
	_Rt operator()(Args&&... args)
	{
		DEBUG_OPERATION(_depth++);
		DEBUG_OPERATION(BREAK_OF_SCOPE({ _depth--; }));
		assert(_has);
		return ((func_type*)_space)->invoke(std::forward<Args>(args)...);
	}

	template <typename Handler>
	void set_handler(Handler& handler)
	{
		typedef LocalRecursiveInvoker_<Handler, _Rt(_Types...)> invoker_type;
		static_assert(sizeof(_space) == sizeof(invoker_type), "");
		destroy();
		new(_space)invoker_type(handler);
		_has = true;
	}

	void destroy()
	{
		assert(0 == _depth);
		if (_has)
		{
			((func_type*)_space)->destroy();
			_has = false;
		}
	}

	DEBUG_OPERATION(size_t _depth = 0);
	__space_align char _space[sizeof(LocalRecursiveInvoker_<null_handler<>, void()>)];
	bool _has = false;
	NONE_COPY(LocalRecursive_);
};

#define DEFINE_LOCAL_RECURSIVE(__name__, __type__)\
	LocalRecursive_<__type__> __name__;

#define _SET_RECURSIVE_FUNC(__name__, __lmd__)\
	const auto NAME_BOND(__lambda_, __name__) = __lmd__; \
	__name__.set_handler(NAME_BOND(__lambda_, __name__));

#define SET_RECURSIVE_FUNC(__name__, __lmd__)\
	_SET_RECURSIVE_FUNC(__name__, __lmd__); \
	BREAK_OF_SCOPE({ __name__.destroy(); });

#define LOCAL_RECURSIVE1(__name__, __type__, __lmd__)\
	DEFINE_LOCAL_RECURSIVE(__name__, __type__); \
	_SET_RECURSIVE_FUNC(__name__, __lmd__) \
	BREAK_OF_SCOPE({ __name__.destroy(); });

#define LOCAL_RECURSIVE2(__name1__, __name2__, __type1__, __type2__, __lmd1__, __lmd2__)\
	DEFINE_LOCAL_RECURSIVE(__name1__, __type1__); \
	DEFINE_LOCAL_RECURSIVE(__name2__, __type2__); \
	_SET_RECURSIVE_FUNC(__name1__, __lmd1__) \
	_SET_RECURSIVE_FUNC(__name2__, __lmd2__) \
	BREAK_OF_SCOPE({ __name1__.destroy(); __name2__.destroy(); });

#define LOCAL_RECURSIVE3(__name1__, __name2__, __name3__, __type1__, __type2__, __type3__, __lmd1__, __lmd2__, __lmd3__)\
	DEFINE_LOCAL_RECURSIVE(__name1__, __type1__); \
	DEFINE_LOCAL_RECURSIVE(__name2__, __type2__); \
	DEFINE_LOCAL_RECURSIVE(__name3__, __type3__); \
	_SET_RECURSIVE_FUNC(__name1__, __lmd1__) \
	_SET_RECURSIVE_FUNC(__name2__, __lmd2__) \
	_SET_RECURSIVE_FUNC(__name3__, __lmd3__) \
	BREAK_OF_SCOPE({ __name1__.destroy(); __name2__.destroy(); __name3__.destroy(); });

#define LOCAL_ACTOR1(__name__, __lmd__)\
	LOCAL_RECURSIVE1(__name__, void(my_actor*), __lmd__)

#define LOCAL_ACTOR2(__name1__, __name2__, __lmd1__, __lmd2__)\
	LOCAL_RECURSIVE2(__name1__, __name2__, void(my_actor*), void(my_actor*), __lmd1__, __lmd2__)

#define LOCAL_ACTOR3(__name1__, __name2__, __name3__, __lmd1__, __lmd2__, __lmd3__)\
	LOCAL_RECURSIVE3(__name1__, __name2__, __name3__, void(my_actor*), void(my_actor*), void(my_actor*), __lmd1__, __lmd2__, __lmd3__)

#define BEGIN_RECURSIVE(__name__, __ret__, __type__)\
	LocalRecursive_<__ret__ __type__> __name__; \
	const auto NAME_BOND(__lambda_, __name__) = [&]__type__{

#define END_RECURSIVE(__name__) }; \
	__name__.set_handler(NAME_BOND(__lambda_, __name__)); \
	BREAK_OF_SCOPE({ __name__.destroy(); });

#define BEGIN_RECURSIVE_ACTOR(__name__) BEGIN_RECURSIVE(__name__, void, (my_actor* self))

#define END_RECURSIVE_ACTOR(__name__)  END_RECURSIVE(__name__)

#if (_DEBUG || DEBUG)
#define DEBUG_LOCAL_RECURSIVE(__name__, __type__)\
	std::function<__type__> __name__
#endif

#define WRAP_LAMBDA_REF(__name__, __lmd__)\
	auto NAME_BOND(__lambda, __name__) = __lmd__; \
	auto __name__ = wrap_ref_handler(NAME_BOND(__lambda, __name__));

#endif