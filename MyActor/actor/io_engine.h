#ifndef __IO_ENGINE_H
#define __IO_ENGINE_H

#include <boost/asio/io_service.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/thread.hpp>
#include <set>
#include <vector>

class StrandEx_;
class ActorTimer_;

class io_engine
{
	friend StrandEx_;
	friend ActorTimer_;
public:
#ifdef _MSC_VER
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
#elif __GNUG__
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
	io_engine();
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
	@brief 检测当前函数是否在本调度器中执行
	*/
	bool runningInThisIos();

	/*!
	@brief 调度器线程数
	*/
	size_t threadNumber();

#ifdef _MSC_VER
	/*!
	@brief 挂起调度器所有线程（仅在win下有效）
	*/
	void suspend();

	/*!
	@brief 恢复调度器所有线程（仅在win下有效）
	*/
	void resume();
#endif

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
	@brief 获取CPU核心数
	*/
	static unsigned physicalConcurrency();

	/*!
	@brief 获取CPU线程数
	*/
	static unsigned hardwareConcurrency();

	/*!
	@brief 运行线程ID
	*/
	const std::set<boost::thread::id>& threadsID();

	/*!
	@brief 调度器对象引用
	*/
	operator boost::asio::io_service& () const;

	/*!
	@brief 获取tls值
	@param 0 <= i < 64
	*/
	static void* getTlsValue(int i);

	/*!
	@brief 设置tls值
	@param 0 <= i < 64
	*/
	static void setTlsValue(int i, void* val);

	/*!
	@brief 获取某个tls变量空间
	@param 0 <= i < 64
	*/
	static void** getTlsValuePtr(int i);
private:
	void* getImpl();
	void freeImpl(void* impl);
	void* getTimer();
	void freeTimer(void* timer);
private:
	bool _opend;
	void* _implPool;
	void* _timerPool;
	priority _priority;
	boost::mutex _runMutex;
	boost::mutex _ctrlMutex;
	boost::asio::io_service _ios;
	boost::thread_group _runThreads;
	boost::atomic<long long> _runCount;
	std::set<boost::thread::id> _threadsID;
	boost::asio::io_service::work* _runLock;
	static boost::thread_specific_ptr<void*> _tls;///<64位void*
#ifdef _MSC_VER
	std::vector<HANDLE> _handleList;
#elif __GNUG__
	sched _policy;
	std::vector<pthread_attr_t> _handleList;
#endif
};

#endif