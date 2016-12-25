#include "io_engine.h"
#include "mem_pool.h"
#include "my_actor.h"
#include "generator.h"
#include "context_yield.h"
#include "waitable_timer.h"

struct safe_stack_info
{
	wrap_local_handler_face<void()>* handler = NULL;
	context_yield::context_info* ctx = NULL;
};

tls_space* io_engine::_tls = NULL;
#if (defined DISABLE_BOOST_TIMER) && (defined ENABLE_GLOBAL_TIMER)
WaitableTimer_* io_engine::_waitableTimer = NULL;
#endif

void io_engine::install()
{
	if (!_tls)
	{
		_tls = new tls_space;
#if (defined DISABLE_BOOST_TIMER) && (defined ENABLE_GLOBAL_TIMER)
		_waitableTimer = new WaitableTimer_();
#endif
	}
}

void io_engine::uninstall()
{
	delete _tls;
	_tls = NULL;
#if (defined DISABLE_BOOST_TIMER) && (defined ENABLE_GLOBAL_TIMER)
	delete _waitableTimer;
	_waitableTimer = NULL;
#endif
}

io_engine::io_engine(bool enableTimer, const char* title)
{
	_opend = false;
	_runLock = NULL;
	_title = title ? title : "io_engine";
#ifdef WIN32
	_priority = normal;
#elif __linux__
	_priority = idle;
	_policy = sched_other;
#endif
	_strandPool = create_shared_pool_mt<boost_strand, std::mutex>(2 * run_thread::cpu_thread_number(), [](void* p)
	{
		new(p)boost_strand();
	}, [](boost_strand* p)->bool
	{
		if (!p->is_running())
		{
			p->~boost_strand();
			return true;
		}
		return false;
	});
#ifdef DISABLE_BOOST_TIMER
#ifndef ENABLE_GLOBAL_TIMER
	_waitableTimer = enableTimer ? new WaitableTimer_() : NULL;
#endif
#endif
}

io_engine::~io_engine()
{
	assert(!_opend);
#ifdef DISABLE_BOOST_TIMER
#ifndef ENABLE_GLOBAL_TIMER
	delete _waitableTimer;
#endif
#endif
	delete _strandPool;
}

void io_engine::run(size_t threadNum, sched policy)
{
	assert(threadNum >= 1);
	std::lock_guard<std::mutex> lg(_runMutex);
	if (!_opend)
	{
		_opend = true;
		_runCount = 0;
		_runLock = new boost::asio::io_service::work(_ios);
		_handleList.resize(threadNum);
#ifdef __linux__
		_policy = policy;
#endif
		size_t rc = 0;
		std::shared_ptr<std::mutex> blockMutex = std::make_shared<std::mutex>();
		std::shared_ptr<std::condition_variable> blockConVar = std::make_shared<std::condition_variable>();
		std::unique_lock<std::mutex> ul(*blockMutex);
		for (size_t i = 0; i < threadNum; i++)
		{
			run_thread* newThread = new run_thread([&, threadNum, i]
			{
				try
				{
					{
						run_thread::set_current_thread_name(_title.c_str());
#ifdef WIN32
						SetThreadPriority(GetCurrentThread(), _priority);
						DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &_handleList[i], 0, FALSE, DUPLICATE_SAME_ACCESS);
#elif __linux__
						pthread_attr_init(&_handleList[i]);
						pthread_attr_setschedpolicy(&_handleList[i], _policy);
						if (0 == i)
						{
							struct sched_param pm;
							int rs = pthread_attr_getschedparam(&_handleList[i], &pm);
							_priority = (priority)pm.sched_priority;
						}
#endif
						auto lockMutex = blockMutex;
						auto lockConVar = blockConVar;
						std::unique_lock<std::mutex> ul(*lockMutex);
						if (threadNum == ++rc)
						{
							lockConVar->notify_all();
						}
						else
						{
							lockConVar->wait(ul);
						}
					}
					context_yield::convert_thread_to_fiber();
					__space_align void* tlsBuff[64] = { 0 };
					_tls->set_space(tlsBuff);
					my_actor::tls_init(threadNum);
					generator::tls_init(threadNum);
					safe_stack_info safeStack;
					setTlsValue(ACTOR_SAFE_STACK_INDEX, &safeStack);
					safeStack.ctx = context_yield::make_context(MAX_STACKSIZE, [](context_yield::context_info* ctx, void* param)
					{
						while (true)
						{
							context_yield::push_yield(ctx);
							safe_stack_info* const safeStack = (safe_stack_info*)param;
							CHECK_EXCEPTION(*safeStack->handler);
						}
					}, &safeStack);
#if (__linux__ && ENABLE_DUMP_STACK)
					__space_align char dumpStack[8 kB];
					my_actor::dump_segmentation_fault(dumpStack, sizeof(dumpStack));
#endif
					_runCount += _ios.run();
#if (__linux__ && ENABLE_DUMP_STACK)
					my_actor::undump_segmentation_fault();
#endif
					context_yield::delete_context(safeStack.ctx);
					generator::tls_uninit();
					my_actor::tls_uninit();
					_tls->set_space(NULL);
					context_yield::convert_fiber_to_thread();
				}
				catch (boost::exception&)
				{
					trace_line("\nerror: ", "boost::exception");
					exit(2);
				}
				catch (std::exception&)
				{
					trace_line("\nerror: ", "std::exception");
					exit(3);
				}
				catch (std::shared_ptr<std::string>& msg)
				{
					trace_line("\nerror: ", *msg);
					exit(4);
				}
				catch (...)
				{
					exit(-1);
				}
			});
			_threadsID.insert(newThread->get_id());
			_runThreads.push_back(newThread);
		}
		blockConVar->wait(ul);
	}
}

