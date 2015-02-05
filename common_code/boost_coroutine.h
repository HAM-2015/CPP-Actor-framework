#ifndef __BOOST_COROUTINE_H
#define __BOOST_COROUTINE_H

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <list>
#include "ios_proxy.h"
#include "shared_strand.h"
#include "time_out.h"
#include "wrapped_trig_handler.h"
#include "ref_ex.h"

/*
*		工业逻辑控制编程基础框架，使用"协程(coroutine)"技术，依赖boost_1.55或更新;
*		如果性能允许，所有轻量级逻辑（包括并行逻辑）可以在单线程io_service中执行（亦可多线程io_service）;
*		一个协程对象(coro_handle)依赖一个shared_strand，多个协程可以共同依赖同一个shared_strand;
*		支持强制结束、挂起/恢复、延时、多子任务（并发控制）;
*		在协程中或所依赖的io_service中进行长时间阻塞的操作或重量级运算，会严重影响依赖同一个io_service的协程响应速度;
*		默认协程栈空间64k字节，远比线程栈小，注意局部变量占用的空间以及调用层次（注意递归）;
*		QQ 591170887.
*/
class boost_coro;
typedef boost::shared_ptr<boost_coro> coro_handle;//协程句柄

using namespace std;

//此函数会进入协程中断标记，使用时注意逻辑的“连续性”可能会被打破
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

// 协程内使用，在使用了协程函数的异常捕捉 catch (...) 之前用于过滤协程退出异常并继续抛出，不然可能导致程序崩溃
#define CATCH_CORO_QUIT()\
catch (boost_coro::coro_force_quit& e)\
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
	typedef typename boost::function<void (T0, T1, T2, T3)> overflow_notify;
	typedef typename ref_ex<T0, T1, T2, T3> ref_type;
	typedef typename const_ref_ex<T0, T1, T2, T3> const_ref_type;

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

	void set_param(ref_type& rp) const
	{
		rp._p0 = _res0;
		rp._p1 = _res1;
		rp._p2 = _res2;
		rp._p3 = _res3;
	}

	T0 _res0;
	T1 _res1;
	T2 _res2;
	T3 _res3;
};

template <typename T0, typename T1, typename T2>
struct msg_param<T0, T1, T2, void>: public msg_param_base
{
	typedef typename boost::function<void (T0, T1, T2)> overflow_notify;
	typedef typename ref_ex<T0, T1, T2> ref_type;
	typedef typename const_ref_ex<T0, T1, T2> const_ref_type;

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

	void set_param(ref_type& rp) const
	{
		rp._p0 = _res0;
		rp._p1 = _res1;
		rp._p2 = _res2;
	}

	T0 _res0;
	T1 _res1;
	T2 _res2;
};

template <typename T0, typename T1>
struct msg_param<T0, T1, void, void>: public msg_param_base
{
	typedef typename boost::function<void (T0, T1)> overflow_notify;
	typedef typename ref_ex<T0, T1> ref_type;
	typedef typename const_ref_ex<T0, T1> const_ref_type;

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

	void set_param(ref_type& rp) const
	{
		rp._p0 = _res0;
		rp._p1 = _res1;
	}

	T0 _res0;
	T1 _res1;
};

template <typename T0>
struct msg_param<T0, void, void, void>: public msg_param_base
{
	typedef typename boost::function<void (T0)> overflow_notify;
	typedef typename ref_ex<T0> ref_type;
	typedef typename const_ref_ex<T0> const_ref_type;

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

	void set_param(ref_type& rp) const
	{
		rp._p0 = _res0;
	}

	T0 _res0;
};

class param_list_base
{
	friend boost_coro;
private:
	param_list_base(const param_list_base&);
	param_list_base& operator =(const param_list_base&);
protected:
	param_list_base();
	virtual ~param_list_base();
	bool closed();
	void begin(long long coroID);
	virtual size_t get_size() = 0;
	virtual void close() = 0;
	virtual void clear() = 0;
protected:
	boost::shared_ptr<bool> _ptrClosed;
	bool _waiting;
	bool _timeout;
	bool _hasTm;
	DEBUG_OPERATION(long long _coroID);
};

template <typename T>
class param_list: public param_list_base
{
	friend boost_coro;
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

	void set_param(const T& p)
	{
		assert(!(*_ptrClosed) && _dstRefPt);
		p.set_param(*_dstRefPt);
		DEBUG_OPERATION(_dstRefPt = NULL);
	}

	void set_param(const const_ref_type& p)
	{
		assert(!(*_ptrClosed) && _dstRefPt);
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
	friend boost_coro;
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
		if (_ptrClosed)
		{
			assert(!(*_ptrClosed));
			(*_ptrClosed) = true;
			_waiting = false;
			_ptrClosed.reset();
		}
	}

	size_t push_back(const T& p)
	{
		assert(!(*_ptrClosed));
		_params.push_back(p);
		return _params.size();
	}

	T* front()
	{
		assert(!(*_ptrClosed));
		if (!_params.empty())
		{
			return &_params.front();
		}
		return NULL;
	}

	size_t pop_front()
	{
		assert(!(*_ptrClosed));
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
	friend boost_coro;
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
		if (_ptrClosed)
		{
			assert(!(*_ptrClosed));
			(*_ptrClosed) = true;
			_waiting = false;
			_ptrClosed.reset();
			_ofh.clear();
		}
	}

	size_t push_back(const T& p)
	{
		assert(!(*_ptrClosed));
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
		assert(!(*_ptrClosed));
		if (!_params.empty())
		{
			return &_params.front();
		}
		return NULL;
	}

	size_t pop_front()
	{
		assert(!(*_ptrClosed));
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
	friend boost_coro;
public:
	size_t get_size();

	void clear();
protected:
	null_param_list();

	void close();

	size_t count;
};

template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class coro_msg_handle: public param_list_no_limit<msg_param<T0, T1, T2, T3> >
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<coro_msg_handle<T0, T1, T2, T3> > ptr;

	static boost::shared_ptr<coro_msg_handle<T0, T1, T2, T3> > make_ptr()
	{
		return boost::shared_ptr<coro_msg_handle<T0, T1, T2, T3> >(new coro_msg_handle<T0, T1, T2, T3>);
	}
};

template <typename T0, typename T1, typename T2>
class coro_msg_handle<T0, T1, T2, void>: public param_list_no_limit<msg_param<T0, T1, T2> >
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<coro_msg_handle<T0, T1, T2> > ptr;

	static boost::shared_ptr<coro_msg_handle<T0, T1, T2> > make_ptr()
	{
		return boost::shared_ptr<coro_msg_handle<T0, T1, T2> >(new coro_msg_handle<T0, T1, T2>);
	}
};

template <typename T0, typename T1>
class coro_msg_handle<T0, T1, void, void>: public param_list_no_limit<msg_param<T0, T1> >
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<coro_msg_handle<T0, T1> > ptr;

	static boost::shared_ptr<coro_msg_handle<T0, T1> > make_ptr()
	{
		return boost::shared_ptr<coro_msg_handle<T0, T1> >(new coro_msg_handle<T0, T1>);
	}
};

