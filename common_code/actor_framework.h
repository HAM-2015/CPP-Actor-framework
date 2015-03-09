/*!
 @header     actor_framework.h
 @abstract   并发逻辑控制框架(Actor Model)，使用"协程(coroutine)"技术，依赖boost_1.55或更新;
 @discussion 一个Actor对象(actor_handle)依赖一个shared_strand(二级调度器，本身依赖于io_service)，多个Actor可以共同依赖同一个shared_strand;
             支持强制结束、挂起/恢复、延时、多子任务(并发控制);
             在Actor中或所依赖的io_service中进行长时间阻塞的操作或重量级运算，会严重影响依赖同一个io_service的Actor响应速度;
             默认Actor栈空间64k字节，远比线程栈小，注意局部变量占用的空间以及调用层次(注意递归).
 @copyright  Copyright (c) 2015 HAM, E-Mail:591170887@qq.com
 */

#ifndef __ACTOR_FRAMEWORK_H
#define __ACTOR_FRAMEWORK_H

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <list>
#include "ios_proxy.h"
#include "shared_strand.h"
#include "wrapped_trig_handler.h"
#include "ref_ex.h"

class boost_actor;
typedef boost::shared_ptr<boost_actor> actor_handle;//Actor句柄

using namespace std;

//此函数会进入Actor中断标记，使用时注意逻辑的“连续性”可能会被打破
#define __yield_interrupt

/*!
@brief 用于检测某个不能被强制退出的函数是否被强制退出了
*/
struct check_force_quit 
{
	check_force_quit();

	~check_force_quit();

	void reset();
private:
	bool _force_quit;
};

// Actor内使用，在使用了Actor函数的异常捕捉 catch (...) 之前用于过滤Actor退出异常并继续抛出，不然可能导致程序崩溃
#define CATCH_ACTOR_QUIT()\
catch (boost_actor::actor_force_quit& e)\
{\
	throw e;\
}

#ifdef _DEBUG
#define DEBUG_OPERATION(__exp__)	__exp__
#else
#define DEBUG_OPERATION(__exp__)
#endif

//默认堆栈大小64k
#define kB	*1024
#define DEFAULT_STACKSIZE	64 kB

struct msg_param_base
{
	virtual ~msg_param_base() { }
};


template <typename T0, typename T1 = void, typename T2 = void, typename T3 = void>
struct msg_param: public msg_param_base
{
	typedef boost::function<void (T0, T1, T2, T3)> overflow_notify;
	typedef ref_ex<T0, T1, T2, T3> ref_type;
	typedef const_ref_ex<T0, T1, T2, T3> const_ref_type;

	msg_param()
	{

	}

	msg_param(const T0& p0, const T1& p1, const T2& p2, const T3& p3)
		:_res0(p0), _res1(p1), _res2(p2), _res3(p3)
	{

	}

	msg_param(const const_ref_type& rp)
		:_res0(rp._p0), _res1(rp._p1), _res2(rp._p2), _res3(rp._p3)
	{

	}

	static void proxy_notify(const overflow_notify& nfy, const msg_param& p)
	{
		nfy(p._res0, p._res1, p._res2, p._res3);
	}

	void get_param(ref_type& rp) const
	{
		rp._p0 = _res0;
		rp._p1 = _res1;
		rp._p2 = _res2;
		rp._p3 = _res3;
	}

	void operator =(const const_ref_type& rp)
	{
		_res0 = rp._p0;
		_res1 = rp._p1;
		_res2 = rp._p2;
		_res3 = rp._p3;
	}

	T0 _res0;
	T1 _res1;
	T2 _res2;
	T3 _res3;
};

template <typename T0, typename T1, typename T2>
struct msg_param<T0, T1, T2, void>: public msg_param_base
{
	typedef boost::function<void (T0, T1, T2)> overflow_notify;
	typedef ref_ex<T0, T1, T2> ref_type;
	typedef const_ref_ex<T0, T1, T2> const_ref_type;

	msg_param()
	{

	}

	msg_param(const T0& p0, const T1& p1, const T2& p2)
		:_res0(p0), _res1(p1), _res2(p2)
	{

	}

	msg_param(const const_ref_type& rp)
		:_res0(rp._p0), _res1(rp._p1), _res2(rp._p2)
	{

	}

	static void proxy_notify(const overflow_notify& nfy, const msg_param& p)
	{
		nfy(p._res0, p._res1, p._res2);
	}

	void get_param(ref_type& rp) const
	{
		rp._p0 = _res0;
		rp._p1 = _res1;
		rp._p2 = _res2;
	}

	void operator =(const const_ref_type& rp)
	{
		_res0 = rp._p0;
		_res1 = rp._p1;
		_res2 = rp._p2;
	}

	T0 _res0;
	T1 _res1;
	T2 _res2;
};

template <typename T0, typename T1>
struct msg_param<T0, T1, void, void>: public msg_param_base
{
	typedef boost::function<void (T0, T1)> overflow_notify;
	typedef ref_ex<T0, T1> ref_type;
	typedef const_ref_ex<T0, T1> const_ref_type;

	msg_param()
	{

	}

	msg_param(const T0& p0, const T1& p1)
		:_res0(p0), _res1(p1)
	{

	}

	msg_param(const const_ref_type& rp)
		:_res0(rp._p0), _res1(rp._p1)
	{

	}

	static void proxy_notify(const overflow_notify& nfy, const msg_param& p)
	{
		nfy(p._res0, p._res1);
	}

	void get_param(ref_type& rp) const
	{
		rp._p0 = _res0;
		rp._p1 = _res1;
	}

	void operator =(const const_ref_type& rp)
	{
		_res0 = rp._p0;
		_res1 = rp._p1;
	}

	T0 _res0;
	T1 _res1;
};

template <typename T0>
struct msg_param<T0, void, void, void>: public msg_param_base
{
	typedef boost::function<void (T0)> overflow_notify;
	typedef ref_ex<T0> ref_type;
	typedef const_ref_ex<T0> const_ref_type;

	msg_param()
	{

	}

	msg_param(const T0& p0)
		:_res0(p0)
	{

	}

	msg_param(const const_ref_type& rp)
		:_res0(rp._p0)
	{

	}

	static void proxy_notify(const overflow_notify& nfy, const msg_param& p)
	{
		nfy(p._res0);
	}

	void get_param(ref_type& rp) const
	{
		rp._p0 = _res0;
	}

	void operator =(const const_ref_type& rp)
	{
		_res0 = rp._p0;
	}

	T0 _res0;
};

class param_list_base
{
	friend boost_actor;
private:
	param_list_base(const param_list_base&);
	param_list_base& operator =(const param_list_base&);
protected:
	param_list_base();
	virtual ~param_list_base();
	bool closed();
	void begin(long long actorID);
	virtual size_t get_size() = 0;
	virtual void close() = 0;
	virtual void clear() = 0;
protected:
	boost::shared_ptr<bool> _pIsClosed;
	bool _waiting;
	bool _timeout;
	bool _hasTm;
	DEBUG_OPERATION(long long _actorID);
};

template <typename T>
class param_list: public param_list_base
{
	friend boost_actor;
	typedef typename T::ref_type ref_type;
	typedef typename T::const_ref_type const_ref_type;
public:
	virtual ~param_list() {}
protected:
	param_list(): _dstRefPt(NULL) {}
	virtual size_t push_back(const T& p) = 0;
	size_t push_back(const const_ref_type& p)
	{
		return push_back(T(p));
	}

	virtual T* front() = 0;
	virtual size_t pop_front() = 0;

	void get_param(const T& p)
	{
		assert(!(*_pIsClosed) && _dstRefPt);
		p.get_param(*_dstRefPt);
		DEBUG_OPERATION(_dstRefPt = NULL);
	}

	void get_param(const const_ref_type& p)
	{
		assert(!(*_pIsClosed) && _dstRefPt);
		*_dstRefPt = p;
		DEBUG_OPERATION(_dstRefPt = NULL);
	}

	void set_ref(ref_type& p)
	{
		_dstRefPt = &p;
	}
protected:
	ref_type* _dstRefPt;
};

template <typename T /*msg_param*/>
class param_list_no_limit: public param_list<T>
{
	friend boost_actor;
public:
	size_t get_size()
	{
		return _params.size();
	}

	void clear()
	{
		_params.clear();
	}
protected:
	void close()
	{
		_params.clear();
		if (_pIsClosed)
		{
			assert(!(*_pIsClosed));
			(*_pIsClosed) = true;
			_waiting = false;
			_pIsClosed.reset();
		}
	}

	size_t push_back(const T& p)
	{
		assert(!(*_pIsClosed));
		_params.push_back(p);
		return _params.size();
	}

