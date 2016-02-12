#ifndef __BIND_QT_RUN_H
#define __BIND_QT_RUN_H

#ifdef ENABLE_QT_UI
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <functional>
#include <mutex>
#include <QtGui/qevent.h>
#include <QtCore/qeventloop.h>
#include "wrapped_post_handler.h"
#include "actor_framework.h"
#include "qt_strand.h"
#include "msg_queue.h"

#define QT_POST_TASK	(QEvent::MaxUser-1)
#define	QT_UI_ACTOR_STACK_SIZE	(128 kB - STACK_RESERVED_SPACE_SIZE)

//开始在Actor中，嵌入一段在qt-ui线程中执行的连续逻辑
#define begin_RUN_IN_QT_UI_AT(__this_ui__, __host__) {(__this_ui__)->send(__host__, [&]() {
#define begin_RUN_IN_QT_UI() begin_RUN_IN_QT_UI_AT(this, self)
//结束在qt-ui线程中执行的一段连续逻辑，只有当这段逻辑执行完毕后才会执行END后续代码
#define end_RUN_IN_QT_UI() });}
//////////////////////////////////////////////////////////////////////////
//在Actor中，嵌入一段在qt-ui线程中执行的语句
#define RUN_IN_QT_UI_AT(__this_ui__, __host__, __exp__) {(__this_ui__)->send(__host__, [&]() {__exp__;});}
#define RUN_IN_QT_UI(__exp__) RUN_IN_QT_UI_AT(this, self, __exp__)
//////////////////////////////////////////////////////////////////////////
//在Actor中，嵌入一段在qt-ui线程中执行的Actor逻辑（当该逻辑中包含异步操作时使用，否则建议用begin_RUN_IN_QT_UI_AT）
#define begin_ACTOR_RUN_IN_QT_UI_AT(__this_ui__, __host__, __ios__) {\
	auto ___host = __host__; \
	my_actor::quit_guard ___qg(__host__); \
	auto ___tactor = (__this_ui__)->create_qt_actor(__ios__, [&](my_actor* __host__) {

//在Actor中，嵌入一段在qt-ui线程中执行的Actor逻辑（当该逻辑中包含异步操作时使用，否则建议用begin_RUN_IN_QT_UI）
#define begin_ACTOR_RUN_IN_QT_UI(__ios__) begin_ACTOR_RUN_IN_QT_UI_AT(this, self, __ios__)

//结束在qt-ui线程中执行的Actor，只有当Actor内逻辑执行完毕后才会执行END后续代码
#define end_ACTOR_RUN_IN_QT_UI()\
	}); \
	___tactor->notify_run(); \
	___host->actor_wait_quit(___tactor); \
}
//////////////////////////////////////////////////////////////////////////
#define __NO_DELETE_FRAME(__frame__)
#define __DELETE_FRAME(__frame__) delete (__frame__)
#define __DESTROY_FRAME(__frame__) (__frame__).destroy();

#define __CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __delete__) {\
	assert(!(__this_ui__)->run_in_ui_thread());\
	bool __inside_loop = true;\
	do\
	{\
		begin_RUN_IN_QT_UI_AT(__this_ui__, __host__);\
		if (!(__frame__)->is_wait_close())\
		{\
			(__frame__)->close();\
			__inside_loop = false;\
		}\
		else\
		{\
			__inside_loop = (__frame__)->inside_wait_close_loop();\
		}\
		if (!__inside_loop) { __delete__(__frame__); }\
		end_RUN_IN_QT_UI();\
	} while (__inside_loop);\
}

