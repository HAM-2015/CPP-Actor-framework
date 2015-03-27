#include "ios_proxy.h"
#include "shared_data.h"
#include "strand_ex.h"
#include <memory>

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
		std::shared_ptr<boost::mutex> blockMutex(new boost::mutex);
		std::shared_ptr<boost::condition_variable> blockConVar(new boost::condition_variable);
		std::weak_ptr<boost::mutex> weakMutex = blockMutex;
		std::weak_ptr<boost::condition_variable> weakConVar = blockConVar;
		boost::unique_lock<boost::mutex> ul(*blockMutex);
		for (size_t i = 0; i < threadNum; i++)
		{
			boost::thread* newThread = new boost::thread([&, i]()
			{
				try
				{
					{
						SetThreadPriority(GetCurrentThread(), _priority);
						DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &_handleList[i], 0, FALSE, DUPLICATE_SAME_ACCESS);
						auto blockMutex = weakMutex.lock();
						auto blockConVar = weakConVar.lock();
						boost::unique_lock<boost::mutex> ul(*blockMutex);
						if (threadNum == ++rc)
						{
							blockConVar->notify_all();
						}
						else
						{
							blockConVar->wait(ul);
						}
					}
					_runCount += _ios.run();
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
				catch (std::shared_ptr<std::string> msg)
				{
					MessageBoxA(NULL, msg->c_str(), NULL, NULL);
					ExitProcess(4);
				}
				catch (...)
				{
					MessageBoxA(NULL, "未知异常", NULL, NULL);
					ExitProcess(-1);
				}
			});
			_threadIDs.insert(newThread->get_id());
			_runThreads.add_thread(newThread);
		}
		blockConVar->wait(ul);
	}
}

void ios_proxy::stop()
{
	boost::lock_guard<boost::mutex> lg(_runMutex);
	if (_opend)
	{
		assert(!runningInThisIos());
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

void* ios_proxy::getImpl()
{
	_implMutex.lock();
	if (_implPool.empty())
	{
		assert(_implCount < 4096);
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