	T* front()
	{
		assert(!(*_pIsClosed));
		if (!_params.empty())
		{
			return &_params.front();
		}
		return NULL;
	}

	size_t pop_front()
	{
		assert(!(*_pIsClosed));
		assert(!_params.empty());
		_params.pop_front();
		return _params.size();
	}
private:
	list<T> _params;
};

template <typename T /*msg_param*/>
class param_list_limit: public param_list<T>
{
	friend boost_actor;
public:
	size_t get_size()
	{
		return _params.size();
	}

	void clear()
	{
		_params.clear();
	}
protected:
	typedef typename T::overflow_notify overflow_notify;

	param_list_limit(size_t maxBuff, const overflow_notify& ofh)
		:_params(maxBuff)
	{
		_ofh = ofh;
	}

	void close()
	{
		_params.clear();
		if (_pIsClosed)
		{
			assert(!(*_pIsClosed));
			(*_pIsClosed) = true;
			_waiting = false;
			_pIsClosed.reset();
			_ofh.clear();
		}
	}

	size_t push_back(const T& p)
	{
		assert(!(*_pIsClosed));
		if (_params.size() < _params.capacity())
		{
			_params.push_back(p);
			return _params.size();
		}
		if (!_ofh)
		{
			_params.pop_front();
			_params.push_back(p);
			return _params.size();
		} 
		T::proxy_notify(_ofh, p);
		return 0;
	}

	T* front()
	{
		assert(!(*_pIsClosed));
		if (!_params.empty())
		{
			return &_params.front();
		}
		return NULL;
	}

	size_t pop_front()
	{
		assert(!(*_pIsClosed));
		assert(!_params.empty());
		_params.pop_front();
		return _params.size();
	}
private:
	overflow_notify _ofh;
	boost::circular_buffer<T> _params;
};

class null_param_list: public param_list_base
{
	friend boost_actor;
public:
	size_t get_size();

	void clear();
protected:
	null_param_list();

	void close();

	size_t count;
};

template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class actor_msg_handle: public param_list_no_limit<msg_param<T0, T1, T2, T3> >
{
	friend boost_actor;
public:
	typedef boost::shared_ptr<actor_msg_handle<T0, T1, T2, T3> > ptr;

	static boost::shared_ptr<actor_msg_handle<T0, T1, T2, T3> > make_ptr()
	{
		return boost::shared_ptr<actor_msg_handle<T0, T1, T2, T3> >(new actor_msg_handle<T0, T1, T2, T3>);
	}
};

template <typename T0, typename T1, typename T2>
class actor_msg_handle<T0, T1, T2, void>: public param_list_no_limit<msg_param<T0, T1, T2> >
{
	friend boost_actor;
public:
	typedef boost::shared_ptr<actor_msg_handle<T0, T1, T2> > ptr;

	static boost::shared_ptr<actor_msg_handle<T0, T1, T2> > make_ptr()
	{
		return boost::shared_ptr<actor_msg_handle<T0, T1, T2> >(new actor_msg_handle<T0, T1, T2>);
	}
};

template <typename T0, typename T1>
class actor_msg_handle<T0, T1, void, void>: public param_list_no_limit<msg_param<T0, T1> >
{
	friend boost_actor;
public:
	typedef boost::shared_ptr<actor_msg_handle<T0, T1> > ptr;

	static boost::shared_ptr<actor_msg_handle<T0, T1> > make_ptr()
	{
		return boost::shared_ptr<actor_msg_handle<T0, T1> >(new actor_msg_handle<T0, T1>);
	}
};

template <typename T0>
class actor_msg_handle<T0, void, void, void>: public param_list_no_limit<msg_param<T0> >
{
	friend boost_actor;
public:
	typedef boost::shared_ptr<actor_msg_handle<T0> > ptr;

	static boost::shared_ptr<actor_msg_handle<T0> > make_ptr()
	{
		return boost::shared_ptr<actor_msg_handle<T0> >(new actor_msg_handle<T0>);
	}
};

template <>
class actor_msg_handle<void, void, void, void>: public null_param_list
{
	friend boost_actor;
public:
	typedef boost::shared_ptr<actor_msg_handle<> > ptr;

	static boost::shared_ptr<actor_msg_handle<> > make_ptr()
	{
		return boost::shared_ptr<actor_msg_handle<> >(new actor_msg_handle<>);
	}
};
//////////////////////////////////////////////////////////////////////////
template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class actor_msg_limit_handle: public param_list_limit<msg_param<T0, T1, T2, T3> >
{
	friend boost_actor;
public:
	typedef typename msg_param<T0, T1, T2, T3>::overflow_notify overflow_notify;
	typedef boost::shared_ptr<actor_msg_limit_handle<T0, T1, T2, T3> > ptr;

	/*!
	@param maxBuff 最大缓存个数
	@param ofh 溢出触发函数
	*/
	actor_msg_limit_handle(int maxBuff,  const overflow_notify& ofh)
		:param_list_limit<msg_param<T0, T1, T2, T3> >(maxBuff, ofh)
	{

	}

	static boost::shared_ptr<actor_msg_limit_handle<T0, T1, T2, T3> > make_ptr(int maxBuff,  const overflow_notify& ofh)
	{
		return boost::shared_ptr<actor_msg_limit_handle<T0, T1, T2, T3> >(new actor_msg_limit_handle<T0, T1, T2, T3>(maxBuff, ofh));
	}
};

template <typename T0, typename T1, typename T2>
class actor_msg_limit_handle<T0, T1, T2, void>: public param_list_limit<msg_param<T0, T1, T2> >
{
	friend boost_actor;
public:
	typedef typename msg_param<T0, T1, T2>::overflow_notify overflow_notify;
	typedef boost::shared_ptr<actor_msg_limit_handle<T0, T1, T2> > ptr;

	actor_msg_limit_handle(int maxBuff,  const overflow_notify& ofh)
		:param_list_limit<msg_param<T0, T1, T2> >(maxBuff, ofh)
	{

	}

	static boost::shared_ptr<actor_msg_limit_handle<T0, T1, T2> > make_ptr(int maxBuff,  const overflow_notify& ofh)
	{
		return boost::shared_ptr<actor_msg_limit_handle<T0, T1, T2> >(new actor_msg_limit_handle<T0, T1, T2>(maxBuff, ofh));
	}
};

template <typename T0, typename T1>
class actor_msg_limit_handle<T0, T1, void, void>: public param_list_limit<msg_param<T0, T1> >
{
	friend boost_actor;
public:
	typedef typename msg_param<T0, T1>::overflow_notify overflow_notify;
	typedef boost::shared_ptr<actor_msg_limit_handle<T0, T1> > ptr;

	actor_msg_limit_handle(int maxBuff,  const overflow_notify& ofh)
		:param_list_limit<msg_param<T0, T1> >(maxBuff, ofh)
	{

	}

	static boost::shared_ptr<actor_msg_limit_handle<T0, T1> > make_ptr(int maxBuff,  const overflow_notify& ofh)
	{
		return boost::shared_ptr<actor_msg_limit_handle<T0, T1> >(new actor_msg_limit_handle<T0, T1>(maxBuff, ofh));
	}
};

template <typename T0>
class actor_msg_limit_handle<T0, void, void, void>: public param_list_limit<msg_param<T0> >
{
	friend boost_actor;
public:
	typedef typename msg_param<T0>::overflow_notify overflow_notify;
	typedef boost::shared_ptr<actor_msg_limit_handle<T0> > ptr;

	actor_msg_limit_handle(int maxBuff,  const overflow_notify& ofh)
		:param_list_limit<msg_param<T0> >(maxBuff, ofh)
	{

	}

	static boost::shared_ptr<actor_msg_limit_handle<T0> > make_ptr(int maxBuff,  const overflow_notify& ofh)
	{
		return boost::shared_ptr<actor_msg_limit_handle<T0> >(new actor_msg_limit_handle<T0>(maxBuff, ofh));
	}
};

class async_trig_base
{
	friend boost_actor;
public:
	async_trig_base();
	virtual ~async_trig_base();
private:
	void begin(long long actorID);
	void close();
	virtual void get_param(void* pref) = 0;
	virtual void set_ref(void* cref) = 0;
	virtual void set_temp(void* cref) = 0;
	async_trig_base(const async_trig_base&);
	async_trig_base& operator=(const async_trig_base&);
protected:
	boost::shared_ptr<bool> _pIsClosed;
	bool _notify;
	bool _waiting;
	bool _timeout;
	bool _hasTm;
	void* _dstRefPt;
	DEBUG_OPERATION(long long _actorID);
};

