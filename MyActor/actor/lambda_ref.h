#ifndef __LAMBDA_REF_H
#define __LAMBDA_REF_H

#include "scattered.h"

//用户内嵌lambda的外部变量引用捕获，可以减小直接"&捕获"sizeof(lambda)的大小，提高调度效率
#define LAMBDA_REF1(__NAME__, __P0__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
} __NAME__ = { __P0__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF2(__NAME__, __P0__, __P1__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
} __NAME__ = { __P0__, __P1__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF3(__NAME__, __P0__, __P1__, __P2__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
} __NAME__ = { __P0__, __P1__, __P2__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF4(__NAME__, __P0__, __P1__, __P2__, __P3__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF5(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF6(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF7(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
	typedef decltype(__P6__) BOND_LINE(type6, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
	BOND_LINE(type6, __LINE__)& __P6__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF8(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
	typedef decltype(__P6__) BOND_LINE(type6, __LINE__); \
	typedef decltype(__P7__) BOND_LINE(type7, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
	BOND_LINE(type6, __LINE__)& __P6__; \
	BOND_LINE(type7, __LINE__)& __P7__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF9(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
	typedef decltype(__P6__) BOND_LINE(type6, __LINE__); \
	typedef decltype(__P7__) BOND_LINE(type7, __LINE__); \
	typedef decltype(__P8__) BOND_LINE(type8, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
	BOND_LINE(type6, __LINE__)& __P6__; \
	BOND_LINE(type7, __LINE__)& __P7__; \
	BOND_LINE(type8, __LINE__)& __P8__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_REF10(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__)\
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
	typedef decltype(__P6__) BOND_LINE(type6, __LINE__); \
	typedef decltype(__P7__) BOND_LINE(type7, __LINE__); \
	typedef decltype(__P8__) BOND_LINE(type8, __LINE__); \
	typedef decltype(__P9__) BOND_LINE(type9, __LINE__); \
struct\
{\
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
	BOND_LINE(type6, __LINE__)& __P6__; \
	BOND_LINE(type7, __LINE__)& __P7__; \
	BOND_LINE(type8, __LINE__)& __P8__; \
	BOND_LINE(type9, __LINE__)& __P9__; \
} __NAME__ = { __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__ };
//////////////////////////////////////////////////////////////////////////

//用户内嵌lambda的外部变量引用捕获（外带当前this捕获，通过->访问），可以减小直接"&捕获"sizeof(lambda)的大小，提高调度效率
#define LAMBDA_THIS_REF1(__NAME__, __P0__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
} __NAME__ = { this, __P0__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF2(__NAME__, __P0__, __P1__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
} __NAME__ = { this, __P0__, __P1__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF3(__NAME__, __P0__, __P1__, __P2__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
} __NAME__ = { this, __P0__, __P1__, __P2__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF4(__NAME__, __P0__, __P1__, __P2__, __P3__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF5(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF6(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF7(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
	typedef decltype(__P6__) BOND_LINE(type6, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
	BOND_LINE(type6, __LINE__)& __P6__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF8(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
	typedef decltype(__P6__) BOND_LINE(type6, __LINE__); \
	typedef decltype(__P7__) BOND_LINE(type7, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
	BOND_LINE(type6, __LINE__)& __P6__; \
	BOND_LINE(type7, __LINE__)& __P7__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF9(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
	typedef decltype(__P6__) BOND_LINE(type6, __LINE__); \
	typedef decltype(__P7__) BOND_LINE(type7, __LINE__); \
	typedef decltype(__P8__) BOND_LINE(type8, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
	BOND_LINE(type6, __LINE__)& __P6__; \
	BOND_LINE(type7, __LINE__)& __P7__; \
	BOND_LINE(type8, __LINE__)& __P8__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__ };
//------------------------------------------------------------------------------------------

#define LAMBDA_THIS_REF10(__NAME__, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__)\
	typedef decltype(this) this_type; \
	typedef decltype(__P0__) BOND_LINE(type0, __LINE__); \
	typedef decltype(__P1__) BOND_LINE(type1, __LINE__); \
	typedef decltype(__P2__) BOND_LINE(type2, __LINE__); \
	typedef decltype(__P3__) BOND_LINE(type3, __LINE__); \
	typedef decltype(__P4__) BOND_LINE(type4, __LINE__); \
	typedef decltype(__P5__) BOND_LINE(type5, __LINE__); \
	typedef decltype(__P6__) BOND_LINE(type6, __LINE__); \
	typedef decltype(__P7__) BOND_LINE(type7, __LINE__); \
	typedef decltype(__P8__) BOND_LINE(type8, __LINE__); \
	typedef decltype(__P9__) BOND_LINE(type9, __LINE__); \
struct\
{\
	this_type get(){ return __this; }\
	this_type operator->(){ return __this; }\
	this_type __this; \
	BOND_LINE(type0, __LINE__)& __P0__; \
	BOND_LINE(type1, __LINE__)& __P1__; \
	BOND_LINE(type2, __LINE__)& __P2__; \
	BOND_LINE(type3, __LINE__)& __P3__; \
	BOND_LINE(type4, __LINE__)& __P4__; \
	BOND_LINE(type5, __LINE__)& __P5__; \
	BOND_LINE(type6, __LINE__)& __P6__; \
	BOND_LINE(type7, __LINE__)& __P7__; \
	BOND_LINE(type8, __LINE__)& __P8__; \
	BOND_LINE(type9, __LINE__)& __P9__; \
} __NAME__ = { this, __P0__, __P1__, __P2__, __P3__, __P4__, __P5__, __P6__, __P7__, __P8__, __P9__ };

#endif