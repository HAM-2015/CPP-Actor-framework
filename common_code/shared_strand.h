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
class boost_strand
{
#ifdef ENABLE_STRAND_IMPL_POOL
	typedef strand_ex strand_type;
#else
	typedef boost::asio::strand strand_type;
#endif
protected:
	boost_strand();
public:
	virtual ~boost_strand();
	static shared_strand create(ios_proxy& iosProxy);
public:
	/*!
	@brief 如果在本strand中调用则直接执行，否则添加到队列中等待执行
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
	@brief 添加一个任务到strand队列
	*/
	template <typename Handler>
	void post(const Handler& handler)
	{
#ifndef ENABLE_MFC_ACTOR
		_strand->post(handler);
#else
		if (_strand)
		{
			_strand->post(handler);
		}
		else
		{
			_post(handler);
		}
#endif
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
	virtual shared_strand clone();

	/*!
	@brief 检查是否在所依赖ios线程中执行
	@return true 在, false 不在
	*/
	virtual bool in_this_ios();

	/*!
	@brief 依赖的ios调度器运行线程数
	*/
	size_t ios_thread_number();

	/*!
	@brief 判断是否在本strand中运行
	*/
	virtual bool running_in_this_thread();

	/*!
	@brief 获取当前调度器代理
	*/
	ios_proxy& get_ios_proxy();

	/*!
	@brief 获取当前调度器
	*/
	boost::asio::io_service& get_io_service();

#ifdef ENABLE_MFC_ACTOR
	virtual void _post(const boost::function<void ()>& h);
#endif
protected:
	ios_proxy* _iosProxy;
	strand_type* _strand;
public:
	/*!
	@brief 在一个strand中调用某个函数，直到这个函数被执行完成后才返回
	@warning 此函数有可能使整个程序陷入死锁，只能在与strand所依赖的ios无关线程中调用
	*/
	static void syncInvoke(shared_strand strand, const boost::function<void ()>& h);

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

	static void asyncInvokeVoid(shared_strand strand, const boost::function<void ()>& h, const boost::function<void ()>& cb);
private:
	static void syncInvoke_proxy(const boost::function<void ()>& h, boost::mutex& mutex, boost::condition_variable& con);

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

	static void asyncInvoke_proxy_ret_void(const boost::function<void ()>& h, const boost::function<void ()>& cb);
};

#endif