template <typename T0>
class coro_msg_handle<T0, void, void, void>: public param_list_no_limit<msg_param<T0> >
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<coro_msg_handle<T0> > ptr;

	static boost::shared_ptr<coro_msg_handle<T0> > make_ptr()
	{
		return boost::shared_ptr<coro_msg_handle<T0> >(new coro_msg_handle<T0>);
	}
};

template <>
class coro_msg_handle<void, void, void, void>: public null_param_list
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<coro_msg_handle<> > ptr;

	static boost::shared_ptr<coro_msg_handle<> > make_ptr()
	{
		return boost::shared_ptr<coro_msg_handle<> >(new coro_msg_handle<>);
	}
};
//////////////////////////////////////////////////////////////////////////
template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class coro_msg_limit_handle: public param_list_limit<msg_param<T0, T1, T2, T3> >
{
	friend boost_coro;
public:
	typedef typename param_list_limit<msg_param<T0, T1, T2, T3> >::overflow_notify overflow_notify;
	typedef boost::shared_ptr<coro_msg_limit_handle<T0, T1, T2, T3> > ptr;

	/*!
	@param maxBuff 最大缓存个数
	@param ofh 溢出触发函数
	*/
	coro_msg_limit_handle(int maxBuff,  const overflow_notify& ofh)
		:param_list_limit<msg_param<T0, T1, T2, T3> >(maxBuff, ofh)
	{

	}

	static boost::shared_ptr<coro_msg_limit_handle<T0, T1, T2, T3> > make_ptr(int maxBuff,  const overflow_notify& ofh)
	{
		return boost::shared_ptr<coro_msg_limit_handle<T0, T1, T2, T3> >(new coro_msg_limit_handle<T0, T1, T2, T3>(maxBuff, ofh));
	}
};

template <typename T0, typename T1, typename T2>
class coro_msg_limit_handle<T0, T1, T2, void>: public param_list_limit<msg_param<T0, T1, T2> >
{
	friend boost_coro;
public:
	typedef typename param_list_limit<msg_param<T0, T1, T2> >::overflow_notify overflow_notify;
	typedef boost::shared_ptr<coro_msg_limit_handle<T0, T1, T2> > ptr;

	coro_msg_limit_handle(int maxBuff,  const overflow_notify& ofh)
		:param_list_limit<msg_param<T0, T1, T2> >(maxBuff, ofh)
	{

	}

	static boost::shared_ptr<coro_msg_limit_handle<T0, T1, T2> > make_ptr(int maxBuff,  const overflow_notify& ofh)
	{
		return boost::shared_ptr<coro_msg_limit_handle<T0, T1, T2> >(new coro_msg_limit_handle<T0, T1, T2>(maxBuff, ofh));
	}
};

template <typename T0, typename T1>
class coro_msg_limit_handle<T0, T1, void, void>: public param_list_limit<msg_param<T0, T1> >
{
	friend boost_coro;
public:
	typedef typename param_list_limit<msg_param<T0, T1> >::overflow_notify overflow_notify;
	typedef boost::shared_ptr<coro_msg_limit_handle<T0, T1> > ptr;

	coro_msg_limit_handle(int maxBuff,  const overflow_notify& ofh)
		:param_list_limit<msg_param<T0, T1> >(maxBuff, ofh)
	{

	}

	static boost::shared_ptr<coro_msg_limit_handle<T0, T1> > make_ptr(int maxBuff,  const overflow_notify& ofh)
	{
		return boost::shared_ptr<coro_msg_limit_handle<T0, T1> >(new coro_msg_limit_handle<T0, T1>(maxBuff, ofh));
	}
};

template <typename T0>
class coro_msg_limit_handle<T0, void, void, void>: public param_list_limit<msg_param<T0> >
{
	friend boost_coro;
public:
	typedef typename param_list_limit<msg_param<T0> >::overflow_notify overflow_notify;
	typedef boost::shared_ptr<coro_msg_limit_handle<T0> > ptr;

	coro_msg_limit_handle(int maxBuff,  const overflow_notify& ofh)
		:param_list_limit<msg_param<T0> >(maxBuff, ofh)
	{

	}

	static boost::shared_ptr<coro_msg_limit_handle<T0> > make_ptr(int maxBuff,  const overflow_notify& ofh)
	{
		return boost::shared_ptr<coro_msg_limit_handle<T0> >(new coro_msg_limit_handle<T0>(maxBuff, ofh));
	}
};

class async_trig_base
{
	friend boost_coro;
public:
	async_trig_base();
	virtual ~async_trig_base();
private:
	void begin(long long coroID);
	void close();
	async_trig_base(const async_trig_base&);
	async_trig_base& operator=(const async_trig_base&);
private:
	boost::shared_ptr<bool> _ptrClosed;
	bool _notify;
	bool _waiting;
	bool _timeout;
	bool _hasTm;
	DEBUG_OPERATION(long long _coroID);
};

template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class async_trig_handle: public async_trig_base
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> > ptr;

	static boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> >(new async_trig_handle<T0, T1, T2, T3>);
	}
private:
	msg_param<T0, T1, T2, T3> _temp;
};

template <typename T0, typename T1, typename T2>
class async_trig_handle<T0, T1, T2, void>: public async_trig_base
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<async_trig_handle<T0, T1, T2> > ptr;

	static boost::shared_ptr<async_trig_handle<T0, T1, T2> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<T0, T1, T2> >(new async_trig_handle<T0, T1, T2>);
	}
private:
	msg_param<T0, T1, T2> _temp;
};

template <typename T0, typename T1>
class async_trig_handle<T0, T1, void, void>: public async_trig_base
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<async_trig_handle<T0, T1> > ptr;

	static boost::shared_ptr<async_trig_handle<T0, T1> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<T0, T1> >(new async_trig_handle<T0, T1>);
	}
private:
	msg_param<T0, T1> _temp;
};

template <typename T0>
class async_trig_handle<T0, void, void, void>: public async_trig_base
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<async_trig_handle<T0> > ptr;

	static boost::shared_ptr<async_trig_handle<T0> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<T0> >(new async_trig_handle<T0>);
	}
private:
	msg_param<T0> _temp;
};

template <>
class async_trig_handle<void, void, void, void>: public async_trig_base
{
	friend boost_coro;
public:
	typedef boost::shared_ptr<async_trig_handle<> > ptr;

	static boost::shared_ptr<async_trig_handle<> > make_ptr()
	{
		return boost::shared_ptr<async_trig_handle<> >(new async_trig_handle<>);
	}
};
//////////////////////////////////////////////////////////////////////////
/*!
@brief 子协程句柄，不可拷贝
*/
class child_coro_handle 
{
public:
	typedef boost::shared_ptr<child_coro_handle> ptr;
private:
	friend boost_coro;
	/*!
	@brief 子协程句柄参数，child_coro_handle内使用
	*/
	struct child_coro_param
	{
#ifdef _DEBUG
		child_coro_param();

		child_coro_param(child_coro_param& s);

		~child_coro_param();

		child_coro_param& operator =(child_coro_param& s);

		bool _isCopy;
#endif
		coro_handle _coro;///<本协程
		list<coro_handle>::iterator _coroIt;///<保存在父协程集合中的节点
	};
private:
	child_coro_handle(child_coro_handle&);

