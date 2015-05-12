#ifndef __STACK_OBJECT_H
#define __STACK_OBJECT_H

/*!
@brief 在栈上分配一段临时空间用于构造临时对象
*/
template <typename OBJ>
class stack_obj
{
public:
	stack_obj()
	{
		DEBUG_OPERATION(_null = true);
	}

	~stack_obj()
	{
		assert(_null);
	}
private:
	stack_obj(const stack_obj&){};
	void operator =(const stack_obj&){};
	void* operator new(size_t){ return 0; };
	void operator delete(void*){};
public:
	/*!
	@brief 构造一个临时对象
	*/
	template <typename T0, typename T1, typename T2, typename T3>
	void create(const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		assert(_null);
		DEBUG_OPERATION(_null = false);
		new(_buff)OBJ((T0&)p0, (T1&)p1, (T2&)p2, (T3&)p3);
	}

	template <typename T0, typename T1, typename T2>
	void create(const T0& p0, const T1& p1, const T2& p2)
	{
		assert(_null);
		DEBUG_OPERATION(_null = false);
		new(_buff)OBJ((T0&)p0, (T1&)p1, (T2&)p2);
	}

	template <typename T0, typename T1>
	void create(const T0& p0, const T1& p1)
	{
		assert(_null);
		DEBUG_OPERATION(_null = false);
		new(_buff)OBJ((T0&)p0, (T1&)p1);
	}

	template <typename T0>
	void create(const T0& p0)
	{
		assert(_null);
		DEBUG_OPERATION(_null = false);
		new(_buff)OBJ((T0&)p0);
	}

	void create()
	{
		assert(_null);
		DEBUG_OPERATION(_null = false);
		new(_buff)OBJ();
	}

	/*!
	@brief 销毁临时对象
	*/
	void destroy()
	{
		assert(!_null);
		DEBUG_OPERATION(_null = true);
		((OBJ*)_buff)->~OBJ();
	}

	/*!
	@brief 调用该临时对象方法
	*/
	OBJ* operator -> () const
	{
		assert(!_null);
		return (OBJ*)_buff;
	}
private:
	BYTE _buff[sizeof(OBJ)];
	DEBUG_OPERATION(bool _null);
};

#endif