//在非UI线程Actor中，关闭一个qt-ui对象
#define CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__) __CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __NO_DELETE_FRAME)
#define CLOSE_QT_UI(__frame__) CLOSE_QT_UI_AT(this, self, __frame__)
#define CLOSE_QT_UI_DELETE_AT(__this_ui__, __host__, __frame__) __CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __DELETE_FRAME)
#define CLOSE_QT_UI_DELETE(__frame__) CLOSE_QT_UI_DELETE_AT(this, self, __frame__)
#define CLOSE_QT_UI_DESTROY_AT(__this_ui__, __host__, __frame__) __CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __DESTROY_FRAME)
#define CLOSE_QT_UI_DESTROY(__frame__) CLOSE_QT_UI_DESTROY_AT(this, self, __frame__)

#ifdef ENABLE_QT_ACTOR
#define __IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __delete__) {\
	assert((__this_ui__)->run_in_ui_thread());\
	assert(!(__frame__)->running_in_this_thread());\
	bool __check_loop = true;\
	if (!(__frame__)->is_wait_close())\
	{\
		begin_RUN_IN_THREAD_STACK(__host__);\
		if (!(__frame__)->is_wait_close())\
		{\
			__check_loop = false;\
			(__frame__)->close();\
		}\
		end_RUN_IN_THREAD_STACK();\
	}\
	if (__check_loop)\
	{\
		while ((__frame__)->inside_wait_close_loop())\
		{\
			(__host__)->yield();\
		}\
	}\
	__delete__(__frame__);\
}

//在UI线程Actor中，关闭一个qt-ui对象
#define IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__) __IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __NO_DELETE_FRAME)
#define IN_CLOSE_QT_UI(__frame__) IN_CLOSE_QT_UI_AT(this, self, __frame__)
#define IN_CLOSE_QT_UI_DELETE_AT(__this_ui__, __host__, __frame__) __IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __DELETE_FRAME)
#define IN_CLOSE_QT_UI_DELETE(__frame__) IN_CLOSE_QT_UI_DELETE_AT(this, self, __frame__)
#define IN_CLOSE_QT_UI_DESTROY_AT(__this_ui__, __host__, __frame__) __IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __DESTROY_FRAME)
#define IN_CLOSE_QT_UI_DESTROY(__frame__) IN_CLOSE_QT_UI_DESTROY_AT(this, self, __frame__)
#endif

//////////////////////////////////////////////////////////////////////////

//closeEvent函数中准备关闭ui
#define BEGIN_CLOSE_QT_UI() if (!is_wait_close() && !in_close_scope()) { set_in_close_scope_sign(true);
//closeEvent函数中等待关闭ui
#define WAIT_CLOSE_QT_UI() enter_wait_close();
//closeEvent函数中结束关闭ui
#define END_CLOSE_QT_UI() set_in_close_scope_sign(false);}

class bind_qt_run_base
{
protected:
#ifdef ENABLE_QT_ACTOR
	struct ui_tls
	{
		ui_tls();
		~ui_tls();

		static void push_stack(bind_qt_run_base*);
		static bind_qt_run_base* pop_stack();
		static bool running_in_this_thread(bind_qt_run_base*);
		static void init();
		static void reset();

		msg_list<bind_qt_run_base*, pool_alloc<mem_alloc2<> > > _uiStack;
		int _count;
		static tls_space _tls;
	};
#endif

	struct wrap_handler_face
	{
		virtual void invoke() = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_handler_face
	{
		template <typename H>
		wrap_handler(H&& h)
			:_handler(TRY_MOVE(h)) {}

		void invoke()
		{
			CHECK_EXCEPTION(_handler);
			this->~wrap_handler();
		}

		Handler _handler;
	};

	template <typename Handler>
	struct wrap_timed_handler : public wrap_handler_face
	{
		template <typename H>
		wrap_timed_handler(std::mutex& mutex, const shared_bool& deadSign, bool& running, H&& h)
			:_mutex(mutex), _deadSign(deadSign), _running(running), _handler(TRY_MOVE(h)) {}

		void invoke()
		{
			bool run = false;
			_mutex.lock();
			if (!*_deadSign)
			{
				_running = true;
				run = true;
			}
			_mutex.unlock();

			if (run)
			{
				CHECK_EXCEPTION(_handler);
			}
			this->~wrap_timed_handler();
		}

