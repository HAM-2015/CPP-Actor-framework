#ifndef __REF_EX_H
#define __REF_EX_H


template <typename T0, typename T1 = void, typename T2 = void, typename T3 = void>
struct const_ref_ex
{
	const_ref_ex(const T0& p0, const T1& p1, const T2& p2, const T3& p3)
		:_p0(p0), _p1(p1), _p2(p2), _p3(p3)
	{

	}

	const_ref_ex(const const_ref_ex& s)
		:_p0(s._p0), _p1(s._p1), _p2(s._p2), _p3(s._p3)
	{

	}

	const T0& _p0;
	const T1& _p1;
	const T2& _p2;
	const T3& _p3;
};

template <typename T0, typename T1, typename T2>
struct const_ref_ex<T0, T1, T2, void>
{
	const_ref_ex(const T0& p0, const T1& p1, const T2& p2)
		:_p0(p0), _p1(p1), _p2(p2)
	{

	}

	const_ref_ex(const const_ref_ex& s)
		:_p0(s._p0), _p1(s._p1), _p2(s._p2)
	{

	}

	const T0& _p0;
	const T1& _p1;
	const T2& _p2;
};

template <typename T0, typename T1>
struct const_ref_ex<T0, T1, void, void>
{
	const_ref_ex(const T0& p0, const T1& p1)
		:_p0(p0), _p1(p1)
	{

	}

	const_ref_ex(const const_ref_ex& s)
		:_p0(s._p0), _p1(s._p1)
	{

	}

	const T0& _p0;
	const T1& _p1;
};

template <typename T0>
struct const_ref_ex<T0, void, void, void>
{
	const_ref_ex(const T0& p0)
		:_p0(p0)
	{

	}

	const_ref_ex(const const_ref_ex& s)
		:_p0(s._p0)
	{

	}

	const T0& _p0;
};
//////////////////////////////////////////////////////////////////////////

template <typename T0, typename T1 = void, typename T2 = void, typename T3 = void>
struct ref_ex
{
	ref_ex(T0& p0, T1& p1, T2& p2, T3& p3)
		:_p0(p0), _p1(p1), _p2(p2), _p3(p3)
	{

	}

	ref_ex(const ref_ex& s)
		:_p0(s._p0), _p1(s._p1), _p2(s._p2), _p3(s._p3)
	{

	}

	void operator =(const ref_ex& s)
	{
		_p0 = s._p0;
		_p1 = s._p1;
		_p2 = s._p2;
		_p3 = s._p3;
	}

	void operator =(const const_ref_ex<T0, T1, T2, T3>& s)
	{
		_p0 = s._p0;
		_p1 = s._p1;
		_p2 = s._p2;
		_p3 = s._p3;
	}

	void operator =(ref_ex&& s)
	{
		_p0 = std::move(s._p0);
		_p1 = std::move(s._p1);
		_p2 = std::move(s._p2);
		_p3 = std::move(s._p3);
	}

	T0& _p0;
	T1& _p1;
	T2& _p2;
	T3& _p3;
};

template <typename T0, typename T1, typename T2>
struct ref_ex<T0, T1, T2, void>
{
	ref_ex(T0& p0, T1& p1, T2& p2)
		:_p0(p0), _p1(p1), _p2(p2)
	{

	}

	ref_ex(const ref_ex& s)
		:_p0(s._p0), _p1(s._p1), _p2(s._p2)
	{

	}

	void operator =(const ref_ex& s)
	{
		_p0 = s._p0;
		_p1 = s._p1;
		_p2 = s._p2;
	}

	void operator =(const const_ref_ex<T0, T1, T2>& s)
	{
		_p0 = s._p0;
		_p1 = s._p1;
		_p2 = s._p2;
	}

	void operator =(ref_ex&& s)
	{
		_p0 = std::move(s._p0);
		_p1 = std::move(s._p1);
		_p2 = std::move(s._p2);
	}

	T0& _p0;
	T1& _p1;
	T2& _p2;
};

template <typename T0, typename T1>
struct ref_ex<T0, T1, void, void>
{
	ref_ex(T0& p0, T1& p1)
		:_p0(p0), _p1(p1)
	{

	}

	ref_ex(const ref_ex& s)
		:_p0(s._p0), _p1(s._p1)
	{

	}

	void operator =(const ref_ex& s)
	{
		_p0 = s._p0;
		_p1 = s._p1;
	}

	void operator =(const const_ref_ex<T0, T1>& s)
	{
		_p0 = s._p0;
		_p1 = s._p1;
	}

	void operator =(ref_ex&& s)
	{
		_p0 = std::move(s._p0);
		_p1 = std::move(s._p1);
	}

	T0& _p0;
	T1& _p1;
};

template <typename T0>
struct ref_ex<T0, void, void, void>
{
	ref_ex(T0& p0)
		:_p0(p0)
	{

	}

	ref_ex(const ref_ex& s)
		:_p0(s._p0)
	{

	}

	void operator =(const ref_ex& s)
	{
		_p0 = s._p0;
	}

	void operator =(const const_ref_ex<T0>& s)
	{
		_p0 = s._p0;
	}

	void operator =(ref_ex&& s)
	{
		_p0 = std::move(s._p0);
	}

	T0& _p0;
};

#endif