template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class async_trig_handle: public async_trig_base
{
	friend boost_actor;
	typedef ref_ex<T0, T1, T2, T3> ref_type;
	typedef const_ref_ex<T0, T1, T2, T3> const_ref_type;
public:
	typedef boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> > ptr;

	static boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> >(new async_trig_handle<T0, T1, T2, T3>);
	}

	void get_param(void* pref)
	{
		_temp.get_param(*(ref_type*)pref);
	}

	void set_ref(void* cref)
	{
		assert(_dstRefPt);
		*(ref_type*)_dstRefPt = *(const_ref_type*)cref;
	}

	void set_temp(void* cref)
	{
		assert(!_waiting);
		_temp = *(const_ref_type*)cref;
	}
private:
	msg_param<T0, T1, T2, T3> _temp;
};

template <typename T0, typename T1, typename T2>
class async_trig_handle<T0, T1, T2, void>: public async_trig_base
{
	friend boost_actor;
	typedef ref_ex<T0, T1, T2> ref_type;
	typedef const_ref_ex<T0, T1, T2> const_ref_type;
public:
	typedef boost::shared_ptr<async_trig_handle<T0, T1, T2> > ptr;

	static boost::shared_ptr<async_trig_handle<T0, T1, T2> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<T0, T1, T2> >(new async_trig_handle<T0, T1, T2>);
	}

	void get_param(void* pref)
	{
		_temp.get_param(*(ref_type*)pref);
	}

	void set_ref(void* cref)
	{
		assert(_dstRefPt);
		*(ref_type*)_dstRefPt = *(const_ref_type*)cref;
	}

	void set_temp(void* cref)
	{
		assert(!_waiting);
		_temp = *(const_ref_type*)cref;
	}
private:
	msg_param<T0, T1, T2> _temp;
};

template <typename T0, typename T1>
class async_trig_handle<T0, T1, void, void>: public async_trig_base
{
	friend boost_actor;
	typedef ref_ex<T0, T1> ref_type;
	typedef const_ref_ex<T0, T1> const_ref_type;
public:
	typedef boost::shared_ptr<async_trig_handle<T0, T1> > ptr;

	static boost::shared_ptr<async_trig_handle<T0, T1> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<T0, T1> >(new async_trig_handle<T0, T1>);
	}

	void get_param(void* pref)
	{
		_temp.get_param(*(ref_type*)pref);
	}

	void set_ref(void* cref)
	{
		assert(_dstRefPt);
		*(ref_type*)_dstRefPt = *(const_ref_type*)cref;
	}

	void set_temp(void* cref)
	{
		assert(!_waiting);
		_temp = *(const_ref_type*)cref;
	}
private:
	msg_param<T0, T1> _temp;
};

template <typename T0>
class async_trig_handle<T0, void, void, void>: public async_trig_base
{
	friend boost_actor;
	typedef ref_ex<T0> ref_type;
	typedef const_ref_ex<T0> const_ref_type;
public:
	typedef boost::shared_ptr<async_trig_handle<T0> > ptr;

	static boost::shared_ptr<async_trig_handle<T0> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<T0> >(new async_trig_handle<T0>);
	}

	void get_param(void* pref)
	{
		_temp.get_param(*(ref_type*)pref);
	}

	void set_ref(void* cref)
	{
		assert(_dstRefPt);
		*(ref_type*)_dstRefPt = *(const_ref_type*)cref;
	}

	void set_temp(void* cref)
	{
		assert(!_waiting);
		_temp = *(const_ref_type*)cref;
	}
private:
	msg_param<T0> _temp;
};

template <>
class async_trig_handle<void, void, void, void>: public async_trig_base
{
	friend boost_actor;
public:
	typedef boost::shared_ptr<async_trig_handle<> > ptr;

	static boost::shared_ptr<async_trig_handle<> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<> >(new async_trig_handle<>);
	}

	void get_param(void* pref)
	{
		assert(!pref);
	}

	void set_ref(void* cref)
	{
		assert(!cref);
	}

	void set_temp(void* cref)
	{
		assert(!_waiting);
		assert(!cref);
	}
};
//////////////////////////////////////////////////////////////////////////
/*!
@brief 子Actor句柄，不可拷贝
*/
class child_actor_handle 
{
public:
	typedef boost::shared_ptr<child_actor_handle> ptr;
private:
	friend boost_actor;
	/*!
	@brief 子Actor句柄参数，child_actor_handle内使用
	*/
	struct child_actor_param
	{
#ifdef _DEBUG
		child_actor_param();

		child_actor_param(child_actor_param& s);

		~child_actor_param();

		child_actor_param& operator =(child_actor_param& s);

		bool _isCopy;
#endif
		actor_handle _actor;///<本Actor
		list<actor_handle>::iterator _actorIt;///<保存在父Actor集合中的节点
	};
private:
	child_actor_handle(child_actor_handle&);

	child_actor_handle& operator =(child_actor_handle&);
public:
	child_actor_handle();

	child_actor_handle(child_actor_param& s);

	~child_actor_handle();

	child_actor_handle& operator =(child_actor_param& s);

	actor_handle get_actor();

	actor_handle peel();

	static ptr make_ptr();
private:
	void* operator new(size_t s);
public:
	void operator delete(void* p);
private:
	DEBUG_OPERATION(list<boost::function<void ()> >::iterator _qh);
	bool _norQuit;///<是否正常退出
	bool _quited;///<检测是否已经关闭
	child_actor_param _param;
};

class boost_actor
{
	struct suspend_resume_option 
	{
		bool _isSuspend;
		boost::function<void ()> _h;
	};

	struct timer_pck;
	class boost_actor_run;
	friend boost_actor_run;
	friend child_actor_handle;
public:
	/*!
	@brief Actor被强制退出的异常类型
	*/
	struct actor_force_quit { };