		std::mutex& _mutex;
		shared_bool _deadSign;
		bool& _running;
		Handler _handler;
	};

	template <typename Handler>
	wrap_handler_face* make_wrap_handler(reusable_mem_mt<>& reuMem, Handler&& handler)
	{
		typedef wrap_handler<RM_CREF(Handler)> handler_type;
		return new(reuMem.allocate(sizeof(handler_type)))handler_type(TRY_MOVE(handler));
	}

	template <typename Handler>
	wrap_handler_face* make_wrap_timed_handler(reusable_mem_mt<>& reuMem, std::mutex& mutex, const shared_bool& deadSign, bool& running, Handler&& handler)
	{
		typedef wrap_timed_handler<RM_CREF(Handler)> handler_type;
		return new(reuMem.allocate(sizeof(handler_type)))handler_type(mutex, deadSign, running, TRY_MOVE(handler));
	}

	struct task_event : public QEvent
	{
		template <typename... Args>
		task_event(wrap_handler_face* handler, Args&&... args)
			:QEvent(TRY_MOVE(args)...), _handler(handler) {}
		void* operator new(size_t s);
		void operator delete(void* p);
		wrap_handler_face* _handler;
		static mem_alloc_mt<task_event> _taskAlloc;
	};
protected:
	bind_qt_run_base();
	virtual ~bind_qt_run_base();
public:
	/*!
	@brief 获取主线程ID
	*/
	boost::thread::id thread_id();

	/*!
	@brief 是否在UI线程中执行
	*/
	bool run_in_ui_thread();

	/*!
	@brief 检测现在是否运行在本UI的post任务下
	*/
	bool running_in_this_thread();

	/*!
	@brief 是否在等待关闭
	*/
	bool is_wait_close();

	/*!
	@brief 等待关闭完成
	*/
	bool wait_close_reached();

	/*!
	@brief 正在关闭中
	*/
	bool inside_wait_close_loop();

	/*!
	@brief 在关闭操作范围内
	*/
	bool in_close_scope();

	/*!
	@brief 设置关闭操作范围标记
	*/
	void set_in_close_scope_sign(bool b);

	/*!
	@brief 发送一个执行函数到UI消息队列中执行
	*/
	template <typename Handler>
	void post(Handler&& handler)
	{
		post_task_event(make_wrap_handler(_reuMem, TRY_MOVE(handler)));
	}

	/*!
	@brief 发送一个执行函数到UI消息队列中执行，完成后返回
	*/
	template <typename Handler>
	void send(my_actor* host, Handler&& handler)
	{
		host->lock_quit();
		host->trig([&](trig_once_notifer<>&& cb)
		{
			post_task_event(make_wrap_handler(_reuMem, std::bind([&handler](const trig_once_notifer<>& cb)
			{
				handler();
				cb();
			}, std::move(cb))));
		});
		host->unlock_quit();
	}

	/*!
	@brief 发送一个超时执行函数到UI消息队列中执行，完成后返回
	*/
	template <typename Handler>
	bool timed_send(int tm, my_actor* host, Handler&& handler)
	{
		host->lock_quit();
		bool running = false;
		actor_trig_handle<> ath;
		actor_trig_notifer<> ntf = host->make_trig_notifer_to_self(ath);
		post_task_event(make_wrap_timed_handler(_reuMem, _mutex, ath.dead_sign(), running, [&handler, &ntf]
		{
			handler();
			ntf();
		}));

		if (!host->timed_wait_trig(tm, ath))
		{
			_mutex.lock();
			if (!running)
			{
				host->close_trig_notifer(ath);
				_mutex.unlock();
				host->unlock_quit();
				return false;
			}
			_mutex.unlock();
			host->wait_trig(ath);
		}
		host->unlock_quit();
		return true;
	}