	child_coro_handle& operator =(child_coro_handle&);
public:
	child_coro_handle();

	child_coro_handle(child_coro_param& s);

	~child_coro_handle();

	child_coro_handle& operator =(child_coro_param& s);

	coro_handle get_coro();

	static ptr make_ptr();
private:
	void quited_set();

	void cancel_quit_it();

	void* operator new(size_t s);
public:
	void operator delete(void* p);
private:
	DEBUG_OPERATION(list<boost::function<void ()> >::iterator _qh);
	bool _norQuit;///<是否正常退出
	bool _quited;///<检测是否已经关闭
	child_coro_param _param;
};

class boost_coro
{
	struct suspend_resume_option 
	{
		bool _isSuspend;
		boost::function<void ()> _h;
	};

	class boost_coro_run;
	friend boost_coro_run;
public:
	/*!
	@brief 协程被强制退出的异常类型
	*/
	struct coro_force_quit { };

	/*!
	@brief 协程入口函数体
	*/
	typedef boost::function<void (boost_coro*)> main_func;
private:
	boost_coro();
	boost_coro(const boost_coro&);
	boost_coro& operator =(const boost_coro&);
public:
	~boost_coro();
public:
	/*!
	@brief 创建一个协程
	@param coroStrand 协程所依赖的strand
	@param mainFunc 协程执行入口
	@param stackSize 协程栈大小，默认64k字节，必须是4k的整数倍，最小4k，最大1M
	*/
	static coro_handle create(shared_strand coroStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 创建一个协程，在coroStrand所依赖的ios无关线程中使用
	*/
	static coro_handle outside_create(shared_strand coroStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 创建一个协程，在本coroStrand调度器执行体中使用
	*/
	static coro_handle local_create(shared_strand coroStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 同上，带完成协程后的回调通知
	@param cb 协程完成后的触发函数，false强制结束的，true正常结束
	*/
	static coro_handle local_create(shared_strand coroStrand, const main_func& mainFunc, 
		const boost::function<void (bool)>& cb, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 异步创建一个协程，创建成功后通过回调函数通知
	*/
	static void async_create(shared_strand coroStrand, const main_func& mainFunc, 
		const boost::function<void (coro_handle)>& ch, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 同上，带完成协程后的回调通知
	*/
	static void async_create(shared_strand coroStrand, const main_func& mainFunc, 
		const boost::function<void (coro_handle)>& ch, const boost::function<void (bool)>& cb, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 启用堆栈内存池
	*/
	static void enable_stack_pool();

	/*!
	@brief 禁用创建协程时自动构造定时器
	*/
	static void disable_auto_make_timer();
public:
	/*!
	@brief 创建一个子协程，父协程终止时，子协程也终止（在子协程都完全退出后，父协程才结束）
	@param coroStrand 子协程依赖的strand
	@param mainFunc 子协程入口函数
	@param stackSize 协程栈大小，4k的整数倍（最大1MB）
	@return 子协程句柄，使用 child_coro_handle 接收返回值
	*/
	__yield_interrupt child_coro_handle::child_coro_param create_child_coro(shared_strand coroStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);
	__yield_interrupt child_coro_handle::child_coro_param create_child_coro(const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 开始运行子协程，只能调用一次
	*/
	void child_coro_run(child_coro_handle& coroHandle);
	void child_coros_run(const list<child_coro_handle::ptr>& coroHandles);

	/*!
	@brief 强制终止一个子协程
	@return 已经被正常退出返回true，被强制退出返回false
	*/
	__yield_interrupt bool child_coro_quit(child_coro_handle& coroHandle);
	__yield_interrupt bool child_coros_quit(const list<child_coro_handle::ptr>& coroHandles);

	/*!
	@brief 等待一个子协程完成后返回
	@return 正常退出的返回true，否则false
	*/
	__yield_interrupt bool child_coro_wait_quit(child_coro_handle& coroHandle);

	/*!
	@brief 等待一组子协程完成后返回
	@return 都正常退出的返回true，否则false
	*/
	__yield_interrupt bool child_coros_wait_quit(const list<child_coro_handle::ptr>& coroHandles);

	/*!
	@brief 挂起子协程
	*/
	__yield_interrupt void child_coro_suspend(child_coro_handle& coroHandle);
	
	/*!
	@brief 挂起一组子协程
	*/
	__yield_interrupt void child_coros_suspend(const list<child_coro_handle::ptr>& coroHandles);

	/*!
	@brief 恢复子协程
	*/
	__yield_interrupt void child_coro_resume(child_coro_handle& coroHandle);
	
	/*!
	@brief 恢复一组子协程
	*/
	__yield_interrupt void child_coros_resume(const list<child_coro_handle::ptr>& coroHandles);

	/*!
	@brief 分离出子协程
	*/
	coro_handle child_coro_peel(child_coro_handle& coroHandle);

	/*!
	@brief 创建另一个协程，协程执行完成后返回
	*/
	__yield_interrupt bool run_child_coro_complete(shared_strand coroStrand, const main_func& h, size_t stackSize = DEFAULT_STACKSIZE);
	__yield_interrupt bool run_child_coro_complete(const main_func& h, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 延时等待，协程内部禁止使用操作系统API Sleep()
	@param ms 等待毫秒数，等于0时暂时放弃协程执行，直到下次被调度器触发
	*/
	__yield_interrupt void sleep(int ms);

	/*!
	@brief 调用disable_auto_make_timer后，使用这个打开当前协程定时器
	*/
	void open_timer();

	/*!
	@brief 获取父协程
	*/
	coro_handle parent_coro();

	/*!
	@brief 获取子协程
	*/
	const list<coro_handle>& child_coros();
public:
	typedef list<boost::function<void ()> >::iterator quit_handle;

	/*!
	@brief 注册一个资源释放函数，在强制退出协程时调用
	*/
	quit_handle regist_quit_handler(const boost::function<void ()>& quitHandler);

	/*!
	@brief 注销资源释放函数
	*/
	void cancel_quit_handler(quit_handle rh);
public:
	/*!
	@brief 创建一个异步触发函数，使用wait_trig()等待
	@param th 触发句柄
	@return 触发函数对象，可以多次调用(线程安全)，但只能wait_trig()到第一次调用的值
	*/
	boost::function<void ()> begin_trig(async_trig_handle<>& th);
	boost::function<void ()> begin_trig(boost::shared_ptr<async_trig_handle<> > th);

	template <typename T0>
	boost::function<void (T0)> begin_trig(async_trig_handle<T0>& th)
	{
		th.begin(_coroID);
		return boost::bind(&boost_coro::async_trig_handler<T0>, shared_from_this(), th._ptrClosed, boost::ref(th), _1);
	}

	template <typename T0>
	boost::function<void (T0)> begin_trig(boost::shared_ptr<async_trig_handle<T0> > th)
	{
		th->begin(_coroID);
		return boost::bind(&boost_coro::async_trig_handler_ptr<T0>, shared_from_this(), th->_ptrClosed, th, _1);
	}

	template <typename T0, typename T1>
	boost::function<void (T0, T1)> begin_trig(async_trig_handle<T0, T1>& th)
	{
		th.begin(_coroID);
		return boost::bind(&boost_coro::async_trig_handler<T0, T1>, shared_from_this(), th._ptrClosed, boost::ref(th), _1, _2);
	}

	template <typename T0, typename T1>
	boost::function<void (T0, T1)> begin_trig(boost::shared_ptr<async_trig_handle<T0, T1> > th)
	{
		th->begin(_coroID);
		return boost::bind(&boost_coro::async_trig_handler_ptr<T0, T1>, shared_from_this(), th->_ptrClosed, th, _1, _2);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void (T0, T1, T2)> begin_trig(async_trig_handle<T0, T1, T2>& th)
	{
		th.begin(_coroID);
		return boost::bind(&boost_coro::async_trig_handler<T0, T1, T2>, shared_from_this(), th._ptrClosed, boost::ref(th), _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void (T0, T1, T2)> begin_trig(boost::shared_ptr<async_trig_handle<T0, T1, T2> > th)
	{
		th->begin(_coroID);
		return boost::bind(&boost_coro::async_trig_handler_ptr<T0, T1, T2>, shared_from_this(), th->_ptrClosed, th, _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void (T0, T1, T2, T3)> begin_trig(async_trig_handle<T0, T1, T2, T3>& th)
	{
		th.begin(_coroID);
		return boost::bind(&boost_coro::async_trig_handler<T0, T1, T2, T3>, shared_from_this(), th._ptrClosed, boost::ref(th), _1, _2, _3, _4);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void (T0, T1, T2, T3)> begin_trig(boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> > th)
	{
		th->begin(_coroID);
		return boost::bind(&boost_coro::async_trig_handler_ptr<T0, T1, T2, T3>, shared_from_this(), th->_ptrClosed, th, _1, _2, _3, _4);
	}

	/*!
	@brief 等待begin_trig创建的回调句柄
	@param th 异步句柄
	@param tm 异步等待超时ms，超时后返回false
	@return 超时后返回false
	*/
	__yield_interrupt bool wait_trig(async_trig_handle<>& th, int tm = -1);

	template <typename T0>
	__yield_interrupt bool wait_trig(async_trig_handle<T0>& th, T0& r0, int tm = -1)
	{
		assert(th._coroID == _coroID);
		assert_enter();
		if (!async_trig_push(th, tm))
		{
			return false;
		}
		r0 = th._temp._res0;
		close_trig(th);
		return true;
	}

	template <typename T0>
	__yield_interrupt T0 wait_trig(async_trig_handle<T0>& th)
	{
		assert(th._coroID == _coroID);
		assert_enter();
		async_trig_push(th, -1);
		close_trig(th);
		return th._temp._res0;
	}

	template <typename T0, typename T1>
	__yield_interrupt bool wait_trig(async_trig_handle<T0, T1>& th, T0& r0, T1& r1, int tm = -1)
	{
		assert(th._coroID == _coroID);
		assert_enter();
		if (!async_trig_push(th, tm))
		{
			return false;
		}
		r0 = th._temp._res0;
		r1 = th._temp._res1;
		close_trig(th);
		return true;
	}

	template <typename T0, typename T1, typename T2>
	__yield_interrupt bool wait_trig(async_trig_handle<T0, T1, T2>& th, T0& r0, T1& r1, T2& r2, int tm = -1)
	{
		assert(th._coroID == _coroID);
		assert_enter();
		if (!async_trig_push(th, tm))
		{
			return false;
		}
		r0 = th._temp._res0;
		r1 = th._temp._res1;
		r2 = th._temp._res2;
		close_trig(th);
		return true;
	}

	template <typename T0, typename T1, typename T2, typename T3>
	__yield_interrupt bool wait_trig(async_trig_handle<T0, T1, T2, T3>& th, T0& r0, T1& r1, T2& r2, T3& r3, int tm = -1)
	{
		assert(th._coroID == _coroID);
		assert_enter();
		if (!async_trig_push(th, tm))
		{
			return false;
		}
		r0 = th._temp._res0;
		r1 = th._temp._res1;
		r2 = th._temp._res2;
		r3 = th._temp._res3;
		close_trig(th);
		return true;
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
		assert(th._ptrClosed);
		assert(_timerSleep);
		_timerSleep->timeOutAgain(ms, boost::bind(&boost_coro::_async_trig_handler<T0>, shared_from_this(), 
			th._ptrClosed, boost::ref(th), p0));
	}

	template <typename T0>
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<T0> > th, const T0& p0)
	{
		assert_enter();
		assert(th->_ptrClosed);
		assert(_timerSleep);
		_timerSleep->timeOutAgain(ms, boost::bind(&boost_coro::_async_trig_handler_ptr<T0>, shared_from_this(), 
			th->_ptrClosed, th, p0));
	}

	template <typename T0, typename T1>
	void delay_trig(int ms, async_trig_handle<T0, T1>& th, const T0& p0, const T1& p1)
	{
		assert_enter();
		assert(th._ptrClosed);
		assert(_timerSleep);
		_timerSleep->timeOutAgain(ms, boost::bind(&boost_coro::_async_trig_handler<T0, T1>, shared_from_this(), 
			th._ptrClosed, boost::ref(th), p0, p1));
	}

	template <typename T0, typename T1>
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<T0, T1> > th, const T0& p0, const T1& p1)
	{
		assert_enter();
		assert(th->_ptrClosed);
		assert(_timerSleep);
		_timerSleep->timeOutAgain(ms, boost::bind(&boost_coro::_async_trig_handler_ptr<T0, T1>, shared_from_this(), 
			th->_ptrClosed, th, p0, p1));
	}

	template <typename T0, typename T1, typename T2>
	void delay_trig(int ms, async_trig_handle<T0, T1, T2>& th, const T0& p0, const T1& p1, const T2& p2)
	{
		assert_enter();
		assert(th._ptrClosed);
		assert(_timerSleep);
		_timerSleep->timeOutAgain(ms, boost::bind(&boost_coro::_async_trig_handler<T0, T1, T2>, shared_from_this(), 
			th._ptrClosed, boost::ref(th), p0, p1, p2));
	}

	template <typename T0, typename T1, typename T2>
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<T0, T1, T2> > th, const T0& p0, const T1& p1, const T2& p2)
	{
		assert_enter();
		assert(th->_ptrClosed);
		assert(_timerSleep);
		_timerSleep->timeOutAgain(ms, boost::bind(&boost_coro::_async_trig_handler_ptr<T0, T1, T2>, shared_from_this(), 
			th->_ptrClosed, th, p0, p1, p2));
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void delay_trig(int ms, async_trig_handle<T0, T1, T2, T3>& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		assert_enter();
		assert(th._ptrClosed);
		assert(_timerSleep);
		_timerSleep->timeOutAgain(ms, boost::bind(&boost_coro::_async_trig_handler<T0, T1, T2, T3>, shared_from_this(), 
			th._ptrClosed, boost::ref(th), p0, p1, p2, p3));
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void delay_trig(int ms, boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> > th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		assert_enter();
		assert(th->_ptrClosed);
		assert(_timerSleep);
		_timerSleep->timeOutAgain(ms, boost::bind(&boost_coro::_async_trig_handler_ptr<T0, T1, T2, T3>, shared_from_this(), 
			th->_ptrClosed, th, p0, p1, p2, p3));
	}

	/*!
	@brief 取消内部定时器触发
	*/
	void cancel_delay_trig();
public:
	/*!
	@brief 调用一个异步函数，异步回调完成后返回
	*/
	__yield_interrupt void trig(const boost::function<void (boost::function<void ()>)>& h);

	__yield_interrupt void trig_ret(shared_strand extStrand, const boost::function<void ()>& h);

	template <typename T0>
	__yield_interrupt void trig(const boost::function<void (boost::function<void (T0)>)>& h, __out T0& r0)
	{
		assert_enter();
#ifdef _DEBUG
		h(wrapped_trig_handler<boost::function<void (T0)> >(
			boost::bind(&boost_coro::trig_handler<T0>, shared_from_this(), boost::ref(r0), _1)));
#else
		h(boost::bind(&boost_coro::trig_handler<T0>, shared_from_this(), boost::ref(r0), _1));
#endif
		push_yield();
	}

	template <typename T0>
	__yield_interrupt T0 trig_ret(shared_strand extStrand, const boost::function<T0 ()>& h)
	{
		T0 r0;
		trig<T0>(boost::bind(&boost_strand::asyncInvoke<T0>, extStrand, h, _1), r0);
		return r0;
	}

	template <typename T0, typename T1>
	__yield_interrupt void trig(const boost::function<void (boost::function<void (T0, T1)>)>& h, __out T0& r0, __out T1& r1)
	{
		assert_enter();
		ref_ex<T0, T1> mulRef(r0, r1);
#ifdef _DEBUG
		h(wrapped_trig_handler<boost::function<void (T0, T1)> >(
			boost::bind(&boost_coro::trig_handler<T0, T1>, shared_from_this(), boost::ref(mulRef), _1, _2)));
#else
		h(boost::bind(&boost_coro::trig_handler<T0, T1>, shared_from_this(), boost::ref(mulRef), _1, _2));
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
			boost::bind(&boost_coro::trig_handler<T0, T1, T2>, shared_from_this(), 
			boost::ref(mulRef), _1, _2, _3)));
#else
		h(boost::bind(&boost_coro::trig_handler<T0, T1, T2>, shared_from_this(), 
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
			boost::bind(&boost_coro::trig_handler<T0, T1, T2, T3>, shared_from_this(), 
			boost::ref(mulRef), _1, _2, _3, _4)));
#else
		h(boost::bind(&boost_coro::trig_handler<T0, T1, T2, T3>, shared_from_this(), 
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
	boost::function<void()> make_msg_notify(coro_msg_handle<>& cmh);
	boost::function<void()> make_msg_notify(boost::shared_ptr<coro_msg_handle<> > cmh);

	template <typename T0>
	boost::function<void(T0)> make_msg_notify(param_list<msg_param<T0> >& cmh)
	{
		cmh.begin(_coroID);
		return boost::bind(&boost_coro::notify_handler<T0>, shared_from_this(), cmh._ptrClosed, boost::ref(cmh), _1);
	}

	template <typename T0>
	boost::function<void(T0)> make_msg_notify(boost::shared_ptr<coro_msg_handle<T0> > cmh)
	{
		cmh->begin(_coroID);
		return boost::bind(&boost_coro::notify_handler_ptr<T0>, shared_from_this(), cmh->_ptrClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0> > >(cmh), _1);
	}

	template <typename T0>
	boost::function<void(T0)> make_msg_notify(boost::shared_ptr<coro_msg_limit_handle<T0> > cmh)
	{
		cmh->begin(_coroID);
		return boost::bind(&boost_coro::notify_handler_ptr<T0>, shared_from_this(), cmh->_ptrClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0> > >(cmh), _1);
	}

	template <typename T0, typename T1>
	boost::function<void(T0, T1)> make_msg_notify(param_list<msg_param<T0, T1> >& cmh)
	{
		cmh.begin(_coroID);
		return boost::bind(&boost_coro::notify_handler<T0, T1>, shared_from_this(), cmh._ptrClosed, boost::ref(cmh), _1, _2);
	}

	template <typename T0, typename T1>
	boost::function<void(T0, T1)> make_msg_notify(boost::shared_ptr<coro_msg_handle<T0, T1> > cmh)
	{
		cmh->begin(_coroID);
		return boost::bind(&boost_coro::notify_handler_ptr<T0, T1>, shared_from_this(), cmh->_ptrClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1> > >(cmh), _1, _2);
	}

	template <typename T0, typename T1>
	boost::function<void(T0, T1)> make_msg_notify(boost::shared_ptr<coro_msg_limit_handle<T0, T1> > cmh)
	{
		cmh->begin(_coroID);
		return boost::bind(&boost_coro::notify_handler_ptr<T0, T1>, shared_from_this(), cmh->_ptrClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1> > >(cmh), _1, _2);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void(T0, T1, T2)> make_msg_notify(param_list<msg_param<T0, T1, T2> >& cmh)
	{
		cmh.begin(_coroID);
		return boost::bind(&boost_coro::notify_handler<T0, T1, T2>, shared_from_this(), cmh._ptrClosed, boost::ref(cmh), _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void(T0, T1, T2)> make_msg_notify(boost::shared_ptr<coro_msg_handle<T0, T1, T2> > cmh)
	{
		cmh->begin(_coroID);
		return boost::bind(&boost_coro::notify_handler_ptr<T0, T1, T2>, shared_from_this(), cmh->_ptrClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1, T2> > >(cmh), _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2>
	boost::function<void(T0, T1, T2)> make_msg_notify(boost::shared_ptr<coro_msg_limit_handle<T0, T1, T2> > cmh)
	{
		cmh->begin(_coroID);
		return boost::bind(&boost_coro::notify_handler_ptr<T0, T1, T2>, shared_from_this(), cmh->_ptrClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1, T2> > >(cmh), _1, _2, _3);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void(T0, T1, T2, T3)> make_msg_notify(param_list<msg_param<T0, T1, T2, T3> >& cmh)
	{
		cmh.begin(_coroID);
		return boost::bind(&boost_coro::notify_handler<T0, T1, T2, T3>, shared_from_this(), cmh._ptrClosed, boost::ref(cmh), _1, _2, _3, _4);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void(T0, T1, T2, T3)> make_msg_notify(boost::shared_ptr<coro_msg_handle<T0, T1, T2, T3> > cmh)
	{
		cmh->begin(_coroID);
		return boost::bind(&boost_coro::notify_handler_ptr<T0, T1, T2, T3>, shared_from_this(), cmh->_ptrClosed, 
			boost::static_pointer_cast<param_list<msg_param<T0, T1, T2, T3> > >(cmh), _1, _2, _3, _4);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	boost::function<void(T0, T1, T2, T3)> make_msg_notify(boost::shared_ptr<coro_msg_limit_handle<T0, T1, T2, T3> > cmh)
	{
		cmh->begin(_coroID);
		return boost::bind(&boost_coro::notify_handler_ptr<T0, T1, T2, T3>, shared_from_this(), cmh->_ptrClosed, 
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
	__yield_interrupt bool pump_msg(coro_msg_handle<>& cmh, int tm = -1);

	template <typename T0>
	__yield_interrupt bool pump_msg(param_list<msg_param<T0> >& cmh, __out T0& r0, int tm = -1)
	{
		typedef typename msg_param<T0> param_type;
		assert(cmh._coroID == _coroID);
		assert_enter();
		assert(cmh._ptrClosed);
		param_type* param = cmh.front();
		if (param)
		{
			r0 = param->_res0;
			cmh.pop_front();
			return true;
		}
		cmh._waiting = true;
		param_type::ref_type ref(r0);
		cmh.set_ref(ref);
		if (!pump_msg_push(cmh, tm))
		{
			return false;
		}
		return true;
	}

	template <typename T0>
	__yield_interrupt T0 pump_msg(param_list<msg_param<T0> >& cmh)
	{
		T0 r0;
		pump_msg(cmh, r0);
		return r0;
	}

	template <typename T0, typename T1>
	__yield_interrupt bool pump_msg(param_list<msg_param<T0, T1> >& cmh, __out T0& r0, __out T1& r1, int tm = -1)
	{
		typedef typename msg_param<T0, T1> param_type;
		assert(cmh._coroID == _coroID);
		assert_enter();
		assert(cmh._ptrClosed);
		param_type* param = cmh.front();
		if (param)
		{
			r0 = param->_res0;
			r1 = param->_res1;
			cmh.pop_front();
			return true;
		}
		cmh._waiting = true;
		param_type::ref_type ref(r0, r1);
		cmh.set_ref(ref);
		if (!pump_msg_push(cmh, tm))
		{
			return false;
		}
		return true;
	}

	template <typename T0, typename T1, typename T2>
	__yield_interrupt bool pump_msg(param_list<msg_param<T0, T1, T2> >& cmh, __out T0& r0, __out T1& r1, __out T2& r2, int tm = -1)
	{
		typedef typename msg_param<T0, T1, T2> param_type;
		assert(cmh._coroID == _coroID);
		assert_enter();
		assert(cmh._ptrClosed);
		param_type* param = cmh.front();
		if (param)
		{
			r0 = param->_res0;
			r1 = param->_res1;
			r2 = param->_res2;
			cmh.pop_front();
			return true;
		}
		cmh._waiting = true;
		param_type::ref_type ref(r0, r1, r2);
		cmh.set_ref(ref);
		if (!pump_msg_push(cmh, tm))
		{
			return false;
		}
		return true;
	}

	template <typename T0, typename T1, typename T2, typename T3>
	__yield_interrupt bool pump_msg(param_list<msg_param<T0, T1, T2, T3> >& cmh, __out T0& r0, __out T1& r1, __out T2& r2, __out T3& r3, int tm = -1)
	{
		typedef typename msg_param<T0, T1, T2, T3> param_type;
		assert(cmh._coroID == _coroID);
		assert_enter();
		assert(cmh._ptrClosed);
		param_type* param = cmh.front();
		if (param)
		{
			r0 = param->_res0;
			r1 = param->_res1;
			r2 = param->_res2;
			r3 = param->_res3;
			cmh.pop_front();
			return true;
		}
		cmh._waiting = true;
		param_type::ref_type ref(r0, r1, r2, r3);
		cmh.set_ref(ref);
		if (!pump_msg_push(cmh, tm))
		{
			return false;
		}
		return true;
	}
private:
	bool pump_msg_push(param_list_base& pm, int tm);

	void pump_msg_timeout(param_list_base& pm);

	void trig_handler();

	template <typename T0>
	void trig_handler(T0& r, const T0& p0)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				r = p0;//必须在此(_strand内部)处理参数，如果协程已退出(_quited == true)，将导致r失效
				_strand->post(boost::bind(&boost_coro::run_one, shared_from_this()));
			}
		} 
		else
		{//此时不能处理参数，因为 r 可能已失效
			_strand->post(boost::bind(&boost_coro::_trig_handler<T0>, shared_from_this(), boost::ref(r), p0));
		}
	}

	template <typename T0>
	void _trig_handler(T0& r, const T0& p0)
	{
		if (!_quited)
		{
			r = p0;//必须在此(_strand内部)处理参数，如果协程已退出(_quited == true)，将导致r失效
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
				r._p0 = p0;
				r._p1 = p1;
				_strand->post(boost::bind(&boost_coro::run_one, shared_from_this()));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::_trig_handler<T0, T1>, shared_from_this(), boost::ref(r), p0, p1));
		}
	}

	template <typename T0, typename T1>
	void _trig_handler(ref_ex<T0, T1>& r, const T0& p0, const T1& p1)
	{
		if (!_quited)
		{
			r._p0 = p0;
			r._p1 = p1;
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
				r._p0 = p0;
				r._p1 = p1;
				r._p2 = p2;
				_strand->post(boost::bind(&boost_coro::run_one, shared_from_this()));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::_trig_handler<T0, T1, T2>, shared_from_this(), boost::ref(r), p0, p1, p2));
		}
	}

	template <typename T0, typename T1, typename T2>
	void _trig_handler(ref_ex<T0, T1, T2>& r, const T0& p0, const T1& p1, const T2& p2)
	{
		if (!_quited)
		{
			r._p0 = p0;
			r._p1 = p1;
			r._p2 = p2;
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
				r._p0 = p0;
				r._p1 = p1;
				r._p2 = p2;
				r._p3 = p3;
				_strand->post(boost::bind(&boost_coro::run_one, shared_from_this()));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::_trig_handler<T0, T1, T2, T3>, shared_from_this(), boost::ref(r), p0, p1, p2, p3));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void _trig_handler(ref_ex<T0, T1, T2, T3>& r, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		if (!_quited)
		{
			r._p0 = p0;
			r._p1 = p1;
			r._p2 = p2;
			r._p3 = p3;
			pull_yield();
		}
	}

	bool async_trig_push(async_trig_base& th, int tm);

	void async_trig_timeout(async_trig_base& th);

	void async_trig_post_yield(async_trig_base& th);

	void async_trig_pull_yield(async_trig_base& th);

	void _async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<>& th);
	void _async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<> >& th);

	template <typename T0>
	void async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<T0>& th, const T0& p0)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th._notify)
			{
				th._temp._res0 = p0;
				async_trig_post_yield(th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler<T0>, shared_from_this(), isClosed, boost::ref(th), p0));
		}
	}

	template <typename T0>
	void async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<T0> >& th, const T0& p0)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th->_notify)
			{
				th->_temp._res0 = p0;
				async_trig_post_yield(*th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler_ptr<T0>, shared_from_this(), isClosed, th, p0));
		}
	}

	template <typename T0>
	void _async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<T0>& th, const T0& p0)
	{
		assert(_strand->running_in_this_thread());
		if (!_quited && !(*isClosed) && !th._notify)
		{
			th._temp._res0 = p0;
			async_trig_pull_yield(th);
		}
	}

	template <typename T0>
	void _async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<T0> >& th, const T0& p0)
	{
		return _async_trig_handler(isClosed, *th, p0);
	}

	template <typename T0, typename T1>
	void async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<T0, T1>& th, const T0& p0, const T1& p1)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th._notify)
			{
				th._temp._res0 = p0;
				th._temp._res1 = p1;
				async_trig_post_yield(th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler<T0, T1>, shared_from_this(), isClosed, boost::ref(th), p0, p1));
		}
	}

	template <typename T0, typename T1>
	void async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<T0, T1> >& th, const T0& p0, const T1& p1)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th->_notify)
			{
				th->_temp._res0 = p0;
				th->_temp._res1 = p1;
				async_trig_post_yield(*th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler_ptr<T0, T1>, shared_from_this(), isClosed, th, p0, p1));
		}
	}

	template <typename T0, typename T1>
	void _async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<T0, T1>& th, const T0& p0, const T1& p1)
	{
		assert(_strand->running_in_this_thread());
		if (!_quited && !(*isClosed) && !th._notify)
		{
			th._temp._res0 = p0;
			th._temp._res1 = p1;
			async_trig_pull_yield(th);
		}
	}

	template <typename T0, typename T1>
	void _async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<T0, T1> >& th, const T0& p0, const T1& p1)
	{
		return _async_trig_handler(isClosed, *th, p0, p1);
	}

	template <typename T0, typename T1, typename T2>
	void async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<T0, T1, T2>& th, const T0& p0, const T1& p1, const T2& p2)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th._notify)
			{
				th._temp._res0 = p0;
				th._temp._res1 = p1;
				th._temp._res2 = p2;
				async_trig_post_yield(th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler<T0, T1, T2>, shared_from_this(), isClosed, boost::ref(th), 
				p0, p1, p2));
		}
	}

	template <typename T0, typename T1, typename T2>
	void async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<T0, T1, T2> >& th, const T0& p0, const T1& p1, const T2& p2)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th->_notify)
			{
				th->_temp._res0 = p0;
				th->_temp._res1 = p1;
				th->_temp._res2 = p2;
				async_trig_post_yield(*th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler_ptr<T0, T1, T2>, shared_from_this(), isClosed, th, 
				p0, p1, p2));
		}
	}

	template <typename T0, typename T1, typename T2>
	void _async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<T0, T1, T2>& th, const T0& p0, const T1& p1, const T2& p2)
	{
		assert(_strand->running_in_this_thread());
		if (!_quited && !(*isClosed) && !th._notify)
		{
			th._temp._res0 = p0;
			th._temp._res1 = p1;
			th._temp._res2 = p2;
			async_trig_pull_yield(th);
		}
	}

	template <typename T0, typename T1, typename T2>
	void _async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<T0, T1, T2> >& th, const T0& p0, const T1& p1, const T2& p2)
	{
		return _async_trig_handler(isClosed, *th, p0, p1, p2);
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<T0, T1, T2, T3>& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th._notify)
			{
				th._temp._res0 = p0;
				th._temp._res1 = p1;
				th._temp._res2 = p2;
				th._temp._res3 = p3;
				async_trig_post_yield(th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler<T0, T1, T2, T3>, shared_from_this(), isClosed, boost::ref(th), 
				p0, p1, p2, p3));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> >& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed) && !th->_notify)
			{
				th->_temp._res0 = p0;
				th->_temp._res1 = p1;
				th->_temp._res2 = p2;
				th->_temp._res3 = p3;
				async_trig_post_yield(*th);
			}
		}
		else
		{
			_strand->post(boost::bind(&boost_coro::_async_trig_handler_ptr<T0, T1, T2, T3>, shared_from_this(), isClosed, th, 
				p0, p1, p2, p3));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void _async_trig_handler(boost::shared_ptr<bool> isClosed, async_trig_handle<T0, T1, T2, T3>& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		assert(_strand->running_in_this_thread());
		if (!_quited && !(*isClosed) && !th._notify)
		{
			th._temp._res0 = p0;
			th._temp._res1 = p1;
			th._temp._res2 = p2;
			th._temp._res3 = p3;
			async_trig_pull_yield(th);
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void _async_trig_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<async_trig_handle<T0, T1, T2, T3> >& th, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		_async_trig_handler(isClosed, *th, p0, p1, p2, p3);
	}

	void create_coro_handler(coro_handle coro, coro_handle& retCoro, list<coro_handle>::iterator& ch);
private:
	void check_run1(boost::shared_ptr<bool> isClosed, coro_msg_handle<>& cmh);
	void check_run1_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<coro_msg_handle<> >& cmh);

	template <typename T0>
	void notify_handler(boost::shared_ptr<bool> isClosed, param_list<msg_param<T0> >& cmh, const T0& p0)
	{
		typedef msg_param<T0> msg_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed))
			{
				check_run3<msg_type, msg_type::const_ref_type>(cmh, msg_type::const_ref_type(p0));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::check_run2<msg_type>, 
				shared_from_this(), isClosed, boost::ref(cmh), msg_type(p0)));
		}
	}

	template <typename T0>
	void notify_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<param_list<msg_param<T0> > >& cmh, const T0& p0)
	{
		typedef msg_param<T0> msg_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed))
			{
				check_run3<msg_type, msg_type::const_ref_type>(*cmh, msg_type::const_ref_type(p0));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::check_run2_ptr<msg_type>, 
				shared_from_this(), isClosed, cmh, msg_type(p0)));
		}
	}

	template <typename T0, typename T1>
	void notify_handler(boost::shared_ptr<bool> isClosed, param_list<msg_param<T0, T1> >& cmh, const T0& p0, const T1& p1)
	{
		typedef msg_param<T0, T1> msg_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed))
			{
				check_run3<msg_type, msg_type::const_ref_type>(cmh, msg_type::const_ref_type(p0, p1));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::check_run2<msg_type>, 
				shared_from_this(), isClosed, boost::ref(cmh), msg_type(p0, p1)));
		}
	}

	template <typename T0, typename T1>
	void notify_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<param_list<msg_param<T0, T1> > >& cmh, const T0& p0, const T1& p1)
	{
		typedef msg_param<T0, T1> msg_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed))
			{
				check_run3<msg_type, msg_type::const_ref_type>(*cmh, msg_type::const_ref_type(p0, p1));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::check_run2_ptr<msg_type>, 
				shared_from_this(), isClosed, cmh, msg_type(p0, p1)));
		}
	}

	template <typename T0, typename T1, typename T2>
	void notify_handler(boost::shared_ptr<bool> isClosed, param_list<msg_param<T0, T1, T2> >& cmh, const T0& p0, const T1& p1, const T2& p2)
	{
		typedef msg_param<T0, T1, T2> msg_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed))
			{
				check_run3<msg_type, msg_type::const_ref_type>(cmh, msg_type::const_ref_type(p0, p1, p2));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::check_run2<msg_type>, 
				shared_from_this(), isClosed, boost::ref(cmh), msg_type(p0, p1, p2)));
		}
	}

	template <typename T0, typename T1, typename T2>
	void notify_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<param_list<msg_param<T0, T1, T2> > >& cmh, const T0& p0, const T1& p1, const T2& p2)
	{
		typedef msg_param<T0, T1, T2> msg_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed))
			{
				check_run3<msg_type, msg_type::const_ref_type>(*cmh, msg_type::const_ref_type(p0, p1, p2));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::check_run2_ptr<msg_type>, 
				shared_from_this(), isClosed, cmh, msg_type(p0, p1, p2)));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void notify_handler(boost::shared_ptr<bool> isClosed, param_list<msg_param<T0, T1, T2, T3> >& cmh, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		typedef msg_param<T0, T1, T2, T3> msg_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed))
			{
				check_run3<msg_type, msg_type::const_ref_type>(cmh, msg_type::const_ref_type(p0, p1, p2, p3));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::check_run2<msg_type>, 
				shared_from_this(), isClosed, boost::ref(cmh), msg_type(p0, p1, p2, p3)));
		}
	}

	template <typename T0, typename T1, typename T2, typename T3>
	void notify_handler_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<param_list<msg_param<T0, T1, T2, T3> > >& cmh, const T0& p0, const T1& p1, const T2& p2, const T3& p3)
	{
		typedef msg_param<T0, T1, T2, T3> msg_type;

		if (_strand->running_in_this_thread())
		{
			if (!_quited && !(*isClosed))
			{
				check_run3<msg_type, msg_type::const_ref_type>(*cmh, msg_type::const_ref_type(p0, p1, p2, p3));
			}
		} 
		else
		{
			_strand->post(boost::bind(&boost_coro::check_run2_ptr<msg_type>, 
				shared_from_this(), isClosed, cmh, msg_type(p0, p1, p2, p3)));
		}
	}

	template <typename T /*msg_param*/>
	void check_run2(boost::shared_ptr<bool> isClosed, param_list<T>& cmh, const T& src)
	{
		if (!_quited && !(*isClosed))
		{
			if (cmh._waiting)
			{
				cmh._waiting = false;
				if (cmh._hasTm)
				{
					cmh._hasTm = false;
					_timerSleep->cancel();
				}
				assert(cmh.get_size() == 0);
				cmh.set_param(src);
				pull_yield();//与check_run3区别
			} 
			else
			{
				cmh.push_back(src);
			}
		}
	}

	template <typename T /*msg_param*/>
	void check_run2_ptr(boost::shared_ptr<bool> isClosed, boost::shared_ptr<param_list<T> >& cmh, const T& src)
	{
		check_run2(isClosed, *cmh, src);
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
				_timerSleep->cancel();
			}
			assert(cmh.get_size() == 0);
			cmh.set_param(srcRef);
			_strand->post(boost::bind(&boost_coro::run_one, shared_from_this()));//与check_run2区别
		} 
		else
		{
			cmh.push_back(srcRef);
		}
	}