	/*!
	@brief Actor入口函数体
	*/
	typedef boost::function<void (boost_actor*)> main_func;
private:
	boost_actor();
	boost_actor(const boost_actor&);
	boost_actor& operator =(const boost_actor&);
public:
	~boost_actor();
public:
	/*!
	@brief 创建一个Actor
	@param actorStrand Actor所依赖的strand
	@param mainFunc Actor执行入口
	@param stackSize Actor栈大小，默认64k字节，必须是4k的整数倍，最小4k，最大1M
	*/
	static actor_handle create(shared_strand actorStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 同上，带完成Actor后的回调通知
	@param cb Actor完成后的触发函数，false强制结束的，true正常结束
	*/
	static actor_handle create(shared_strand actorStrand, const main_func& mainFunc, 
		const boost::function<void (bool)>& cb, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 异步创建一个Actor，创建成功后通过回调函数通知
	*/
	static void async_create(shared_strand actorStrand, const main_func& mainFunc, 
		const boost::function<void (actor_handle)>& ch, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 同上，带完成Actor后的回调通知
	*/
	static void async_create(shared_strand actorStrand, const main_func& mainFunc, 
		const boost::function<void (actor_handle)>& ch, const boost::function<void (bool)>& cb, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 启用堆栈内存池
	*/
	static void enable_stack_pool();

	/*!
	@brief 禁用创建Actor时自动构造定时器
	*/
	static void disable_auto_make_timer();
public:
	/*!
	@brief 创建一个子Actor，父Actor终止时，子Actor也终止（在子Actor都完全退出后，父Actor才结束）
	@param actorStrand 子Actor依赖的strand
	@param mainFunc 子Actor入口函数
	@param stackSize Actor栈大小，4k的整数倍（最大1MB）
	@return 子Actor句柄，使用 child_actor_handle 接收返回值
	*/
	child_actor_handle::child_actor_param create_child_actor(shared_strand actorStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);
	child_actor_handle::child_actor_param create_child_actor(const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 开始运行子Actor，只能调用一次
	*/
	void child_actor_run(child_actor_handle& actorHandle);
	void child_actor_run(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 强制终止一个子Actor
	@return 已经被正常退出返回true，被强制退出返回false
	*/
	__yield_interrupt bool child_actor_force_quit(child_actor_handle& actorHandle);
	__yield_interrupt bool child_actors_force_quit(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 等待一个子Actor完成后返回
	@return 正常退出的返回true，否则false
	*/
	__yield_interrupt bool child_actor_wait_quit(child_actor_handle& actorHandle);

	/*!
	@brief 等待一组子Actor完成后返回
	@return 都正常退出的返回true，否则false
	*/
	__yield_interrupt bool child_actors_wait_quit(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 挂起子Actor
	*/
	__yield_interrupt void child_actor_suspend(child_actor_handle& actorHandle);
	
	/*!
	@brief 挂起一组子Actor
	*/
	__yield_interrupt void child_actors_suspend(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 恢复子Actor
	*/
	__yield_interrupt void child_actor_resume(child_actor_handle& actorHandle);
	
	/*!
	@brief 恢复一组子Actor
	*/
	__yield_interrupt void child_actors_resume(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 分离出子Actor
	*/
	actor_handle child_actor_peel(child_actor_handle& actorHandle);

	/*!
	@brief 创建另一个Actor，Actor执行完成后返回
	*/
	__yield_interrupt bool run_child_actor_complete(shared_strand actorStrand, const main_func& h, size_t stackSize = DEFAULT_STACKSIZE);
	__yield_interrupt bool run_child_actor_complete(const main_func& h, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 延时等待，Actor内部禁止使用操作系统API Sleep()
	@param ms 等待毫秒数，等于0时暂时放弃Actor执行，直到下次被调度器触发
	*/
	__yield_interrupt void sleep(int ms);

	/*!
	@brief 调用disable_auto_make_timer后，使用这个打开当前Actor定时器
	*/
	void open_timer();

	/*!
	@brief 获取父Actor
	*/
	actor_handle parent_actor();

	/*!
	@brief 获取子Actor
	*/
	const list<actor_handle>& child_actors();
public:
	typedef list<boost::function<void ()> >::iterator quit_iterator;

	/*!
	@brief 注册一个资源释放函数，在强制退出Actor时调用
	*/
	quit_iterator regist_quit_handler(const boost::function<void ()>& quitHandler);

	/*!
	@brief 注销资源释放函数
	*/
	void cancel_quit_handler(quit_iterator qh);
public:
	/*!
	@brief 创建一个异步触发函数，使用timed_wait_trig()等待
	@param th 触发句柄
	@return 触发函数对象，可以多次调用(线程安全)，但只能timed_wait_trig()到第一次调用的值
	*/
	boost::function<void ()> begin_trig(async_trig_handle<>& th);
	boost::function<void ()> begin_trig(boost::shared_ptr<async_trig_handle<> > th);

	template <typename T0>
	boost::function<void (T0)> begin_trig(async_trig_handle<T0>& th)
	{
		th.begin(_actorID);
		return boost::bind(&boost_actor::async_trig_handler<T0>, shared_from_this(), th._pIsClosed, boost::ref(th), _1);
	}

	template <typename T0>
	boost::function<void (T0)> begin_trig(boost::shared_ptr<async_trig_handle<T0> > th)
	{
		th->begin(_actorID);
		return boost::bind(&boost_actor::async_trig_handler_ptr<T0>, shared_from_this(), th->_pIsClosed, th, _1);
	}

	template <typename T0, typename T1>
	boost::function<void (T0, T1)> begin_trig(async_trig_handle<T0, T1>& th)
	{
		th.begin(_actorID);
		return boost::bind(&boost_actor::async_trig_handler<T0, T1>, shared_from_this(), th._pIsClosed, boost::ref(th), _1, _2);
	}

	template <typename T0, typename T1>
	boost::function<void (T0, T1)> begin_trig(boost::shared_ptr<async_trig_handle<T0, T1> > th)
	{
		th->begin(_actorID);
		return boost::bind(&boost_actor::async_trig_handler_ptr<T0, T1>, shared_from_this(), th->_pIsClosed, th, _1, _2);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void (T0, T1, T2)> begin_trig(async_trig_handle<T0, T1, T2>& th)
	{
		th.begin(_actorID);
		return boost::bind(&boost_actor::async_trig_handler<T0, T1, T2>, shared_from_this(), th._pIsClosed, boost::ref(th), _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void (T0, T1, T2)> begin_trig(boost::shared_ptr<async_trig_handle<T0, T1, T2> > th)
	{
		th->begin(_actorID);
		return boost::bind(&boost_actor::async_trig_handler_ptr<T0, T1, T2>, shared_from_this(), th->_pIsClosed, th, _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void (T0, T1, T2, T3)> begin_trig(async_trig_handle<T0, T1, T2, T3>& th)
	{
		th.begin(_actorID);
		return boost::bind(&boost_actor::async_trig_handler<T0, T1, T2, T3>, shared_from_this(), th._pIsClosed, boost::ref(th), _1, _2, _3, _4);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void (T0, T1, T2, T3)> begin_trig(boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> > th)
	{
		th->begin(_actorID);
		return boost::bind(&boost_actor::async_trig_handler_ptr<T0, T1, T2, T3>, shared_from_this(), th->_pIsClosed, th, _1, _2, _3, _4);
	}

	/*!
	@brief 等待begin_trig创建的回调句柄
	@param th 异步句柄
	@param tm 异步等待超时ms，超时后返回false
	@return 超时后返回false
	*/
	__yield_interrupt bool timed_wait_trig(async_trig_handle<>& th, int tm);
	__yield_interrupt void wait_trig(async_trig_handle<>& th);

	template <typename T0>
	__yield_interrupt void wait_trig(async_trig_handle<T0>& th, T0& r0)
	{
		timed_wait_trig(th, r0, -1);
	}

	template <typename T0>
	__yield_interrupt bool timed_wait_trig(async_trig_handle<T0>& th, T0& r0, int tm)
	{
		assert(th._actorID == _actorID);
		assert_enter();
		ref_ex<T0> ref(r0);
		if (async_trig_push(th, tm, &ref))
		{
			close_trig(th);
			return true;
		}
		return false;
	}

	template <typename T0>
	__yield_interrupt T0 wait_trig(async_trig_handle<T0>& th)
	{
		T0 r;
		timed_wait_trig(th, r, -1);
		return r;
	}

	template <typename T0, typename T1>
	__yield_interrupt void wait_trig(async_trig_handle<T0, T1>& th, T0& r0, T1& r1)
	{
		timed_wait_trig(th, r0, r1, -1);
	}

	template <typename T0, typename T1>
	__yield_interrupt bool timed_wait_trig(async_trig_handle<T0, T1>& th, T0& r0, T1& r1, int tm)
	{
		assert(th._actorID == _actorID);
		assert_enter();
		ref_ex<T0, T1> ref(r0, r1);
		if (async_trig_push(th, tm, &ref))
		{
			close_trig(th);
			return true;
		}
		return false;
	}

	template <typename T0, typename T1, typename T2>
	__yield_interrupt void wait_trig(async_trig_handle<T0, T1, T2>& th, T0& r0, T1& r1, T2& r2)
	{
		timed_wait_trig(th, r0, r1, r2, -1);
	}

	template <typename T0, typename T1, typename T2>
	__yield_interrupt bool timed_wait_trig(async_trig_handle<T0, T1, T2>& th, T0& r0, T1& r1, T2& r2, int tm)
	{
		assert(th._actorID == _actorID);
		assert_enter();
		ref_ex<T0, T1, T2> ref(r0, r1, r2);
		if (async_trig_push(th, tm, &ref))
		{
			close_trig(th);
			return true;
		}
		return false;
	}

	template <typename T0, typename T1, typename T2, typename T3>
	__yield_interrupt void wait_trig(async_trig_handle<T0, T1, T2, T3>& th, T0& r0, T1& r1, T2& r2, T3& r3)
	{
		timed_wait_trig(th, r0, r1, r2, r3, -1);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	__yield_interrupt bool timed_wait_trig(async_trig_handle<T0, T1, T2, T3>& th, T0& r0, T1& r1, T2& r2, T3& r3, int tm)
	{
		assert(th._actorID == _actorID);
		assert_enter();
		ref_ex<T0, T1, T2, T3> ref(r0, r1, r2, r3);
		if (async_trig_push(th, tm, &ref))
		{
			close_trig(th);
			return true;
		}
		return false;
	}

	/*!
	@brief 关闭触发句柄
	*/
	void close_trig(async_trig_base& th);

	/*!
	@brief 使用内部定时器延时触发某个函数，在触发完成之前不能多次调用
	@param ms 触发延时(毫秒)
	@param h 触发函数
	*/
	void delay_trig(int ms, const boost::function<void ()>& h);
	
	/*!
	@brief 使用内部定时器延时触发异步句柄，使用之前必须已经调用了begin_trig(async_trig_handle)，在触发完成之前不能多次调用
	@param ms 触发延时(毫秒)
	@param th 异步触发句柄
	*/
	void delay_trig(int ms, async_trig_handle<>& th);
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<> > th);

	template <typename T0>
	void delay_trig(int ms, async_trig_handle<T0>& th, const T0& p0)
	{
		assert_enter();
		assert(th._pIsClosed);
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::_async_trig_handler<T0>, shared_from_this(), 
			th._pIsClosed, boost::ref(th), p0));
	}

	template <typename T0>
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<T0> > th, const T0& p0)
	{
		assert_enter();
		assert(th->_pIsClosed);
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::_async_trig_handler_ptr<T0>, shared_from_this(), 
			th->_pIsClosed, th, p0));
	}

	template <typename T0, typename T1>
	void delay_trig(int ms, async_trig_handle<T0, T1>& th, const T0& p0, const T1& p1)
	{
		assert_enter();
		assert(th._pIsClosed);
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::_async_trig_handler<T0, T1>, shared_from_this(), 
			th._pIsClosed, boost::ref(th), p0, p1));
	}

	template <typename T0, typename T1>
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<T0, T1> > th, const T0& p0, const T1& p1)
	{
		assert_enter();
		assert(th->_pIsClosed);
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::_async_trig_handler_ptr<T0, T1>, shared_from_this(), 
			th->_pIsClosed, th, p0, p1));
	}

	template <typename T0, typename T1, typename T2>
	void delay_trig(int ms, async_trig_handle<T0, T1, T2>& th, const T0& p0, const T1& p1, const T2& p2)
	{
		assert_enter();
		assert(th._pIsClosed);
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::_async_trig_handler<T0, T1, T2>, shared_from_this(), 
			th._pIsClosed, boost::ref(th), p0, p1, p2));
	}

	template <typename T0, typename T1, typename T2>
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<T0, T1, T2> > th, const T0& p0, const T1& p1, const T2& p2)
	{
		assert_enter();
		assert(th->_pIsClosed);
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::_async_trig_handler_ptr<T0, T1, T2>, shared_from_this(), 
			th->_pIsClosed, th, p0, p1, p2));
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void delay_trig(int ms, async_trig_handle<T0, T1, T2, T3>& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		assert_enter();
		assert(th._pIsClosed);
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::_async_trig_handler<T0, T1, T2, T3>, shared_from_this(), 
			th._pIsClosed, boost::ref(th), p0, p1, p2, p3));
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> > th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		assert_enter();
		assert(th->_pIsClosed);
		assert(_timerSleep);
		time_out(ms, boost::bind(&boost_actor::_async_trig_handler_ptr<T0, T1, T2, T3>, shared_from_this(), 
			th->_pIsClosed, th, p0, p1, p2, p3));
	}

	/*!
	@brief 取消内部定时器触发
	*/
	void cancel_delay_trig();
public:
	/*!
	@brief 发送一个异步函数到shared_strand中执行，完成后返回
	*/
	__yield_interrupt void send(shared_strand exeStrand, const boost::function<void ()>& h);

	template <typename T0>
	__yield_interrupt T0 send(shared_strand exeStrand, const boost::function<T0 ()>& h)
	{
		assert_enter();
		if (exeStrand != _strand)
		{
			return trig<T0>(boost::bind(&boost_strand::asyncInvoke<T0>, exeStrand, h, _1));
		} 
		return h();
	}

	/*!
	@brief 调用一个异步函数，异步回调完成后返回
	*/
	__yield_interrupt void trig(const boost::function<void (boost::function<void ()>)>& h);

	template <typename T0>
	__yield_interrupt void trig(const boost::function<void (boost::function<void (T0)>)>& h, __out T0& r0)
	{
		assert_enter();
#ifdef _DEBUG
		h(wrapped_trig_handler<boost::function<void (T0)> >(
			boost::bind(&boost_actor::trig_handler<T0>, shared_from_this(), boost::ref(r0), _1)));
#else
		h(boost::bind(&boost_actor::trig_handler<T0>, shared_from_this(), boost::ref(r0), _1));
#endif
		push_yield();
	}

	template <typename T0>
	__yield_interrupt T0 trig(const boost::function<void (boost::function<void (T0)>)>& h)
	{
		T0 r0;
		trig(h, r0);
		return r0;
	}

	template <typename T0, typename T1>
	__yield_interrupt void trig(const boost::function<void (boost::function<void (T0, T1)>)>& h, __out T0& r0, __out T1& r1)
	{
		assert_enter();
		ref_ex<T0, T1> mulRef(r0, r1);
#ifdef _DEBUG
		h(wrapped_trig_handler<boost::function<void (T0, T1)> >(
			boost::bind(&boost_actor::trig_handler<T0, T1>, shared_from_this(), boost::ref(mulRef), _1, _2)));
#else
		h(boost::bind(&boost_actor::trig_handler<T0, T1>, shared_from_this(), boost::ref(mulRef), _1, _2));
#endif
		push_yield();
	}

	template <typename T0, typename T1, typename T2>
	__yield_interrupt void trig(const boost::function<void (boost::function<void (T0, T1, T2)>)>& h, __out T0& r0, __out T1& r1, __out T2& r2)
	{
		assert_enter();
		ref_ex<T0, T1, T2> mulRef(r0, r1, r2);
#ifdef _DEBUG
		h(wrapped_trig_handler<boost::function<void (T0, T1, T2)> >(
			boost::bind(&boost_actor::trig_handler<T0, T1, T2>, shared_from_this(), 
			boost::ref(mulRef), _1, _2, _3)));
#else
		h(boost::bind(&boost_actor::trig_handler<T0, T1, T2>, shared_from_this(), 
			boost::ref(mulRef), _1, _2, _3));
#endif
		push_yield();
	}

	template <typename T0, typename T1, typename T2, typename T3>
	__yield_interrupt void trig(const boost::function<void (boost::function<void (T0, T1, T2, T3)>)>& h, __out T0& r0, __out T1& r1, __out T2& r2, __out T3& r3)
	{
		assert_enter();
		ref_ex<T0, T1, T2, T3> mulRef(r0, r1, r2, r3);
#ifdef _DEBUG
		h(wrapped_trig_handler<boost::function<void (T0, T1, T2, T3)> >(
			boost::bind(&boost_actor::trig_handler<T0, T1, T2, T3>, shared_from_this(), 
			boost::ref(mulRef), _1, _2, _3, _4)));
#else
		h(boost::bind(&boost_actor::trig_handler<T0, T1, T2, T3>, shared_from_this(), 
			boost::ref(mulRef), _1, _2, _3, _4));
#endif
		push_yield();
	}
	//////////////////////////////////////////////////////////////////////////
	/*!
	@brief 创建一个"生产者"对象，用pump_msg"消费者"取出回调内容，T0-T3是回调参数类型
	@param cmh 异步通知对象
	@return 异步触发函数
	*/
	boost::function<void()> make_msg_notify(actor_msg_handle<>& cmh);
	boost::function<void()> make_msg_notify(boost::shared_ptr<actor_msg_handle<> > cmh);

	template <typename T0>
	boost::function<void(T0)> make_msg_notify(param_list<msg_param<T0> >& cmh)
	{
		cmh.begin(_actorID);
		return boost::bind(&boost_actor::notify_handler<T0>, shared_from_this(), cmh._pIsClosed, boost::ref(cmh), _1);
	}

	template <typename T0>
	boost::function<void(T0)> make_msg_notify(boost::shared_ptr<actor_msg_handle<T0> > cmh)
	{
		cmh->begin(_actorID);
		return boost::bind(&boost_actor::notify_handler_ptr<T0>, shared_from_this(), cmh->_pIsClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0> > >(cmh), _1);
	}

	template <typename T0>
	boost::function<void(T0)> make_msg_notify(boost::shared_ptr<actor_msg_limit_handle<T0> > cmh)
	{
		cmh->begin(_actorID);
		return boost::bind(&boost_actor::notify_handler_ptr<T0>, shared_from_this(), cmh->_pIsClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0> > >(cmh), _1);
	}

	template <typename T0, typename T1>
	boost::function<void(T0, T1)> make_msg_notify(param_list<msg_param<T0, T1> >& cmh)
	{
		cmh.begin(_actorID);
		return boost::bind(&boost_actor::notify_handler<T0, T1>, shared_from_this(), cmh._pIsClosed, boost::ref(cmh), _1, _2);
	}

	template <typename T0, typename T1>
	boost::function<void(T0, T1)> make_msg_notify(boost::shared_ptr<actor_msg_handle<T0, T1> > cmh)
	{
		cmh->begin(_actorID);
		return boost::bind(&boost_actor::notify_handler_ptr<T0, T1>, shared_from_this(), cmh->_pIsClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1> > >(cmh), _1, _2);
	}

	template <typename T0, typename T1>
	boost::function<void(T0, T1)> make_msg_notify(boost::shared_ptr<actor_msg_limit_handle<T0, T1> > cmh)
	{
		cmh->begin(_actorID);
		return boost::bind(&boost_actor::notify_handler_ptr<T0, T1>, shared_from_this(), cmh->_pIsClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1> > >(cmh), _1, _2);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void(T0, T1, T2)> make_msg_notify(param_list<msg_param<T0, T1, T2> >& cmh)
	{
		cmh.begin(_actorID);
		return boost::bind(&boost_actor::notify_handler<T0, T1, T2>, shared_from_this(), cmh._pIsClosed, boost::ref(cmh), _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void(T0, T1, T2)> make_msg_notify(boost::shared_ptr<actor_msg_handle<T0, T1, T2> > cmh)
	{
		cmh->begin(_actorID);
		return boost::bind(&boost_actor::notify_handler_ptr<T0, T1, T2>, shared_from_this(), cmh->_pIsClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1, T2> > >(cmh), _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void(T0, T1, T2)> make_msg_notify(boost::shared_ptr<actor_msg_limit_handle<T0, T1, T2> > cmh)
	{
		cmh->begin(_actorID);
		return boost::bind(&boost_actor::notify_handler_ptr<T0, T1, T2>, shared_from_this(), cmh->_pIsClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1, T2> > >(cmh), _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void(T0, T1, T2, T3)> make_msg_notify(param_list<msg_param<T0, T1, T2, T3> >& cmh)
	{
		cmh.begin(_actorID);
		return boost::bind(&boost_actor::notify_handler<T0, T1, T2, T3>, shared_from_this(), cmh._pIsClosed, boost::ref(cmh), _1, _2, _3, _4);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void(T0, T1, T2, T3)> make_msg_notify(boost::shared_ptr<actor_msg_handle<T0, T1, T2, T3> > cmh)
	{
		cmh->begin(_actorID);
		return boost::bind(&boost_actor::notify_handler_ptr<T0, T1, T2, T3>, shared_from_this(), cmh->_pIsClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1, T2, T3> > >(cmh), _1, _2, _3, _4);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void(T0, T1, T2, T3)> make_msg_notify(boost::shared_ptr<actor_msg_limit_handle<T0, T1, T2, T3> > cmh)
	{
		cmh->begin(_actorID);
		return boost::bind(&boost_actor::notify_handler_ptr<T0, T1, T2, T3>, shared_from_this(), cmh->_pIsClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1, T2, T3> > >(cmh), _1, _2, _3, _4);
	}

	/*!
	@brief 关闭make_msg_notify创建的句柄，之后将不再接收任何消息
	*/
	void close_msg_notify(param_list_base& cmh);
	//////////////////////////////////////////////////////////////////////////
	/*!
	@brief 取出make_msg_notify创建后的回调内容
	@param cmh 消息句柄
	@param tm 消息等待超时ms，超时后返回false
	@return 超时后返回false
	*/
	__yield_interrupt bool timed_pump_msg(actor_msg_handle<>& cmh, int tm);
	__yield_interrupt void pump_msg(actor_msg_handle<>& cmh);

	template <typename T0>
	__yield_interrupt void pump_msg(param_list<msg_param<T0> >& cmh, __out T0& r0)
	{
		timed_pump_msg(cmh, r0, -1);
	}

	template <typename T0>
	__yield_interrupt bool timed_pump_msg(param_list<msg_param<T0> >& cmh, __out T0& r0, int tm)
	{
		assert(cmh._actorID == _actorID);
		assert_enter();
		assert(cmh._pIsClosed);
		ref_ex<T0> ref(r0);
		msg_param<T0>* param = cmh.front();
		if (param)
		{
			param->get_param(ref);
			cmh.pop_front();
			return true;
		}
		cmh._waiting = true;
		cmh.set_ref(ref);
		return pump_msg_push(cmh, tm);
	}

	template <typename T0>
	__yield_interrupt T0 pump_msg(param_list<msg_param<T0> >& cmh)
	{
		T0 r0;
		timed_pump_msg(cmh, r0, -1);
		return r0;
	}

	template <typename T0, typename T1>
	__yield_interrupt void pump_msg(param_list<msg_param<T0, T1> >& cmh, __out T0& r0, __out T1& r1)
	{
		timed_pump_msg(cmh, r0, r1, -1);
	}

	template <typename T0, typename T1>
	__yield_interrupt bool timed_pump_msg(param_list<msg_param<T0, T1> >& cmh, __out T0& r0, __out T1& r1, int tm)
	{
		assert(cmh._actorID == _actorID);
		assert_enter();
		assert(cmh._pIsClosed);
		ref_ex<T0, T1> ref(r0, r1);
		msg_param<T0, T1>* param = cmh.front();
		if (param)
		{
			param->get_param(ref);
			cmh.pop_front();
			return true;
		}
		cmh._waiting = true;
		cmh.set_ref(ref);
		return pump_msg_push(cmh, tm);
	}

	template <typename T0, typename T1, typename T2>
	__yield_interrupt void pump_msg(param_list<msg_param<T0, T1, T2> >& cmh, __out T0& r0, __out T1& r1, __out T2& r2)
	{
		timed_pump_msg(cmh, r0, r1, r2, -1);
	}

	template <typename T0, typename T1, typename T2>
	__yield_interrupt bool timed_pump_msg(param_list<msg_param<T0, T1, T2> >& cmh, __out T0& r0, __out T1& r1, __out T2& r2, int tm)
	{
		assert(cmh._actorID == _actorID);
		assert_enter();
		assert(cmh._pIsClosed);
		ref_ex<T0, T1, T2> ref(r0, r1, r2);
		msg_param<T0, T1, T2>* param = cmh.front();
		if (param)
		{
			param->get_param(ref);
			cmh.pop_front();
			return true;
		}
		cmh._waiting = true;
		cmh.set_ref(ref);
		return pump_msg_push(cmh, tm);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	__yield_interrupt void pump_msg(param_list<msg_param<T0, T1, T2, T3> >& cmh, __out T0& r0, __out T1& r1, __out T2& r2, __out T3& r3)
	{
		timed_pump_msg(cmh, r0, r1, r2, r3, -1);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	__yield_interrupt bool timed_pump_msg(param_list<msg_param<T0, T1, T2, T3> >& cmh, __out T0& r0, __out T1& r1, __out T2& r2, __out T3& r3, int tm)
	{
		assert(cmh._actorID == _actorID);
		assert_enter();
		assert(cmh._pIsClosed);
		ref_ex<T0, T1, T2, T3> ref(r0, r1, r2, r3);
		msg_param<T0, T1, T2, T3>* param = cmh.front();
		if (param)
		{
			param->get_param(ref);
			cmh.pop_front();
			return true;
		}
		cmh._waiting = true;
		cmh.set_ref(ref);
		return pump_msg_push(cmh, tm);
	}
private:
	bool pump_msg_push(param_list_base& pm, int tm);

	void trig_handler();

	template <typename T0>
	void trig_handler(T0& r, const T0& p0)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				r = p0;//必须在此(_strand内部)处理参数，如果Actor已退出(_quited == true)，将导致r失效
				_strand->post(boost::bind(&boost_actor::run_one, shared_from_this()));
			}
		} 
		else
		{//此时不能处理参数，因为 r 可能已失效
			_strand->post(boost::bind(&boost_actor::_trig_handler<T0>, shared_from_this(), boost::ref(r), p0));
		}
	}

	template <typename T0>
	void _trig_handler(T0& r, const T0& p0)
	{
		if (!_quited)
		{
			r = p0;//必须在此(_strand内部)处理参数，如果Actor已退出(_quited == true)，将导致r失效
			pull_yield();
		}
	}

	template <typename T0, typename T1>
	void trig_handler(ref_ex<T0, T1>& r, const T0& p0, const T1& p1)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				r = const_ref_ex<T0, T1>(p0, p1);
				_strand->post(boost::bind(&boost_actor::run_one, shared_from_this()));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::_trig_handler<T0, T1>, shared_from_this(), boost::ref(r), p0, p1));
		}
	}

	template <typename T0, typename T1>
	void _trig_handler(ref_ex<T0, T1>& r, const T0& p0, const T1& p1)
	{
		if (!_quited)
		{
			r = const_ref_ex<T0, T1>(p0, p1);
			pull_yield();
		}
	}

	template <typename T0, typename T1, typename T2>
	void trig_handler(ref_ex<T0, T1, T2>& r, const T0& p0, const T1& p1, const T2& p2)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				r = const_ref_ex<T0, T1, T2>(p0, p1, p2);
				_strand->post(boost::bind(&boost_actor::run_one, shared_from_this()));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::_trig_handler<T0, T1, T2>, shared_from_this(), boost::ref(r), p0, p1, p2));
		}
	}

	template <typename T0, typename T1, typename T2>
	void _trig_handler(ref_ex<T0, T1, T2>& r, const T0& p0, const T1& p1, const T2& p2)
	{
		if (!_quited)
		{
			r = const_ref_ex<T0, T1, T2>(p0, p1, p2);
			pull_yield();
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void trig_handler(ref_ex<T0, T1, T2, T3>& r, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				r = const_ref_ex<T0, T1, T2, T3>(p0, p1, p2, p3);
				_strand->post(boost::bind(&boost_actor::run_one, shared_from_this()));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::_trig_handler<T0, T1, T2, T3>, shared_from_this(), boost::ref(r), p0, p1, p2, p3));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void _trig_handler(ref_ex<T0, T1, T2, T3>& r, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		if (!_quited)
		{
			r = const_ref_ex<T0, T1, T2, T3>(p0, p1, p2, p3);
			pull_yield();
		}
	}

	bool async_trig_push(async_trig_base& th, int tm, void* pref);

	void async_trig_post_yield(async_trig_base& th, void* cref);

	void async_trig_pull_yield(async_trig_base& th, void* cref);

	void _async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<>& th);
	void _async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<> >& th);

