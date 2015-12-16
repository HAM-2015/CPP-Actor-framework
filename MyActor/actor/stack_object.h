#ifndef __STACK_OBJECT_H
#define __STACK_OBJECT_H

#include "scattered.h"

/*!
@brief 在栈上分配一段临时空间用于构造临时对象
*/
template <typename Type = void, bool AUTO = true>
class stack_obj
{
	typedef TYPE_PIPE(Type) type;
public:
	stack_obj()
	{
		_null = true;
	}

	~stack_obj()
	{
		destroy();
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
	template <typename... Args>
	void create(Args&&... args)
	{
		if (!_null)
		{
			free();
		}
		_null = false;
		new(_buff)type(TRY_MOVE(args)...);
	}

	void create()
	{
		if (!_null)
		{
			free();
		}
		_null = false;
		new(_buff)type();
	}

	template <typename PT0>
	void operator =(PT0&& p0)
	{
		if (!_null)
		{
			free();
		}
		_null = false;
		new(_buff)type(TRY_MOVE(p0));
	}

	bool has() const
	{
		return !_null;
	}

	/*!
	@brief 销毁临时对象
	*/
	void destroy()
	{
		if (!_null)
		{
			free();
			_null = true;
		}
	}

	Type& get()
	{
		assert(!_null);
		return *(type*)_buff;
	}

	Type& operator *()
	{
		assert(!_null);
		return *(type*)_buff;
	}

	/*!
	@brief 调用该临时对象方法
	*/
	RM_REF(Type)* operator -> ()
	{
		assert(!_null);
		return &get();
	}
private:
	void free()
	{
		assert(!_null);
		((type*)_buff)->~type();
#if (_DEBUG || DEBUG)
		memset(_buff, 0xcf, sizeof(_buff));
#endif
	}
private:
	unsigned char _buff[sizeof(type)];
	bool _null;
};

template <typename Type>
class stack_obj<Type, false>
{
	typedef TYPE_PIPE(Type) type;
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
	template <typename... Args>
	void create(Args&&... args)
	{
		assert(_null);
		DEBUG_OPERATION(_null = false);
		new(_buff)type(TRY_MOVE(args)...);
	}

	void create()
	{
		assert(_null);
		DEBUG_OPERATION(_null = false);
		new(_buff)type();
	}

	template <typename PT0>
	void operator =(PT0&& p0)
	{
		assert(_null);
		DEBUG_OPERATION(_null = false);
		new(_buff)type(TRY_MOVE(p0));
	}

	/*!
	@brief 销毁临时对象
	*/
	void destroy()
	{
		assert(!_null);
		DEBUG_OPERATION(_null = true);
		((type*)_buff)->~type();
#if (_DEBUG || DEBUG)
		memset(_buff, 0xcf, sizeof(_buff));
#endif
	}

	Type& get()
	{
		assert(!_null);
		return *(type*)_buff;
	}

	Type& operator *()
	{
		assert(!_null);
		return *(type*)_buff;
	}

	/*!
	@brief 调用该临时对象方法
	*/
	RM_REF(Type)* operator -> ()
	{
		assert(!_null);
		return &get();
	}
private:
	unsigned char _buff[sizeof(type)];
	DEBUG_OPERATION(bool _null);
};

template <bool AUTO>
class stack_obj<void, AUTO>
{
public:
	stack_obj()
	{
		_null = true;
	}

	~stack_obj()
	{
		destroy();
	}

	void create()
	{
		_null = false;
	}

	bool has() const
	{
		return !_null;
	}
	void destroy()
	{
		_null = true;
	}

	void get()
	{
	}

	bool _null;
};

template <typename Type>
struct check_stack_obj_type
{
	typedef Type type;
};

template <typename Type, bool AUTO>
struct check_stack_obj_type<stack_obj<Type, AUTO>>
{
	typedef Type type;
};

#endif