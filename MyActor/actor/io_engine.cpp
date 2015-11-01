#include "io_engine.h"
#include "mem_pool.h"
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/detail/strand_service.hpp>
#include <memory>

typedef boost::asio::detail::strand_service::strand_impl impl_type;
typedef boost::asio::basic_waitable_timer<boost::chrono::high_resolution_clock> timer_type;

boost::thread_specific_ptr<void*> io_engine::_tls(NULL);

io_engine::io_engine()
{
	_opend = false;
	_runLock = NULL;
#ifdef _MSC_VER
	_priority = normal;
#elif __GNUG__
	_priority = idle;
	_policy = sched_other;
#endif
	_implPool = create_pool<impl_type>(256, [](void* p)
	{
		new(p)impl_type();
	});
	_timerPool = create_pool<timer_type>(64, [this](void* p)
	{
		new(p)timer_type(_ios);
	});
}

io_engine::~io_engine()
{
	assert(!_opend);
}

void io_engine::run(size_t threadNum, sched policy)
{
	assert(threadNum >= 1);
	boost::lock_guard<boost::mutex> lg(_runMutex);
	if (!_opend)
	{
		_opend = true;
		_runCount = 0;
		_runLock = new boost::asio::io_service::work(_ios);
		_handleList.resize(threadNum);
#ifdef __GNUG__
		_policy = policy;
#endif
		size_t rc = 0;
		std::shared_ptr<boost::mutex> blockMutex(new boost::mutex);
		std::shared_ptr<boost::condition_variable> blockConVar(new boost::condition_variable);
		boost::unique_lock<boost::mutex> ul(*blockMutex);
		for (size_t i = 0; i < threadNum; i++)
		{
			boost::thread* newThread = new boost::thread([&, i]
			{
				try
				{
					{
#ifdef _MSC_VER
						SetThreadPriority(GetCurrentThread(), _priority);
						DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &_handleList[i], 0, FALSE, DUPLICATE_SAME_ACCESS);
#elif __GNUG__
						pthread_attr_init(&_handleList[i]);
						pthread_attr_setschedpolicy (&_handleList[i], _policy);
#endif
						auto lockMutex = blockMutex;
						auto lockConVar = blockConVar;
						boost::unique_lock<boost::mutex> ul(*lockMutex);
						if (threadNum == ++rc)
						{
							lockConVar->notify_all();
						}
						else
						{
							lockConVar->wait(ul);
						}
					}
					void* tssBuff[64] = { 0 };
					_tls.reset(tssBuff);
					_runCount += _ios.run();
					_tls.release();
				}
				catch (boost::exception&)
				{
					exit(2);
				}
				catch (std::exception&)
				{
					exit(3);
				}
				catch (std::shared_ptr<std::string> msg)
				{
					exit(4);
				}
				catch (...)
				{
					exit(-1);
				}
			});
			_threadsID.insert(newThread->get_id());
			_runThreads.add_thread(newThread);
		}
		blockConVar->wait(ul);
#ifdef __GNUG__
		struct sched_param pm;
		int rs = pthread_attr_getschedparam(&_handleList[0], &pm);
		_priority = (priority)pm.__sched_priority;
#endif
	}
}

void io_engine::stop()
{
	boost::lock_guard<boost::mutex> lg(_runMutex);
	if (_opend)
	{
		assert(!runningInThisIos());
		delete _runLock;
		_runLock = NULL;
		_runThreads.join_all();
		_opend = false;
		_ios.reset();
		_threadsID.clear();
		_ctrlMutex.lock();
		for (auto& ele : _handleList)
		{
#ifdef _MSC_VER
			CloseHandle(ele);
#elif __GNUG__
			pthread_attr_destroy(&ele);
#endif
		}
		_handleList.clear();
		_ctrlMutex.unlock();
	}
}

bool io_engine::runningInThisIos()
{
	assert(_opend);
	return _threadsID.find(boost::this_thread::get_id()) != _threadsID.end();
}

size_t io_engine::threadNumber()
{
	assert(_opend);
	return _threadsID.size();
}

#ifdef _MSC_VER
void io_engine::suspend()
{
	boost::lock_guard<boost::mutex> lg(_ctrlMutex);
	for (auto& ele : _handleList)
	{
		SuspendThread(ele);
	}
}

void io_engine::resume()
{
	boost::lock_guard<boost::mutex> lg(_ctrlMutex);
	for (auto& ele : _handleList)
	{
		ResumeThread(ele);
	}
}
#endif

void io_engine::runPriority(priority pri)
{
	boost::lock_guard<boost::mutex> lg(_ctrlMutex);
#ifdef _MSC_VER
	_priority = pri;
	for (auto& ele : _handleList)
	{
		SetThreadPriority(ele, _priority);
	}
#elif __GNUG__
	if (sched_fifo == _policy || sched_rr == _policy)
	{
		_priority = pri;
		struct sched_param pm = { pri };
		for (auto& ele : _handleList)
		{
			pthread_attr_setschedparam(&ele, &pm);
		}
	}
#endif
}

io_engine::priority io_engine::getPriority()
{
	return _priority;
}

long long io_engine::getRunCount()
{
	return _runCount;
}

unsigned io_engine::physicalConcurrency()
{
	return boost::thread::physical_concurrency();
}

unsigned io_engine::hardwareConcurrency()
{
	return boost::thread::hardware_concurrency();
}

const std::set<boost::thread::id>& io_engine::threadsID()
{
	return _threadsID;
}

io_engine::operator boost::asio::io_service&() const
{
	return (boost::asio::io_service&)_ios;
}

void* io_engine::getImpl()
{
	return ((obj_pool<impl_type>*)_implPool)->pick();
}

void io_engine::freeImpl(void* impl)
{
	((obj_pool<impl_type>*)_implPool)->recycle(impl);
}

void* io_engine::getTimer()
{
	return ((obj_pool<timer_type>*)_timerPool)->pick();
}

void io_engine::freeTimer(void* timer)
{
	((obj_pool<timer_type>*)_timerPool)->recycle(timer);
}

void* io_engine::getTlsValue(int i)
{
	assert(i >= 0 && i < 64);
	return _tls.get()[i];
}

void io_engine::setTlsValue(int i, void* val)
{
	assert(i >= 0 && i < 64);
	_tls.get()[i] = val;
}

void** io_engine::getTlsValuePtr(int i)
{
	assert(i >= 0 && i < 64);
	return _tls.get() + i;
}