	template <typename T0>
	void async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<T0>& th, const T0& p0)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th._notify)
			{
				const_ref_ex<T0> cref(p0);
				async_trig_post_yield(th, &cref);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler<T0>, shared_from_this(), pIsClosed, boost::ref(th), p0));
		}
	}

	template <typename T0>
	void async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<T0> >& th, const T0& p0)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th->_notify)
			{
				const_ref_ex<T0> cref(p0);
				async_trig_post_yield(*th, &cref);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler_ptr<T0>, shared_from_this(), pIsClosed, th, p0));
		}
	}

	template <typename T0>
	void _async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<T0>& th, const T0& p0)
	{
		assert(_strand->running_in_this_thread());
		if (!_quited && !(*pIsClosed) && !th._notify)
		{
			const_ref_ex<T0> cref(p0);
			async_trig_pull_yield(th, &cref);
		}
	}

	template <typename T0>
	void _async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<T0> >& th, const T0& p0)
	{
		return _async_trig_handler(pIsClosed, *th, p0);
	}

	template <typename T0, typename T1>
	void async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<T0, T1>& th, const T0& p0, const T1& p1)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th._notify)
			{
				const_ref_ex<T0, T1> cref(p0, p1);
				async_trig_post_yield(th, &cref);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler<T0, T1>, shared_from_this(), pIsClosed, boost::ref(th), p0, p1));
		}
	}

	template <typename T0, typename T1>
	void async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<T0, T1> >& th, const T0& p0, const T1& p1)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th->_notify)
			{
				const_ref_ex<T0, T1> cref(p0, p1);
				async_trig_post_yield(*th, &cref);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler_ptr<T0, T1>, shared_from_this(), pIsClosed, th, p0, p1));
		}
	}

	template <typename T0, typename T1>
	void _async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<T0, T1>& th, const T0& p0, const T1& p1)
	{
		assert(_strand->running_in_this_thread());
		if (!_quited && !(*pIsClosed) && !th._notify)
		{
			const_ref_ex<T0, T1> cref(p0, p1);
			async_trig_pull_yield(th, &cref);
		}
	}

	template <typename T0, typename T1>
	void _async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<T0, T1> >& th, const T0& p0, const T1& p1)
	{
		return _async_trig_handler(pIsClosed, *th, p0, p1);
	}

	template <typename T0, typename T1, typename T2>
	void async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<T0, T1, T2>& th, const T0& p0, const T1& p1, const T2& p2)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th._notify)
			{
				const_ref_ex<T0, T1, T2> cref(p0, p1, p2);
				async_trig_post_yield(th, &cref);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler<T0, T1, T2>, shared_from_this(), pIsClosed, boost::ref(th), 
				p0, p1, p2));
		}
	}

	template <typename T0, typename T1, typename T2>
	void async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<T0, T1, T2> >& th, const T0& p0, const T1& p1, const T2& p2)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th->_notify)
			{
				const_ref_ex<T0, T1, T2> cref(p0, p1, p2);
				async_trig_post_yield(*th, &cref);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler_ptr<T0, T1, T2>, shared_from_this(), pIsClosed, th, 
				p0, p1, p2));
		}
	}

	template <typename T0, typename T1, typename T2>
	void _async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<T0, T1, T2>& th, const T0& p0, const T1& p1, const T2& p2)
	{
		assert(_strand->running_in_this_thread());
		if (!_quited && !(*pIsClosed) && !th._notify)
		{
			const_ref_ex<T0, T1, T2> cref(p0, p1, p2);
			async_trig_pull_yield(th, &cref);
		}
	}

	template <typename T0, typename T1, typename T2>
	void _async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<T0, T1, T2> >& th, const T0& p0, const T1& p1, const T2& p2)
	{
		return _async_trig_handler(pIsClosed, *th, p0, p1, p2);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<T0, T1, T2, T3>& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th._notify)
			{
				const_ref_ex<T0, T1, T2, T3> cref(p0, p1, p2, p3);
				async_trig_post_yield(th, &cref);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler<T0, T1, T2, T3>, shared_from_this(), pIsClosed, boost::ref(th), 
				p0, p1, p2, p3));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> >& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed) && !th->_notify)
			{
				const_ref_ex<T0, T1, T2, T3> cref(p0, p1, p2, p3);
				async_trig_post_yield(*th, &cref);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_actor::_async_trig_handler_ptr<T0, T1, T2, T3>, shared_from_this(), pIsClosed, th, 
				p0, p1, p2, p3));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void _async_trig_handler(boost::shared_ptr<bool>& pIsClosed, async_trig_handle<T0, T1, T2, T3>& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		assert(_strand->running_in_this_thread());
		if (!_quited && !(*pIsClosed) && !th._notify)
		{
			const_ref_ex<T0, T1, T2, T3> cref(p0, p1, p2, p3);
			async_trig_pull_yield(th, &cref);
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void _async_trig_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> >& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		_async_trig_handler(pIsClosed, *th, p0, p1, p2, p3);
	}
private:
	void check_run1(boost::shared_ptr<bool>& pIsClosed, actor_msg_handle<>& cmh);
	void check_run1_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<actor_msg_handle<> >& cmh);

	template <typename T0>
	void notify_handler(boost::shared_ptr<bool>& pIsClosed, param_list<msg_param<T0> >& cmh, const T0& p0)
	{
		typedef msg_param<T0> msg_type;
		typedef const_ref_ex<T0> const_ref_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed))
			{
				check_run3<msg_type, const_ref_type>(cmh, const_ref_type(p0));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::check_run2<msg_type>, 
				shared_from_this(), pIsClosed, boost::ref(cmh), msg_type(p0)));
		}
	}

	template <typename T0>
	void notify_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<param_list<msg_param<T0> > >& cmh, const T0& p0)
	{
		typedef msg_param<T0> msg_type;
		typedef const_ref_ex<T0> const_ref_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed))
			{
				check_run3<msg_type, const_ref_type>(*cmh, const_ref_type(p0));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::check_run2_ptr<msg_type>, 
				shared_from_this(), pIsClosed, cmh, msg_type(p0)));
		}
	}

	template <typename T0, typename T1>
	void notify_handler(boost::shared_ptr<bool>& pIsClosed, param_list<msg_param<T0, T1> >& cmh, const T0& p0, const T1& p1)
	{
		typedef msg_param<T0, T1> msg_type;
		typedef const_ref_ex<T0, T1> const_ref_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed))
			{
				check_run3<msg_type, const_ref_type>(cmh, const_ref_type(p0, p1));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::check_run2<msg_type>, 
				shared_from_this(), pIsClosed, boost::ref(cmh), msg_type(p0, p1)));
		}
	}

	template <typename T0, typename T1>
	void notify_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<param_list<msg_param<T0, T1> > >& cmh, const T0& p0, const T1& p1)
	{
		typedef msg_param<T0, T1> msg_type;
		typedef const_ref_ex<T0, T1> const_ref_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed))
			{
				check_run3<msg_type, const_ref_type>(*cmh, const_ref_type(p0, p1));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::check_run2_ptr<msg_type>, 
				shared_from_this(), pIsClosed, cmh, msg_type(p0, p1)));
		}
	}

	template <typename T0, typename T1, typename T2>
	void notify_handler(boost::shared_ptr<bool>& pIsClosed, param_list<msg_param<T0, T1, T2> >& cmh, const T0& p0, const T1& p1, const T2& p2)
	{
		typedef msg_param<T0, T1, T2> msg_type;
		typedef const_ref_ex<T0, T1, T2> const_ref_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed))
			{
				check_run3<msg_type, const_ref_type>(cmh, const_ref_type(p0, p1, p2));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::check_run2<msg_type>, 
				shared_from_this(), pIsClosed, boost::ref(cmh), msg_type(p0, p1, p2)));
		}
	}

	template <typename T0, typename T1, typename T2>
	void notify_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<param_list<msg_param<T0, T1, T2> > >& cmh, const T0& p0, const T1& p1, const T2& p2)
	{
		typedef msg_param<T0, T1, T2> msg_type;
		typedef const_ref_ex<T0, T1, T2> const_ref_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed))
			{
				check_run3<msg_type, const_ref_type>(*cmh, const_ref_type(p0, p1, p2));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::check_run2_ptr<msg_type>, 
				shared_from_this(), pIsClosed, cmh, msg_type(p0, p1, p2)));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void notify_handler(boost::shared_ptr<bool>& pIsClosed, param_list<msg_param<T0, T1, T2, T3> >& cmh, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		typedef msg_param<T0, T1, T2, T3> msg_type;
		typedef const_ref_ex<T0, T1, T2, T3> const_ref_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed))
			{
				check_run3<msg_type, const_ref_type>(cmh, const_ref_type(p0, p1, p2, p3));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::check_run2<msg_type>, 
				shared_from_this(), pIsClosed, boost::ref(cmh), msg_type(p0, p1, p2, p3)));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void notify_handler_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<param_list<msg_param<T0, T1, T2, T3> > >& cmh, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		typedef msg_param<T0, T1, T2, T3> msg_type;
		typedef const_ref_ex<T0, T1, T2, T3> const_ref_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*pIsClosed))
			{
				check_run3<msg_type, const_ref_type>(*cmh, const_ref_type(p0, p1, p2, p3));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_actor::check_run2_ptr<msg_type>, 
				shared_from_this(), pIsClosed, cmh, msg_type(p0, p1, p2, p3)));
		}
	}

	template <typename T /*msg_param*/>
	void check_run2(boost::shared_ptr<bool>& pIsClosed, param_list<T>& cmh, const T& src)
	{
		if (!_quited && !(*pIsClosed))
		{
			if (cmh._waiting)
			{
				cmh._waiting = false;
				if (cmh._hasTm)
				{
					cmh._hasTm = false;
					cancel_timer();
				}
				assert(cmh.get_size() == 0);
				cmh.get_param(src);
				pull_yield();//与check_run3区别
			} 
			else
			{
				cmh.push_back(src);
			}
		}
	}

	template <typename T /*msg_param*/>
	void check_run2_ptr(boost::shared_ptr<bool>& pIsClosed, boost::shared_ptr<param_list<T> >& cmh, const T& src)
	{
		check_run2(pIsClosed, *cmh, src);
	}

	template <typename T /*msg_param*/, typename RT /*msg_param::const_ref_type*/>
	void check_run3(param_list<T>& cmh, const RT& srcRef)
	{
		if (cmh._waiting)
		{
			cmh._waiting = false;
			if (cmh._hasTm)
			{
				cmh._hasTm = false;
				cancel_timer();
			}
			assert(cmh.get_size() == 0);
			cmh.get_param(srcRef);
			_strand->post(boost::bind(&boost_actor::run_one, shared_from_this()));//与check_run2区别
		} 
		else
		{
			cmh.push_back(srcRef);
		}
	}