public:
	/*!
	@brief 测试当前下的协程栈是否安全
	*/
	void check_stack();

	/*!
	@brief 获取当前协程剩余安全栈空间
	*/
	size_t stack_free_space();

	/*!
	@brief 获取当前协程调度器
	*/
	shared_strand this_strand();

	/*!
	@brief 返回本对象的智能指针
	*/
	coro_handle shared_from_this();

	/*!
	@brief 获取当前协程ID号
	*/
	long long this_id();

	/*!
	@brief 获取当前协程定时器
	*/
	boost::shared_ptr<timeout_trig> this_timer();

	/*!
	@brief 获取协程切换计数
	*/
	size_t yield_count();

	/*!
	@brief 协程切换计数清零
	*/
	void reset_yield();

	/*!
	@brief 开始运行建立好的协程
	*/
	void notify_start_run();

	/*!
	@brief 强制退出该协程，不可滥用，有可能会造成资源泄漏
	*/
	void notify_force_quit();

	/*!
	@brief 强制退出该协程，完成后回调
	*/
	void notify_force_quit(const boost::function<void (bool)>& h);

	/*!
	@brief 暂停协程
	*/
	void notify_suspend();
	void notify_suspend(const boost::function<void ()>& h);

	/*!
	@brief 恢复已暂停协程
	*/
	void notify_resume();
	void notify_resume(const boost::function<void ()>& h);

	/*!
	@brief 切换挂起/非挂起状态
	*/
	void switch_pause_play();
	void switch_pause_play(const boost::function<void (bool isPaused)>& h);

	/*!
	@brief 等待协程退出，在协程所依赖的ios无关线程中使用
	*/
	bool outside_wait_quit();

	/*!
	@brief 添加一个协程结束回调
	*/
	void append_quit_callback(const boost::function<void (bool)>& h);

	/*!
	@brief 启动一堆协程
	*/
	void another_coros_start_run(const list<coro_handle>& anotherCoros);

	/*!
	@brief 强制退出另一个协程，并且等待完成
	*/
	__yield_interrupt bool another_coro_force_quit(coro_handle anotherCoro);
	__yield_interrupt void another_coros_force_quit(const list<coro_handle>& anotherCoros);

	/*!
	@brief 等待另一个协程结束后返回
	*/
	__yield_interrupt bool another_coro_wait_quit(coro_handle anotherCoro);
	__yield_interrupt void another_coros_wait_quit(const list<coro_handle>& anotherCoros);

	/*!
	@brief 挂起另一个协程，等待其所有子协程都调用后才返回
	*/
	__yield_interrupt void another_coro_suspend(coro_handle anotherCoro);
	__yield_interrupt void another_coros_suspend(const list<coro_handle>& anotherCoros);

	/*!
	@brief 恢复另一个协程，等待其所有子协程都调用后才返回
	*/
	__yield_interrupt void another_coro_resume(coro_handle anotherCoro);
	__yield_interrupt void another_coros_resume(const list<coro_handle>& anotherCoros);

	/*!
	@brief 对另一个协程进行挂起/恢复状态切换
	@return 都已挂起返回true，否则false
	*/
	__yield_interrupt bool another_coro_switch(coro_handle anotherCoro);
	__yield_interrupt bool another_coros_switch(const list<coro_handle>& anotherCoros);