void io_engine::stop()
{
	std::lock_guard<std::mutex> lg(_runMutex);
	if (_opend)
	{
		assert(!runningInThisIos());
		delete _runLock;
		_runLock = NULL;
		while (!_runThreads.empty())
		{
			_runThreads.front()->join();
			delete _runThreads.front();
			_runThreads.pop_front();
		}
		_ios.reset();
		_threadsID.clear();
		_ctrlMutex.lock();
		for (auto& ele : _handleList)
		{
#ifdef WIN32
			CloseHandle(ele);
#elif __linux__
			pthread_attr_destroy(&ele);
#endif
		}
		_handleList.clear();
		_ctrlMutex.unlock();
		_opend = false;
	}
}

bool io_engine::runningInThisIos()
{
	assert(_opend);
	return _threadsID.find(run_thread::this_thread_id()) != _threadsID.end();
}

size_t io_engine::threadNumber()
{
	assert(_opend);
	return _threadsID.size();
}

void io_engine::switch_invoke(wrap_local_handler_face<void()>* handler)
{
	safe_stack_info* si = (safe_stack_info*)getTlsValue(ACTOR_SAFE_STACK_INDEX);
	assert(!si->handler);
	si->handler = handler;
	context_yield::pull_yield(si->ctx);
	si->handler = NULL;
}

void io_engine::runPriority(priority pri)
{
	std::lock_guard<std::mutex> lg(_ctrlMutex);
#ifdef WIN32
	_priority = pri;
	for (auto& ele : _handleList)
	{
		SetThreadPriority(ele, _priority);
	}
#elif __linux__
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

const std::set<run_thread::thread_id>& io_engine::threadsID()
{
	return _threadsID;
}

const std::string& io_engine::title()
{
	return _title;
}

io_engine::operator boost::asio::io_service&() const
{
	return (boost::asio::io_service&)_ios;
}

void io_engine::setTlsBuff(void** buf)
{
	_tls->set_space(buf);
}

void* io_engine::getTlsValue(int i)
{
	assert(i >= 0 && i < 64);
	return _tls->get_space()[i];
}

void*& io_engine::getTlsValueRef(int i)
{
	assert(i >= 0 && i < 64);
	return _tls->get_space()[i];
}

void io_engine::setTlsValue(int i, void* val)
{
	assert(i >= 0 && i < 64);
	_tls->get_space()[i] = val;
}

void* io_engine::swapTlsValue(int i, void* val)
{
	assert(i >= 0 && i < 64);
	void** sp = _tls->get_space();
	void* old = sp[i];
	sp[i] = val;
	return old;
}

void** io_engine::getTlsValueBuff()
{
	return _tls->get_space();
}