public:
	/*!
	@brief 测试当前下的Actor栈是否安全
	*/
	void check_stack();

	/*!
	@brief 获取当前Actor剩余安全栈空间
	*/
	size_t stack_free_space();

	/*!
	@brief 获取当前Actor调度器
	*/
	shared_strand this_strand();

	/*!
	@brief 返回本对象的智能指针
	*/
	actor_handle shared_from_this();

	/*!
	@brief 获取当前ActorID号
	*/
	long long this_id();

	/*!
	@brief 获取Actor切换计数
	*/
	size_t yield_count();

	/*!
	@brief Actor切换计数清零
	*/
	void reset_yield();

	/*!
	@brief 开始运行建立好的Actor
	*/
	void notify_start_run();

	/*!
	@brief 强制退出该Actor，不可滥用，有可能会造成资源泄漏
	*/
	void notify_force_quit();

	/*!
	@brief 强制退出该Actor，完成后回调
	*/
	void notify_force_quit(const boost::function<void (bool)>& h);

	/*!
	@brief 暂停Actor
	*/
	void notify_suspend();
	void notify_suspend(const boost::function<void ()>& h);

	/*!
	@brief 恢复已暂停Actor
	*/
	void notify_resume();
	void notify_resume(const boost::function<void ()>& h);

	/*!
	@brief 切换挂起/非挂起状态
	*/
	void switch_pause_play();
	void switch_pause_play(const boost::function<void (bool isPaused)>& h);

	/*!
	@brief 等待Actor退出，在Actor所依赖的ios无关线程中使用
	*/
	bool outside_wait_quit();

	/*!
	@brief 添加一个Actor结束回调
	*/
	void append_quit_callback(const boost::function<void (bool)>& h);

	/*!
	@brief 启动一堆Actor
	*/
	void another_actors_start_run(const list<actor_handle>& anotherActors);

	/*!
	@brief 强制退出另一个Actor，并且等待完成
	*/
	__yield_interrupt bool another_actor_force_quit(actor_handle anotherActor);
	__yield_interrupt void another_actors_force_quit(const list<actor_handle>& anotherActors);

	/*!
	@brief 等待另一个Actor结束后返回
	*/
	__yield_interrupt bool another_actor_wait_quit(actor_handle anotherActor);
	__yield_interrupt void another_actors_wait_quit(const list<actor_handle>& anotherActors);

	/*!
	@brief 挂起另一个Actor，等待其所有子Actor都调用后才返回
	*/
	__yield_interrupt void another_actor_suspend(actor_handle anotherActor);
	__yield_interrupt void another_actors_suspend(const list<actor_handle>& anotherActors);

	/*!
	@brief 恢复另一个Actor，等待其所有子Actor都调用后才返回
	*/
	__yield_interrupt void another_actor_resume(actor_handle anotherActor);
	__yield_interrupt void another_actors_resume(const list<actor_handle>& anotherActors);

	/*!
	@brief 对另一个Actor进行挂起/恢复状态切换
	@return 都已挂起返回true，否则false
	*/
	__yield_interrupt bool another_actor_switch(actor_handle anotherActor);
	__yield_interrupt bool another_actors_switch(const list<actor_handle>& anotherActors);
