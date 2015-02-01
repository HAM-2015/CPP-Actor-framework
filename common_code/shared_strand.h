#ifndef __SHARED_STRAND_H
#define __SHARED_STRAND_H


#ifdef ENABLE_STRAND_IMPL_POOL
#include "strand_ex.h"
#else
//#define BOOST_ASIO_ENABLE_SEQUENTIAL_STRAND_ALLOCATION
//#define BOOST_ASIO_STRAND_IMPLEMENTATIONS	11//单个ios中strand最大数目（超过会重叠）
#include <boost/asio/strand.hpp>
#endif //end ENABLE_SHARE_STRAND_IMPL

#include <boost/asio/io_service.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "ios_proxy.h"
#include "wrapped_post_handler.h"
#include "wrapped_dispatch_handler.h"

class boost_strand;
typedef boost::shared_ptr<boost_strand> shared_strand;

/*!
@brief 重新定义dispatch实现，所有不同strand中以消息方式进行函数调用
*/
#ifdef ENABLE_STRAND_IMPL_POOL
class boost_strand: public strand_ex
{
private:
	boost_strand(ios_proxy& iosProxy)
		: strand_ex(iosProxy), _iosProxy(iosProxy)

#else //defined ENABLE_SHARE_STRAND_IMPL

class boost_strand: public boost::asio::strand
{
private:
	boost_strand(ios_proxy& iosProxy)
		: boost::asio::strand(iosProxy), _iosProxy(iosProxy)

#endif //end ENABLE_SHARE_STRAND_IMPL

	{

	}
public:
	static shared_strand create(ios_proxy& iosProxy)
	{
		return shared_strand(new boost_strand(iosProxy));
	}
public:
	/*!
	@brief 重定义原dispatch实现，如果在本strand中调用则直接执行，否则添加到队列中等待执行
	@param handler 被调用函数
	*/
	template <typename Handler>
	void dispatch(const Handler& handler)
	{
		if (running_in_this_thread())
		{
			handler();
		} 
		else
		{
			post(handler);
		}
	}

	/*!
	@brief 把被调用函数包装到dispatch中，用于不同strand间消息传递
	*/
	template <typename Handler>
	wrapped_dispatch_handler<boost_strand, Handler> wrap(const Handler& handler)
	{
		return wrapped_dispatch_handler<boost_strand, Handler>(this, handler);
	}
	
	/*!
	@brief 把被调用函数包装到post中
	*/
	template <typename Handler>
	wrapped_post_handler<boost_strand, Handler> wrap_post(const Handler& handler)
	{
		return wrapped_post_handler<boost_strand, Handler>(this, handler);
	}

	/*!
	@brief 复制一个依赖同一个ios的strand
	*/
	shared_strand clone()
	{
		return create(_iosProxy);
	}

	/*!
	@brief 检查是否在所依赖ios线程中执行
	@return true 在, false 不在
	*/
	bool in_this_ios()
	{
		return _iosProxy.runningInThisIos();
	}

	/*!
	@brief 依赖的ios调度器运行线程数
	*/
	size_t ios_thread_number()
	{
		return _iosProxy.threadNumber();
	}

	/*!
	@brief 获取当前调度器代理
	*/
	ios_proxy& get_ios_proxy()
	{
		return _iosProxy;
	}
private:
	ios_proxy& _iosProxy;
public:
	/*!
	@brief 在一个strand中调用某个函数，直到这个函数被执行完成后才返回
	@warning 此函数有可能使整个程序陷入死锁，只能在与strand所依赖的ios无关线程中调用
	*/
	static void syncInvoke(shared_strand strand, const boost::function<void ()>& h)
	{
		assert(!strand->in_this_ios());
		boost::mutex mutex;
		boost::condition_variable con;
		boost::unique_lock<boost::mutex> ul(mutex);
		strand->post(boost::bind(&syncInvoke_proxy, boost::ref(h), boost::ref(mutex), boost::ref(con)));
		con.wait(ul);
	}

	/*!
	@brief 同上，带返回值
	*/
	template <class R>
	static R syncInvoke(shared_strand strand, const boost::function<R ()>& h)
	{
		assert(!strand->in_this_ios());
		R r;
		boost::mutex mutex;
		boost::condition_variable con;
		boost::unique_lock<boost::mutex> ul(mutex);
		strand->post(boost::bind(&syncInvoke_proxy_ret<R>, boost::ref(r), boost::ref(h), boost::ref(mutex), boost::ref(con)));
		con.wait(ul);
		return r;
	}

	/*!
	@brief 在strand中调用某个带返回值函数，然后通过一个回调函数把返回值传出
	@param cb 传出参数的函数
	*/
	template <class R>
	static void asyncInvoke(shared_strand strand, const boost::function<R ()>& h, const boost::function<void (R)>& cb)
	{
		strand->post(boost::bind(&asyncInvoke_proxy_ret<R>, h, cb));
	}

	static void asyncInvokeVoid(shared_strand strand, const boost::function<void ()>& h, const boost::function<void ()>& cb)
	{
		strand->post(boost::bind(&asyncInvoke_proxy_ret_void, h, cb));
	}
private:
	static void syncInvoke_proxy(const boost::function<void ()>& h, boost::mutex& mutex, boost::condition_variable& con)
	{
		h();
		mutex.lock();
		con.notify_one();
		mutex.unlock();
	}

	template <class R>
	static void syncInvoke_proxy_ret(R& r, const boost::function<R ()>& h, boost::mutex& mutex, boost::condition_variable& con)
	{
		r = h();
		mutex.lock();
		con.notify_one();
		mutex.unlock();
	}

	template <class R>
	static void asyncInvoke_proxy_ret(const boost::function<R ()>& h, const boost::function<void (R)>& cb)
	{
		cb(h());
	}

	static void asyncInvoke_proxy_ret_void(const boost::function<void ()>& h, const boost::function<void ()>& cb)
	{
		h();
		cb();
	}
};

#endif