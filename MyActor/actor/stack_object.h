#ifndef __STACK_OBJECT_H
#define __STACK_OBJECT_H

#include "scattered.h"

/*!
@brief 在栈上分配一段临时空间用于构造临时对象
*/
template <typename Type = void, bool AutoDestroy = true>
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
	void* operator new(size_t){ return NULL; };
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
		new(_space)type(std::forward<Args>(args)...);
		_null = false;
	}

	template <typename Arg>
	void operator =(Arg&& arg)
	{
		create(std::forward<Arg>(arg));
	}

	operator Type&()
	{
		return *(type*)_space;
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
		return *(type*)_space;
	}

	Type& operator *()
	{
		assert(!_null);
		return *(type*)_space;
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
		((type*)_space)->~type();
#if (_DEBUG || DEBUG)
		memset(_space, 0xcf, sizeof(_space));
#endif
	}
private:
	__space_align char _space[sizeof(type)];
	bool _null;
	NONE_COPY(stack_obj);
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
	void* operator new(size_t){ return NULL; };
	void operator delete(void*){};
public:
	/*!
	@brief 构造一个临时对象
	*/
	template <typename... Args>
	void create(Args&&... args)
	{
		assert(_null);
		new(_space)type(std::forward<Args>(args)...);
		DEBUG_OPERATION(_null = false);
	}

	template <typename Arg>
	void operator =(Arg&& arg)
	{
		create(std::forward<Arg>(arg));
	}

	operator Type&()
	{
		return *(type*)_space;
	}

	/*!
	@brief 销毁临时对象
	*/
	void destroy()
	{
		assert(!_null);
		DEBUG_OPERATION(_null = true);
		((type*)_space)->~type();
#if (_DEBUG || DEBUG)
		memset(_space, 0xcf, sizeof(_space));
#endif
	}

	Type& get()
	{
		assert(!_null);
		return *(type*)_space;
	}

	Type& operator *()
	{
		assert(!_null);
		return *(type*)_space;
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
	__space_align char _space[sizeof(type)];
	DEBUG_OPERATION(bool _null);
	NONE_COPY(stack_obj);
};

template <bool AutoDestroy>
class stack_obj<void, AutoDestroy>
{
private:
	void* operator new(size_t){ return NULL; };
	void operator delete(void*){};
public:
	stack_obj()
	{
		_null = true;
	}

	~stack_obj()
	{
		destroy();
	}

	template <typename... Args>
	void create(Args&&... args)
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
	NONE_COPY(stack_obj);
};

template <typename Type>
struct check_stack_obj_type
{
	typedef Type type;
};

template <typename Type, bool AutoDestroy>
struct check_stack_obj_type<stack_obj<Type, AutoDestroy>>
{
	typedef Type type;
};

#endif