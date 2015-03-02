#include "ios_proxy.h"
#include "shared_data.h"
#include "strand_ex.h"
#include <boost/bind.hpp>

ios_proxy::ios_proxy()
{
	_opend = false;
	_runLock = NULL;
	_implCount = 0;
	_runCount = 0;
	_priority = normal;
}

ios_proxy::~ios_proxy()
{
	assert(!_opend);
	boost::lock_guard<boost::mutex> lg(_implMutex);
	assert(_implPool.size() == _implCount);
	for (auto it = _implPool.begin(); it != _implPool.end(); it++)
	{
		delete (boost::asio::detail::strand_service::strand_impl*)*it;
	}
}

void ios_proxy::run(size_t threadNum)
{
	assert(threadNum >= 1);
	boost::lock_guard<boost::mutex> lg(_runMutex);
	if (!_opend)
	{
		_opend = true;
		_runCount = 0;
		_runLock = new boost::asio::io_service::work(_ios);
		_handleList.resize(threadNum);
		size_t rc = 0;
		boost::shared_ptr<boost::mutex> blockMutex(new boost::mutex);
		boost::shared_ptr<boost::condition_variable> blockConVar(new boost::condition_variable);
		boost::unique_lock<boost::mutex> ul(*blockMutex);
		for (size_t i = 0; i < threadNum; i++)
		{
			auto newThread = new boost::thread(boost::bind(&ios_proxy::runThread, this, &rc, (int)i, 
				(boost::weak_ptr<boost::mutex>)blockMutex, (boost::weak_ptr<boost::condition_variable>)blockConVar));
			_threadIDs.insert(newThread->get_id());
			_runThreads.add_thread(newThread);
		}
		blockConVar->wait(ul);
	}
}

void ios_proxy::stop()
{
	assert(!runningInThisIos());
	boost::lock_guard<boost::mutex> lg(_runMutex);
	if (_opend)
	{
		delete _runLock;
		_runLock = NULL;
		_runThreads.join_all();
		_ios.reset();
		_threadIDs.clear();
		_ctrlMutex.lock();
		_handleList.clear();
		_ctrlMutex.unlock();
		_opend = false;
	}
}

void ios_proxy::suspend()
{
	boost::lock_guard<boost::mutex> lg(_ctrlMutex);
	for (auto it = _handleList.begin(); it != _handleList.end(); it++)
	{
		SuspendThread(*it);
	}
}

void ios_proxy::resume()
{
	boost::lock_guard<boost::mutex> lg(_ctrlMutex);
	for (auto it = _handleList.begin(); it != _handleList.end(); it++)
	{
		ResumeThread(*it);
	}
}

bool ios_proxy::runningInThisIos()
{
	assert(_opend);
	return _threadIDs.find(boost::this_thread::get_id()) != _threadIDs.end();
}

size_t ios_proxy::threadNumber()
{
	assert(_opend);
	return _threadIDs.size();
}

void ios_proxy::runPriority(priority pri)
{
	boost::lock_guard<boost::mutex> lg(_ctrlMutex);
	_priority = pri;
	for (auto it = _handleList.begin(); it != _handleList.end(); it++)
	{
		SetThreadPriority(*it, _priority);
	}
}

ios_proxy::priority ios_proxy::getPriority()
{
	return _priority;
}

long long ios_proxy::getRunCount()
{
	return _runCount;
}

unsigned ios_proxy::physicalConcurrency()
{
	return boost::thread::physical_concurrency();
}

unsigned ios_proxy::hardwareConcurrency()
{
	return boost::thread::hardware_concurrency();
}

void ios_proxy::cpuAffinity(unsigned mask)
{
	boost::lock_guard<boost::mutex> lg(_ctrlMutex);
	for (auto it = _handleList.begin(); it != _handleList.end(); it++)
	{
		SetThreadAffinityMask(*it, mask);
	}
}

ios_proxy::operator boost::asio::io_service&() const
{
	return (boost::asio::io_service&)_ios;
}

void ios_proxy::runThread(size_t* rc, int id, boost::weak_ptr<boost::mutex> mutex, boost::weak_ptr<boost::condition_variable> conVar)
{
	try
	{
		{
			SetThreadPriority(GetCurrentThread(), _priority);
			DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &_handleList[id], 0, FALSE, DUPLICATE_SAME_ACCESS);
			auto blockMutex = mutex.lock();
			auto blockConVar = conVar.lock();
			boost::unique_lock<boost::mutex> ul(*blockMutex);
			if (_handleList.size() == ++(*rc))
			{
				blockConVar->notify_all();
			} 
			else
			{
				blockConVar->wait(ul);
			}
		}
		size_t r = _ios.run();
		_ctrlMutex.lock();
		_runCount += r;
		_ctrlMutex.unlock();
	}
	catch (msg_data::pool_memory_exception&)
	{
		MessageBoxA(NULL, "内存不足", NULL, NULL);
		ExitProcess(1);
	}
	catch (boost::exception&)
	{
		MessageBoxA(NULL, "未处理的BOOST异常", NULL, NULL);
		ExitProcess(2);
	}
	catch (std::exception&)
	{
		MessageBoxA(NULL, "未处理的STD异常", NULL, NULL);
		ExitProcess(3);
	}
	catch (boost::shared_ptr<std::string> msg)
	{
		MessageBoxA(NULL, msg->c_str(), NULL, NULL);
		ExitProcess(4);
	}
	catch (...)
	{
		MessageBoxA(NULL, "未知异常", NULL, NULL);
		ExitProcess(-1);
	}
}

void* ios_proxy::getImpl()
{
	_implMutex.lock();
	if (_implPool.empty())
	{
		assert(_implCount < 256);
		_implCount++;
		_implMutex.unlock();
		return new boost::asio::detail::strand_service::strand_impl;
	}
	void* res = _implPool.front();
	_implPool.pop_front();
	_implMutex.unlock();
	return res;
}

void ios_proxy::freeImpl(void* impl)
{
	boost::lock_guard<boost::mutex> lg(_implMutex);
	_implPool.push_back(impl);
}