#ifndef __IO_ENGINE_H
#define __IO_ENGINE_H

#include <algorithm>
#include <boost/asio/io_service.hpp>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <set>
#include <vector>
#include "scattered.h"
#include "strand_ex.h"
#include "mem_pool.h"
#include "run_thread.h"

class my_actor;
class boost_strand;
#ifdef DISABLE_BOOST_TIMER
class WaitableTimer_;
class WaitableTimerEvent_;
#endif

class io_engine
{
	friend boost_strand;
#ifdef DISABLE_BOOST_TIMER
	friend WaitableTimerEvent_;
#endif
public:
#ifdef WIN32
	enum priority
	{
		time_critical = THREAD_PRIORITY_TIME_CRITICAL,
		highest = THREAD_PRIORITY_HIGHEST,
		above_normal = THREAD_PRIORITY_ABOVE_NORMAL,
		normal = THREAD_PRIORITY_NORMAL,
		below_normal = THREAD_PRIORITY_BELOW_NORMAL,
		lowest = THREAD_PRIORITY_LOWEST,
		idle = THREAD_PRIORITY_IDLE
	};

	enum sched
	{
		sched_fifo = 0,
		sched_rr = 0,
		sched_other = 0
	};
#elif __linux__
	enum priority
	{
		time_critical = 99,
		highest = 83,
		above_normal = 66,
		normal = 50,
		below_normal = 33,
		lowest = 16,
		idle = 0
	};

	enum sched
	{
		sched_fifo = SCHED_FIFO,
		sched_rr = SCHED_RR,
		sched_other = SCHED_OTHER
	};
#endif
public:
	io_engine(bool enableTimer = true, const char* title = NULL);
	~io_engine();
public:
	/*!
	@brief 开始运行调度器，非阻塞，启动完毕后立即返回
	@param threadNum 调度器并发线程数
	@param policy 线程调度策略(linux下有效，win下忽略)
	*/
	void run(size_t threadNum = 1, sched policy = sched_other);

	/*!
	@brief 等待调度器中无任务时返回
	*/
	void stop();

	/*!
	@brief 改变调度线程数，之前的线程将退出
	*/
	void changeThreadNumber(size_t threadNum);

	/*!
	@brief 检测当前函数是否在本调度器中执行
	*/
	bool runningInThisIos();

	/*!
	@brief 调度器线程数
	*/
	size_t threadNumber();

	/*!
	@brief 挂起任务
	*/
	void suspend();

	/*!
	@brief 恢复任务
	*/
	void resume();

	/*!
	@brief 调度器线程优先级设置，（linux下 sched_fifo, sched_rr 有效 ）
	*/
	void runPriority(priority pri);

	/*!
	@brief 获取当前调度器优先级
	*/
	priority getPriority();

	/*!
	@brief 获取调度器本次从run()到stop()后的迭代次数
	*/
	long long getRunCount();

	/*!
	@brief 运行线程ID
	*/
	const std::set<run_thread::thread_id>& threadsID();

	/*!
	@brief ios title
	*/
	const std::string& title();

	/*!
	@brief 调度器对象引用
	*/
	operator boost::asio::io_service& () const;

	/*!
	@brief 在非ios线程中初始化一个tls空间
	*/
	static void setTlsBuff(void** buf);

	/*!
	@brief 获取tls值
	@param 0 <= i < 64
	*/
	static void* getTlsValue(int i);

	/*!
	@brief 获取tls值引用
	@param 0 <= i < 64
	*/
	static void*& getTlsValueRef(int i);

	/*!
	@brief 设置tls值
	@param 0 <= i < 64
	*/
	static void setTlsValue(int i, void* val);

	/*!
	@brief 交换tls值
	@param 0 <= i < 64
	*/
	static void* swapTlsValue(int i, void* val);

	/*!
	@brief 获取tls变量空间
	*/
	static void** getTlsValueBuff();
private:
	friend my_actor;
	static void install();
	static void uninstall();
private:
	void _run(size_t threadNum, sched policy);
	void _stop();
private:
	bool _opend;
	bool _suspend;
	shared_obj_pool<boost_strand>* _strandPool;
#ifdef DISABLE_BOOST_TIMER
#ifdef ENABLE_GLOBAL_TIMER
	static WaitableTimer_* _waitableTimer;
#else
	WaitableTimer_* _waitableTimer;
#endif
#endif
	priority _priority;
	std::string _title;
	std::mutex _runMutex;
	std::mutex _ctrlMutex;
	std::atomic<long long> _runCount;
	std::set<run_thread::thread_id> _threadsID;
	std::list<run_thread*> _runThreads;
	boost::asio::io_service _ios;
	boost::asio::io_service::work* _runLock;
#ifdef WIN32
	std::vector<HANDLE> _handleList;
#elif __linux__
	sched _policy;
	std::vector<pthread_attr_t> _handleList;
#endif
	static tls_space* _tls;
};

#endif