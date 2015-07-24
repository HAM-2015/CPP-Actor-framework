#ifndef __WRAPPED_CAPTURE_H
#define __WRAPPED_CAPTURE_H

#include "try_move.h"
#include <boost/atomic/atomic.hpp>

#if _MSC_VER == 1600

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
wrapped_capture<RM_REF(Handler), RM_CREF(TP0), RM_CREF(TP1), RM_CREF(TP2), RM_CREF(TP3)>
wrap_capture(Handler&& h, TP0&& p0, TP1&& p1, TP2&& p2, TP3&& p3)
{
	return wrapped_capture<RM_REF(Handler), RM_CREF(TP0), RM_CREF(TP1), RM_CREF(TP2), RM_CREF(TP3)>
		(TRY_MOVE(h), TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2), TRY_MOVE(p3));
}

template <typename Handler, typename TP0, typename TP1, typename TP2>
wrapped_capture<RM_REF(Handler), RM_CREF(TP0), RM_CREF(TP1), RM_CREF(TP2)> wrap_capture(Handler&& h, TP0&& p0, TP1&& p1, TP2&& p2)
{
	return wrapped_capture<RM_REF(Handler), RM_CREF(TP0), RM_CREF(TP1), RM_CREF(TP2)>
		(TRY_MOVE(h), TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2)());
}

template <typename Handler, typename TP0, typename TP1>
wrapped_capture<RM_REF(Handler), RM_CREF(TP0), RM_CREF(TP1)> wrap_capture(Handler&& h, TP0&& p0, TP1&& p1)
{
	return wrapped_capture<RM_REF(Handler), RM_CREF(TP0), RM_CREF(TP1)>(TRY_MOVE(h), TRY_MOVE(p0), TRY_MOVE(p1));
}

template <typename Handler, typename TP0>
wrapped_capture<RM_REF(Handler), RM_CREF(TP0)> wrap_capture(Handler&& h, TP0&& p0)
{
	return wrapped_capture<RM_REF(Handler), RM_CREF(TP0)>(TRY_MOVE(h), TRY_MOVE(p0));
}

#elif _MSC_VER > 1600

#include <tuple>

template <typename R, size_t N>
struct apply_arg
{
	template <typename H, typename TUPLE, typename... Args>
	static inline R append(H&& h, TUPLE&& tp, Args&&... args)
	{
		return apply_arg<R, N - 1>::append(TRY_MOVE(h), TRY_MOVE(tp), std::get<N - 1>(tp), TRY_MOVE(args)...);
	}
};

template <typename R>
struct apply_arg<R, 0>
{
	template <typename H, typename TUPLE, typename... Args>
	static inline R append(H&& h, TUPLE&&, Args&&... args)
	{
		return h(args...);
	}
};

template <typename R, typename H, typename TUPLE>
inline R tuple_invoke(H&& h, TUPLE&& t)
{
	static_assert(std::tuple_size<RM_REF(TUPLE)>::value <= 4, "up to 4");
	return apply_arg<R, std::tuple_size<RM_REF(TUPLE)>::value>::append(TRY_MOVE(h), TRY_MOVE(t));
}

template <typename H, typename... ARGS>
struct wrapped_capture 
{
	template <typename Handler, typename... Args>
	wrapped_capture(Handler&& h, Args&&... args)
		:_h(TRY_MOVE(h)), _args(TRY_MOVE(args)...) {}

	wrapped_capture(wrapped_capture&& s)
		:_h(std::move(s._h)), _args(std::move(s._args)) {}

	void operator =(wrapped_capture&& s)
	{
		_h = std::move(s._h);
		_args = std::move(s._args);
	}

	void operator ()()
	{
		tuple_invoke<void>(_h, _args);
	}

	H _h;
	std::tuple<ARGS...> _args;
};

template <typename Handler, typename... Args>
wrapped_capture<RM_REF(Handler), RM_CREF(Args)...> wrap_capture(Handler&& h, Args&&... args)
{
	static_assert(sizeof...(Args) <= 4, "up to 4");
	return wrapped_capture<RM_REF(Handler), RM_CREF(Args)...>(TRY_MOVE(h), TRY_MOVE(args)...);
}

#endif

#endif