private:
	void assert_enter();
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
	void* _coroPull;///<协程中断点恢复
	void* _coroPush;///<协程中断点
	void* _stackTop;///<协程栈顶
	long long _coroID;///<协程ID
	size_t _stackSize;///<协程栈大小
	shared_strand _strand;///<协程调度器
	bool _inCoro;///<当前正在协程内部执行标记
	bool _started;///<已经开始运行的标记
	bool _quited;///<已经准备退出标记
	bool _suspended;///<协程挂起标记
	bool _hasNotify;///<当前协程挂起，有外部触发准备进入协程标记
	bool _isForce;///<是否是强制退出的标记，成功调用了force_quit
	size_t _yieldCount;//yield计数
	size_t _childOverCount;///<子协程退出时计数
	size_t _childSuspendResumeCount;///<子协程挂起/恢复计数
	boost::weak_ptr<boost_coro> _parentCoro;///<父协程
	boost::shared_ptr<timeout_trig> _timerSleep;///<提供延时功能
	main_func _mainFunc;///<协程入口
	list<suspend_resume_option> _suspendResumeQueue;///<挂起/恢复操作队列
	list<coro_handle> _childCoroList;///<子协程集合
	list<boost::function<void (bool)> > _exitCallback;///<协程结束后的回调函数，强制退出返回false，正常退出返回true
	list<boost::function<void ()> > _quitHandlerList;///<协程退出时强制调用的函数，后注册的先执行
	boost::shared_ptr<void> _sharedPoolMem;
	boost::weak_ptr<boost_coro> _weakThis;
};

#endif