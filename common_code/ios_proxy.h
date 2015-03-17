#ifndef __IOS_PROXY_H
#define __IOS_PROXY_H

#include <boost/asio/io_service.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/thread.hpp>
#include <set>
#include <list>
#include <vector>

class strand_ex;

/*!
@brief io_service调度器封装
*/
class ios_proxy
{
	friend strand_ex;
public:
	enum priority
	{
		above_normal = THREAD_PRIORITY_ABOVE_NORMAL,
		below_normal = THREAD_PRIORITY_BELOW_NORMAL,
		highest = THREAD_PRIORITY_HIGHEST,
		idle = THREAD_PRIORITY_IDLE,
		lowest = THREAD_PRIORITY_LOWEST,
		normal = THREAD_PRIORITY_NORMAL,
		time_critical = THREAD_PRIORITY_TIME_CRITICAL
	};
public:
	ios_proxy();
	~ios_proxy();
public:
	/*!
	@brief 开始运行调度器，非阻塞，启动完毕后立即返回
	@param threadNum 调度器并发线程数
	*/
	void run(size_t threadNum = 1);

	/*!
	@brief 等待调度器中无任务时返回
	*/
	void stop();

	/*!
	@brief 挂起调度器所有线程
	*/
	void suspend();

	/*!
	@brief 恢复调度器所有线程
	*/
	void resume();

	/*!
	@brief 检测当前函数是否在本调度器中执行
	*/
	bool runningInThisIos();

	/*!
	@brief 调度器线程数
	*/
	size_t threadNumber();

	/*!
	@brief 调度器线程优先级设置
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
	@brief 设置CPU关联
	*/
	void cpuAffinity(unsigned mask);

	/*!
	@brief 调度器对象引用
	*/
	operator boost::asio::io_service& () const;
private:
	void* getImpl();
	void freeImpl(void* impl);
private:
	bool _opend;
	size_t _implCount;
	priority _priority;
	std::set<boost::thread::id> _threadIDs;
	std::list<void*> _implPool;
	std::vector<HANDLE> _handleList;
	boost::atomic<long long> _runCount;
	boost::mutex _ctrlMutex;
	boost::mutex _runMutex;
	boost::mutex _implMutex;
	boost::asio::io_service _ios;
	boost::asio::io_service::work* _runLock;
	boost::thread_group _runThreads;
};

#endif