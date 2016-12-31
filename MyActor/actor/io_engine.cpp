#include "io_engine.h"
#include "mem_pool.h"
#include "my_actor.h"
#include "generator.h"
#include "context_yield.h"
#include "waitable_timer.h"

#ifdef ASIO_HANDLER_ALLOCATE_EX

#define ASIO_HANDLER_SPACE_SIZE (sizeof(void*)*16)
typedef MemTlsNode_<char[ASIO_HANDLER_SPACE_SIZE]> handler_alloc1;
typedef MemTlsNode_<char[ASIO_HANDLER_SPACE_SIZE * 2]> handler_alloc2;
typedef MemTlsNode_<char[ASIO_HANDLER_SPACE_SIZE * 3]> handler_alloc3;
typedef MemTlsNode_<char[ASIO_HANDLER_SPACE_SIZE * 4]> handler_alloc4;
typedef ReuMemTls_ handler_reu_alloc;

namespace boost
{
	namespace asio
	{
		void* asio_handler_allocate_ex(std::size_t size)
		{
			void* pointer = NULL;
			void*** tls = (void***)io_engine::getTlsValueBuff();
			if (tls && tls[ASIO_HANDLER_ALLOC_EX_INDEX])
			{
				void** const alloc = tls[ASIO_HANDLER_ALLOC_EX_INDEX];
				switch (MEM_ALIGN(size + 1, ASIO_HANDLER_SPACE_SIZE) / ASIO_HANDLER_SPACE_SIZE)
				{
				case 1: pointer = !as_ptype<handler_alloc1>(alloc[0])->overflow() ? as_ptype<handler_alloc1>(alloc[0])->allocate() : NULL; break;
				case 2: pointer = !as_ptype<handler_alloc2>(alloc[1])->overflow() ? as_ptype<handler_alloc2>(alloc[1])->allocate() : NULL; break;
				case 3: pointer = !as_ptype<handler_alloc3>(alloc[2])->overflow() ? as_ptype<handler_alloc3>(alloc[2])->allocate() : NULL; break;
				case 4: pointer = !as_ptype<handler_alloc4>(alloc[3])->overflow() ? as_ptype<handler_alloc4>(alloc[3])->allocate() : NULL; break;
				}
				if (pointer)
				{
					as_ptype<char>(pointer)[size] = 1;
				}
				else
				{
					pointer = as_ptype<handler_reu_alloc>(alloc[4])->allocate(size + 1);
					as_ptype<char>(pointer)[size] = 2;
				}
			}
			else
			{
				pointer = malloc(size + 1);
				as_ptype<char>(pointer)[size] = 0;
			}
			return pointer;
		}

		void asio_handler_deallocate_ex(void* pointer, std::size_t size)
		{
			const char type = as_ptype<char>(pointer)[size];
			if (type)
			{
				void*** tls = (void***)io_engine::getTlsValueBuff();
				assert(tls && tls[ASIO_HANDLER_ALLOC_EX_INDEX]);
				void** const alloc = tls[ASIO_HANDLER_ALLOC_EX_INDEX];
				if (1 == type)
				{
					switch (MEM_ALIGN(size + 1, ASIO_HANDLER_SPACE_SIZE) / ASIO_HANDLER_SPACE_SIZE)
					{
					case 1: as_ptype<handler_alloc1>(alloc[0])->deallocate(pointer); break;
					case 2: as_ptype<handler_alloc2>(alloc[1])->deallocate(pointer); break;
					case 3: as_ptype<handler_alloc3>(alloc[2])->deallocate(pointer); break;
					case 4: as_ptype<handler_alloc4>(alloc[3])->deallocate(pointer); break;
					default: assert(false); break;
					}
				}
				else
				{
					assert(2 == type);
					as_ptype<handler_reu_alloc>(alloc[4])->deallocate(pointer, size + 1);
				}
			}
			else
			{
				free(pointer);
			}
		}
	}
}
#endif

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
:io_engine(MEM_POOL_LENGTH, enableTimer, title) {}

io_engine::io_engine(size_t poolSize, bool enableTimer, const char* title)
{
	_opend = false;
	_runLock = NULL;
	_poolSize = poolSize > 4 ? poolSize : 4;
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
		if (p->running_in_this_thread())
		{
			assert(p->is_running());
		}
		else if (!p->safe_is_running())
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
#ifdef ASIO_HANDLER_ALLOCATE_EX
					void* asioAll[5] =
					{
						new handler_alloc1(_poolSize),
						new handler_alloc2(_poolSize / 2),
						new handler_alloc3(_poolSize / 3),
						new handler_alloc4(_poolSize / 4),
						new handler_reu_alloc()
					};
					tlsBuff[ASIO_HANDLER_ALLOC_EX_INDEX] = asioAll;
#endif
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
#ifdef ASIO_HANDLER_ALLOCATE_EX
					delete (handler_alloc1*)asioAll[0];
					delete (handler_alloc2*)asioAll[1];
					delete (handler_alloc3*)asioAll[2];
					delete (handler_alloc4*)asioAll[3];
					delete (handler_reu_alloc*)asioAll[4];
#endif
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