	/*!
	@brief 绑定一个函数到UI队列执行
	*/
	template <typename Handler>
	wrapped_post_handler<bind_qt_run_base, Handler> wrap(Handler&& handler)
	{
		return wrapped_post_handler<bind_qt_run_base, RM_CREF(Handler)>(this, TRY_MOVE(handler));
	}

	template <typename Handler>
	std::function<void()> wrap_check_close(Handler&& handler)
	{
		assert(run_in_ui_thread());
		_waitCount++;
		return std::function<void()>(wrap(std::bind([this](const Handler& handler)
		{
			handler();
			check_close();
#ifdef _MSC_VER
		}, TRY_MOVE(handler))), reusable_alloc<void, reusable_mem_mt<>>(_reuMem));
#elif __GNUG__
		}, TRY_MOVE(handler))));
#endif
	}

	std::function<void()> wrap_check_close();
#ifdef ENABLE_QT_ACTOR
	/*!
	@brief 开启shared_qt_strand
	*/
	void start_qt_strand(io_engine& ios);

	/*!
	@brief 获取shared_qt_strand
	*/
	const shared_qt_strand& ui_strand();

	/*!
	@brief 在UI线程中创建一个Actor，先执行start_qt_strand
	*/
	actor_handle create_qt_actor(const my_actor::main_func& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE);
	actor_handle create_qt_actor(my_actor::main_func&& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE);

	/*!
	@brief 在UI线程中创建一个子Actor，先执行start_qt_strand
	*/
	child_actor_handle create_qt_child_actor(my_actor* host, const my_actor::main_func& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE);
	child_actor_handle create_qt_child_actor(my_actor* host, my_actor::main_func&& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE);
#endif
protected:
	virtual void post_task_event(wrap_handler_face*) = 0;
	virtual void enter_loop() = 0;
	virtual void close_now() = 0;
	void run_one_task(wrap_handler_face* h);
	void check_close();
	void enter_wait_close();
private:
	boost::thread::id _threadID;
	reusable_mem_mt<> _reuMem;
	std::mutex _mutex;
#ifdef ENABLE_QT_ACTOR
	shared_qt_strand _qtStrand;
#endif;
protected:
	DEBUG_OPERATION(std::atomic<size_t> _taskCount);
	QEventLoop* _eventLoop;
	int _waitCount;
private:
	bool _waitClose;
	bool _inCloseScope;
};

template <typename FRAME>
class bind_qt_run : public FRAME, private bind_qt_run_base
{
protected:
	template <typename... Args>
	bind_qt_run(Args&&... args)
		: FRAME(TRY_MOVE(args)...) {}
public:
	boost::thread::id thread_id()
	{
		return bind_qt_run_base::thread_id();
	}

	bool run_in_ui_thread()
	{
		return bind_qt_run_base::run_in_ui_thread();
	}

	bool running_in_this_thread()
	{
		return bind_qt_run_base::running_in_this_thread();
	}

	template <typename Handler>
	void post(Handler&& handler)
	{
		bind_qt_run_base::post(TRY_MOVE(handler));
	}

	template <typename Handler>
	void send(my_actor* host, Handler&& handler)
	{
		bind_qt_run_base::send(host, TRY_MOVE(handler));
	}

	template <typename Handler>
	bool timed_send(int tm, my_actor* host, Handler&& handler)
	{
		return bind_qt_run_base::timed_send(tm, host, TRY_MOVE(handler));
	}

	template <typename Handler>
	wrapped_post_handler<bind_qt_run_base, Handler> wrap(Handler&& handler)
	{
		return bind_qt_run_base::wrap(TRY_MOVE(handler));
	}

	template <typename Handler>
	std::function<void()> wrap_check_close(Handler&& handler)
	{
		return bind_qt_run_base::wrap_check_close(TRY_MOVE(handler));
	}