private:
	void assert_enter();
	void time_out(int ms, const boost::function<void ()>& h);
	void expires_timer();
	void cancel_timer();
	void suspend_timer();
	void resume_timer();
	void start_run();
	void force_quit(const boost::function<void (bool)>& h);
	void suspend(const boost::function<void ()>& h);
	void resume(const boost::function<void ()>& h);
	void suspend();
	void resume();
	void run_one();
	void pull_yield();
	void push_yield();
	void force_quit_cb_handler();
	void exit_callback();
	void child_suspend_cb_handler();
	void child_resume_cb_handler();
	void outside_wait_quit_proxy(boost::condition_variable* conVar, boost::mutex* mutex, bool* rt);
	void outside_wait_quit_handler(boost::condition_variable* conVar, boost::mutex* mutex, bool* rt, bool ok);
private:
	void* _actorPull;///<Actor中断点恢复
	void* _actorPush;///<Actor中断点
	void* _stackTop;///<Actor栈顶
	long long _actorID;///<ActorID
	size_t _stackSize;///<Actor栈大小
	shared_strand _strand;///<Actor调度器
	DEBUG_OPERATION(bool _inActor);///<当前正在Actor内部执行标记
	bool _started;///<已经开始运行的标记
	bool _quited;///<已经准备退出标记
	bool _suspended;///<Actor挂起标记
	bool _hasNotify;///<当前Actor挂起，有外部触发准备进入Actor标记
	bool _isForce;///<是否是强制退出的标记，成功调用了force_quit
	size_t _yieldCount;//yield计数
	size_t _childOverCount;///<子Actor退出时计数
	size_t _childSuspendResumeCount;///<子Actor挂起/恢复计数
	boost::weak_ptr<boost_actor> _parentActor;///<父Actor
	main_func _mainFunc;///<Actor入口
	list<suspend_resume_option> _suspendResumeQueue;///<挂起/恢复操作队列
	list<actor_handle> _childActorList;///<子Actor集合
	list<boost::function<void (bool)> > _exitCallback;///<Actor结束后的回调函数，强制退出返回false，正常退出返回true
	list<boost::function<void ()> > _quitHandlerList;///<Actor退出时强制调用的函数，后注册的先执行
	timer_pck* _timerSleep;///<提供延时功能
	boost::weak_ptr<boost_actor> _weakThis;
};

#endif