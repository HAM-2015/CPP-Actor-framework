#ifndef __TIME_OUT_H
#define __TIME_OUT_H

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include "shared_strand.h"

class deadline_timer_trig;
class system_timer_trig;
class steady_timer_trig;
class high_resolution_timer_trig;
class super_resolution_timer_trig;
/*!
@brief 异步延时触发
*/
class timeout_trig: public boost::enable_shared_from_this<timeout_trig>
{
	friend class deadline_timer_trig;
	friend class system_timer_trig;
	friend class steady_timer_trig;
	friend class high_resolution_timer_trig;
	friend class super_resolution_timer_trig;
public:
	enum timer_type
	{
		deadline_timer,
		system_timer,
		steady_timer,
		high_resolution_timer,
		super_resolution_timer
	};
protected:
	timeout_trig(shared_strand st);
public:
	virtual ~timeout_trig();
public:
	/*!
	@brief 创建一个定时器
	*/
	static boost::shared_ptr<timeout_trig> create(shared_strand st, timer_type type = deadline_timer);

	/*!
	@brief 创建一个基于当前系统时间的定时器，更改系统时间会影响定时
	*/
	static boost::shared_ptr<timeout_trig> create_deadline(shared_strand st);

	/*!
	@brief 创建一个基于当前系统时间的定时器，更改系统时间会影响定时
	*/
	static boost::shared_ptr<timeout_trig> create_system(shared_strand st);

	/*!
	@brief 创建一个稳定定时器，不受系统时间影响
	*/
	static boost::shared_ptr<timeout_trig> create_steady(shared_strand st);

	/*!
	@brief 创建一个高精度定时器，不受系统时间影响
	*/
	static boost::shared_ptr<timeout_trig> create_high_resolution(shared_strand st);
	
	/*!
	@brief 创建一个超高精度定时器，不受系统时间影响，需要先启用query_performance_counter::enable()
	*/
	static boost::shared_ptr<timeout_trig> create_super_resolution(shared_strand st);

	/*!
	@brief 启用内存池模式下创建对象，不过所用内存是父对象给的
	@param sharedPoolMem 内存池对象（该内存区中可能不只一个对象），父对象内也保存一份副本，当所有保存sharedPoolMem副本的对象都析构后，自动回收内存
	@param objPtr 本次对象使用的地址
	*/
	static boost::shared_ptr<timeout_trig> create_in_pool(boost::shared_ptr<void> sharedPoolMem, void* objPtr, shared_strand st, timer_type tp);

	/*!
	@brief 获取启用某种timer类型后的对象大小
	*/
	static size_t object_size(timer_type tp);

	/*!
	@brief 启用高精度时钟
	*/
	static void enable_high_resolution();

	/*!
	@brief 程序优先权提升为“软实时”
	*/
	static void enable_realtime_priority();

	/*!
	@brief 获取系统时间戳(us)
	*/
	static long long get_tick();
public:
	/*!
	@brief 开始超时
	@param ms 毫秒数/微秒
	@param h 触发函数
	*/
	void timeOut(int ms, const boost::function<void ()>& h);
	
	/*!
	@brief 取消上次计时，重新开始超时
	@param ms 毫秒数/微秒
	@param h 触发函数
	*/
	void timeOutAgain(int ms, const boost::function<void ()>& h);

	/*!
	@brief 取消超时触发，取消后将不再调用回调函数
	*/
	void cancel();

	/*!
	@brief 挂起计时器，恢复后继续剩余计时
	*/
	void suspend();

	/*!
	@brief 恢复计时器
	*/
	void resume();

	/*!
	@brief 不计时，马上就触发，调用完毕后返回
	*/
	void trigger();

	/*!
	@brief 获取超时剩余时间
	*/
	int surplusTime();

	/*!
	@brief 开启微秒级
	*/
	void enableMicrosec();
protected:
	virtual void expiresTimer() = 0;
	virtual void cancelTimer() = 0;
	void timeoutHandler(int id, const boost::system::error_code& error);
protected:
	int _timeoutID;
	int _isMillisec;
	bool _completed;
	bool _suspend;
	shared_strand _strand;
	boost::posix_time::microsec _time;
	boost::posix_time::ptime _stampBegin;
	boost::posix_time::ptime _stampEnd;
	boost::function<void ()> _h;
};

#endif