	std::function<void()> wrap_check_close()
	{
		return bind_qt_run_base::wrap_check_close();
	}
public:
	void enter_wait_close()
	{
		bind_qt_run_base::enter_wait_close();
	}

	bool is_wait_close()
	{
		return bind_qt_run_base::is_wait_close();
	}

	bool wait_close_reached()
	{
		return bind_qt_run_base::wait_close_reached();
	}

	bool inside_wait_close_loop()
	{
		return bind_qt_run_base::inside_wait_close_loop();
	}

	bool in_close_scope()
	{
		return bind_qt_run_base::in_close_scope();
	}

	void set_in_close_scope_sign(bool b)
	{
		bind_qt_run_base::set_in_close_scope_sign(b);
	}
#ifdef ENABLE_QT_ACTOR
	void start_qt_strand(io_engine& ios)
	{
		bind_qt_run_base::start_qt_strand(ios);
	}

	const shared_qt_strand& ui_strand()
	{
		return bind_qt_run_base::ui_strand();
	}

	actor_handle create_qt_actor(const my_actor::main_func& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE)
	{
		return bind_qt_run_base::create_qt_actor(mainFunc, stackSize);
	}

	actor_handle create_qt_actor(my_actor::main_func&& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE)
	{
		return bind_qt_run_base::create_qt_actor(std::move(mainFunc), stackSize);
	}

	child_actor_handle create_qt_child_actor(my_actor* host, const my_actor::main_func& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE)
	{
		return bind_qt_run_base::create_qt_child_actor(host, mainFunc, stackSize);
	}

	child_actor_handle create_qt_child_actor(my_actor* host, my_actor::main_func&& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE)
	{
		return bind_qt_run_base::create_qt_child_actor(host, std::move(mainFunc), stackSize);
	}
#endif
private:
	void post_task_event(wrap_handler_face* h) override final
	{
		DEBUG_OPERATION(_taskCount++);
		QCoreApplication::postEvent(this, new task_event(h, QEvent::Type(QT_POST_TASK)));
	}

	void customEvent(QEvent* e) override final
	{
		if (e->type() == QEvent::Type(QT_POST_TASK))
		{
			assert(dynamic_cast<task_event*>(e));
			DEBUG_OPERATION(_taskCount--);
			bind_qt_run_base::run_one_task(static_cast<task_event*>(e)->_handler);
		}
		else
		{
			customEvent_(e);
		}
	}

	void enter_loop() override final
	{
		QEventLoop eventLoop(this);
		_eventLoop = &eventLoop;
		eventLoop.exec();
		_eventLoop = NULL;
	}

	void close_now() override final
	{
		assert(_eventLoop);
		_eventLoop->exit();
	}
protected:
	virtual void customEvent_(QEvent*)
	{

	}
};
//////////////////////////////////////////////////////////////////////////

struct ui_sync_closed_exception {};

struct ui_sync_lost_exeption{};

/*!
@brief ui非阻塞同步消息返回值
*/
template <typename R = void>
struct qt_ui_sync_result
{
#if (_DEBUG || DEBUG)
	qt_ui_sync_result(bind_qt_run_base* runBase, QEventLoop& eventLoop)
	:_runBase(runBase), _eventLoop(eventLoop) {}
#else
	qt_ui_sync_result(QEventLoop& eventLoop)
	:_eventLoop(eventLoop) {}
#endif

	template <typename... Args>
	void return_(Args&&... args)
	{
		assert(_runBase->running_in_this_thread());
		assert(!has());
		_res.create(TRY_MOVE(args)...);
		_eventLoop.exit();
	}

	bool has()
	{
		return _res.has();
	}

	R& result()
	{
		return _res.get();
	}
private:
	DEBUG_OPERATION(bind_qt_run_base* _runBase);
	QEventLoop& _eventLoop;
	stack_obj<R> _res;
};

