#ifndef __WRAPPED_CAPTURE_H
#define __WRAPPED_CAPTURE_H

#include "try_move.h"
#include <boost/atomic/atomic.hpp>

template <typename H, typename T0, typename T1 = void, typename T2 = void, typename T3 = void>
struct wrapped_capture 
{
	template <typename Handler, typename TP0, typename TP1, typename TP2, typename TP3>
	wrapped_capture(Handler&& h, TP0&& p0, TP1&& p1, TP2&& p2, TP3&& p3)
		:_h(TRY_MOVE(h)), _p0(TRY_MOVE(p0)), _p1(TRY_MOVE(p1)), _p2(TRY_MOVE(p2)), _p3(TRY_MOVE(p3)) {}

	wrapped_capture(wrapped_capture&& s)
		:_h(std::move(s._h)), _p0(std::move(s._p0)), _p1(std::move(s._p1)), _p2(std::move(s._p2)), _p3(std::move(s._p3)){}

	void operator =(wrapped_capture&& s)
	{
		_h = std::move(s._h);
		_p0 = std::move(s._p0);
		_p1 = std::move(s._p1);
		_p2 = std::move(s._p2);
		_p3 = std::move(s._p3);
	}

	void operator ()()
	{
		_h(_p0, _p1, _p2, _p3);
	}

	H _h;
	T0 _p0;
	T1 _p1;
	T2 _p2;
	T3 _p3;
};

template <typename H, typename T0, typename T1, typename T2>
struct wrapped_capture<H, T0, T1, T2, void>
{
	template <typename Handler, typename TP0, typename TP1, typename TP2>
	wrapped_capture(Handler&& h, TP0&& p0, TP1&& p1, TP2&& p2)
		:_h(TRY_MOVE(h)), _p0(TRY_MOVE(p0)), _p1(TRY_MOVE(p1)), _p2(TRY_MOVE(p2)) {}

	wrapped_capture(wrapped_capture&& s)
		:_h(std::move(s._h)), _p0(std::move(s._p0)), _p1(std::move(s._p1)), _p2(std::move(s._p2)) {}

	void operator =(wrapped_capture&& s)
	{
		_h = std::move(s._h);
		_p0 = std::move(s._p0);
		_p1 = std::move(s._p1);
		_p2 = std::move(s._p2);
	}

	void operator ()()
	{
		_h(_p0, _p1, _p2);
	}

	H _h;
	T0 _p0;
	T1 _p1;
	T2 _p2;
};

template <typename H, typename T0, typename T1>
struct wrapped_capture<H, T0, T1, void, void>
{
	template <typename Handler, typename TP0, typename TP1>
	wrapped_capture(Handler&& h, TP0&& p0, TP1&& p1)
		:_h(TRY_MOVE(h)), _p0(TRY_MOVE(p0)), _p1(TRY_MOVE(p1)) {}

	wrapped_capture(wrapped_capture&& s)
		:_h(std::move(s._h)), _p0(std::move(s._p0)), _p1(std::move(s._p1)) {}

	void operator =(wrapped_capture&& s)
	{
		_h = std::move(s._h);
		_p0 = std::move(s._p0);
		_p1 = std::move(s._p1);
	}

	void operator ()()
	{
		_h(_p0, _p1);
	}

	H _h;
	T0 _p0;
	T1 _p1;
};

template <typename H, typename T0>
struct wrapped_capture<H, T0, void, void, void>
{
	template <typename Handler, typename TP0>
	wrapped_capture(Handler&& h, TP0&& p0)
		:_h(TRY_MOVE(h)), _p0(TRY_MOVE(p0)) {}

	wrapped_capture(wrapped_capture&& s)
		:_h(std::move(s._h)), _p0(std::move(s._p0)) {}

	void operator =(wrapped_capture&& s)
	{
		_h = std::move(s._h);
		_p0 = std::move(s._p0);
	}

	void operator ()()
	{
		_h(_p0);
	}

	H _h;
	T0 _p0;
};

template <typename Handler, typename TP0, typename TP1, typename TP2, typename TP3>
wrapped_capture<RM_REF(Handler), TP0, TP1, TP2, TP3> wrap_capture(Handler&& h, TP0&& p0, TP1&& p1, TP2&& p2, TP3&& p3)
{
	return wrapped_capture<RM_REF(Handler), TP0, TP1, TP2, TP3>(TRY_MOVE(h), TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2), TRY_MOVE(p3));
}

template <typename Handler, typename TP0, typename TP1, typename TP2>
wrapped_capture<RM_REF(Handler), TP0, TP1, TP2> wrap_capture(Handler&& h, TP0&& p0, TP1&& p1, TP2&& p2)
{
	return wrapped_capture<RM_REF(Handler), TP0, TP1, TP2>(TRY_MOVE(h), TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2)());
}

template <typename Handler, typename TP0, typename TP1>
wrapped_capture<RM_REF(Handler), TP0, TP1> wrap_capture(Handler&& h, TP0&& p0, TP1&& p1)
{
	return wrapped_capture<RM_REF(Handler), TP0, TP1>(TRY_MOVE(h), TRY_MOVE(p0), TRY_MOVE(p1));
}

template <typename Handler, typename TP0>
wrapped_capture<RM_REF(Handler), TP0> wrap_capture(Handler&& h, TP0&& p0)
{
	return wrapped_capture<RM_REF(Handler), TP0>(TRY_MOVE(h), TRY_MOVE(p0));
}

#endif