template <>
struct qt_ui_sync_result<void>
{
#if (_DEBUG || DEBUG)
	qt_ui_sync_result(bind_qt_run_base* runBase, QEventLoop& eventLoop)
	:_runBase(runBase), _eventLoop(eventLoop), _has(false) {}
#else
	qt_ui_sync_result(QEventLoop& eventLoop)
	:_eventLoop(eventLoop), _has(false) {}
#endif

	void return_()
	{
		assert(_runBase->running_in_this_thread());
		assert(!has());
		_has = true;
		_eventLoop.exit();
	}

	bool has()
	{
		return _has;
	}
private:
	DEBUG_OPERATION(bind_qt_run_base* _runBase);
	QEventLoop& _eventLoop;
	bool _has;
};

/*!
@brief ui非阻塞同步消息发送器，接连多个发送时，先发送的后返回
*/
template <typename R, typename... ARGS>
struct qt_ui_sync_notifer;

template <typename R, typename... ARGS>
struct qt_ui_sync_notifer<R(ARGS...)>
{
	template <typename Parent>
	explicit qt_ui_sync_notifer(Parent* parent)
#if (_DEBUG || DEBUG)
		: _runBase((bind_qt_run_base*)parent), _notifyCount(0), _parent(parent) {}
#else
		: _parent(parent) {}
#endif

	~qt_ui_sync_notifer()
	{
	}

	template <typename... Args>
	R operator()(Args&&... args) const
	{
		assert(_runBase->run_in_ui_thread());
		assert(!empty());
		DEBUG_OPERATION(_notifyCount++);
		QEventLoop eventLoop(_parent);
#if (_DEBUG || DEBUG)
		qt_ui_sync_result<R> res(_runBase, eventLoop);
#else
		qt_ui_sync_result<R> res(eventLoop);
#endif
		_handler(&res, TRY_MOVE(args)...);
		eventLoop.exec();
		DEBUG_OPERATION(_notifyCount--);
		if (!res.has())
		{
			throw ui_sync_closed_exception();
		}
		return std::move(res.result());
	}

	template <typename H>
	void set_notifer(H&& h)
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		_handler = TRY_MOVE(h);
	}

	void reset()
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		clear_function(_handler);
	}

	bool empty() const
	{
		assert(_runBase->run_in_ui_thread());
		return !_handler;
	}
private:
	DEBUG_OPERATION(bind_qt_run_base* _runBase);
	DEBUG_OPERATION(mutable size_t _notifyCount);
	QObject* _parent;
	std::function<void(qt_ui_sync_result<R>*, ARGS...)> _handler;
};

template <typename... ARGS>
struct qt_ui_sync_notifer<void(ARGS...)>
{
	template <typename Parent>
	explicit qt_ui_sync_notifer(Parent* parent)
#if (_DEBUG || DEBUG)
		: _runBase((bind_qt_run_base*)parent), _notifyCount(0), _parent(parent) {}
#else
		: _parent(parent) {}
#endif

	~qt_ui_sync_notifer()
	{
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		assert(_runBase->run_in_ui_thread());
		assert(!empty());
		DEBUG_OPERATION(_notifyCount++);
		QEventLoop eventLoop(_parent);
#if (_DEBUG || DEBUG)
		qt_ui_sync_result<void> res(_runBase, eventLoop);
#else
		qt_ui_sync_result<void> res(eventLoop);
#endif
		_handler(&res, TRY_MOVE(args)...);
		eventLoop.exec();
		DEBUG_OPERATION(_notifyCount--);
		if (!res.has())
		{
			throw ui_sync_closed_exception();
		}
	}

	template <typename H>
	void set_notifer(H&& h)
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		_handler = TRY_MOVE(h);
	}

	void reset()
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		clear_function(_handler);
	}

	bool empty() const
	{
		assert(_runBase->run_in_ui_thread());
		return !_handler;
	}
private:
	DEBUG_OPERATION(bind_qt_run_base* _runBase);
	DEBUG_OPERATION(mutable size_t _notifyCount);
	QObject* _parent;
	std::function<void(qt_ui_sync_result<void>*, ARGS...)> _handler;
};

//////////////////////////////////////////////////////////////////////////

template <typename R>
struct check_lost_qt_ui_sync_result;

template <typename R = void>
struct _check_lost_qt_ui_sync_result
{
	friend check_lost_qt_ui_sync_result<R>;

	_check_lost_qt_ui_sync_result(bind_qt_run_base* runBase, QEventLoop& eventLoop)
	:_runBase(runBase), _eventLoop(eventLoop) {}

	template <typename... Args>
	void return_(Args&&... args)
	{
		assert(_runBase->running_in_this_thread());
		assert(!has());
		_res.create(TRY_MOVE(args)...);
		_eventLoop.exit();
	}

	bool has()
	{
		return _res.has();
	}

	R& result()
	{
		return _res.get();
	}
private:
	bind_qt_run_base* _runBase;
	QEventLoop& _eventLoop;
	stack_obj<R> _res;
};

template <>
struct _check_lost_qt_ui_sync_result<void>
{
	friend check_lost_qt_ui_sync_result<void>;

	_check_lost_qt_ui_sync_result(bind_qt_run_base* runBase, QEventLoop& eventLoop)
	:_runBase(runBase), _eventLoop(eventLoop), _has(false) {}

	void return_()
	{
		assert(_runBase->running_in_this_thread());
		assert(!has());
		_has = true;
		_eventLoop.exit();
	}

	bool has()
	{
		return _has;
	}
private:
	bind_qt_run_base* _runBase;
	QEventLoop& _eventLoop;
	bool _has;
};

/*!
@brief 检测ui消息是否丢失未处理
*/
template <typename R>
struct check_lost_qt_ui_sync_result
{
	check_lost_qt_ui_sync_result(_check_lost_qt_ui_sync_result<R>* res, const shared_bool& b, const std::shared_ptr<std::mutex>& m)
	:_res(res), _lostSign(b), _mutex(m) {}

	check_lost_qt_ui_sync_result(check_lost_qt_ui_sync_result&& s)
		:_res(s._res), _lostSign(std::move(s._lostSign)), _mutex(std::move(s._mutex))
	{
		s._res = NULL;
	}

	~check_lost_qt_ui_sync_result()
	{
		if (_res)
		{
			std::lock_guard<std::mutex> lg(*_mutex);
			if (!*_lostSign)
			{
				assert(!_res->has());
				if (_res->_runBase->running_in_this_thread())
				{
					_res->_eventLoop.exit(-1);
				}
				else
				{
					_res->_runBase->post(std::bind([](_check_lost_qt_ui_sync_result<R>* res,
						const shared_bool& lostSign, const std::shared_ptr<std::mutex>& mutex)
					{
						std::lock_guard<std::mutex> lg(*mutex);
						if (!*lostSign)
						{
							res->_eventLoop.exit(-1);
						}
					}, _res, std::move(_lostSign), std::move(_mutex)));
				}
			}
		}
	}

	template <typename... Args>
	void return_(Args&&... args)
	{
		assert(_res);
		_res->return_(TRY_MOVE(args)...);
		_res = NULL;
	}
private:
	check_lost_qt_ui_sync_result(const check_lost_qt_ui_sync_result&) {}
	void operator=(const check_lost_qt_ui_sync_result&) {}
	void operator=(check_lost_qt_ui_sync_result&&) {}
private:
	_check_lost_qt_ui_sync_result<R>* _res;
	shared_bool _lostSign;
	std::shared_ptr<std::mutex> _mutex;
};

/*!
@brief ui非阻塞同步消息发送器(带丢失检测)，接连多个发送时，先发送的后返回
*/
template <typename R, typename... ARGS>
struct qt_ui_sync_check_lost_notifer;

template <typename R, typename... ARGS>
struct qt_ui_sync_check_lost_notifer<R(ARGS...)>
{
	template <typename Parent>
	explicit qt_ui_sync_check_lost_notifer(Parent* parent)
		: _runBase((bind_qt_run_base*)parent), _parent(parent), _mutex(std::shared_ptr<std::mutex>(new std::mutex))
	{
		DEBUG_OPERATION(_notifyCount = 0);
	}

	~qt_ui_sync_check_lost_notifer()
	{
	}

	template <typename... Args>
	R operator()(Args&&... args) const
	{
		assert(_runBase->run_in_ui_thread());
		assert(!empty());
		DEBUG_OPERATION(_notifyCount++);
		QEventLoop eventLoop(_parent);
		shared_bool sign = shared_bool::new_(false);
		_check_lost_qt_ui_sync_result<R> res(_runBase, eventLoop);
		_handler(check_lost_qt_ui_sync_result<R>(&res, sign, _mutex), TRY_MOVE(args)...);
		int rcd = eventLoop.exec();
		_mutex->lock();
		*sign = true;
		_mutex->unlock();
		DEBUG_OPERATION(_notifyCount--);
		if (-1 == rcd)
		{
			assert(!res.has());
			throw ui_sync_lost_exeption();
		}
		if (!res.has())
		{
			throw ui_sync_closed_exception();
		}
		return std::move(res.result());
	}

	template <typename H>
	void set_notifer(H&& h)
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		_handler = TRY_MOVE(h);
	}

	void reset()
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		clear_function(_handler);
	}

	bool empty() const
	{
		assert(_runBase->run_in_ui_thread());
		return !_handler;
	}
private:
	DEBUG_OPERATION(mutable size_t _notifyCount);
	QObject* _parent;
	bind_qt_run_base* _runBase;
	std::shared_ptr<std::mutex> _mutex;
	std::function<void(check_lost_qt_ui_sync_result<R>, ARGS...)> _handler;
};

template <typename... ARGS>
struct qt_ui_sync_check_lost_notifer<void(ARGS...)>
{
	template <typename Parent>
	explicit qt_ui_sync_check_lost_notifer(Parent* parent)
		: _runBase((bind_qt_run_base*)parent), _parent(parent), _mutex(std::shared_ptr<std::mutex>(new std::mutex))
	{
		DEBUG_OPERATION(_notifyCount = 0);
	}

	~qt_ui_sync_check_lost_notifer()
	{
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		assert(_runBase->run_in_ui_thread());
		assert(!empty());
		DEBUG_OPERATION(_notifyCount++);
		QEventLoop eventLoop(_parent);
		shared_bool sign = shared_bool::new_(false);
		_check_lost_qt_ui_sync_result<void> res(_runBase, eventLoop);
		_handler(check_lost_qt_ui_sync_result<void>(&res, sign, _mutex), TRY_MOVE(args)...);
		int rcd = eventLoop.exec();
		_mutex->lock();
		*sign = true;
		_mutex->unlock();
		DEBUG_OPERATION(_notifyCount--);
		if (-1 == rcd)
		{
			assert(!res.has());
			throw ui_sync_lost_exeption();
		}
		if (!res.has())
		{
			throw ui_sync_closed_exception();
		}
	}

	template <typename H>
	void set_notifer(H&& h)
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		_handler = TRY_MOVE(h);
	}

	void reset()
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		clear_function(_handler);
	}

	bool empty() const
	{
		assert(_runBase->run_in_ui_thread());
		return !_handler;
	}
private:
	DEBUG_OPERATION(mutable size_t _notifyCount);
	QObject* _parent;
	bind_qt_run_base* _runBase;
	std::shared_ptr<std::mutex> _mutex;
	std::function<void(check_lost_qt_ui_sync_result<void>, ARGS...)> _handler;
};
#endif

#endif