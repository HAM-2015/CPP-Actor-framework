#ifndef __BIND_QT_RUN_H
#define __BIND_QT_RUN_H

#ifdef ENABLE_QT_UI
#include <functional>
#include <mutex>
#include <QtGui/qevent.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qcoreapplication.h>
#include "wrapped_post_handler.h"
#include "my_actor.h"
#include "msg_queue.h"
#include "run_strand.h"
#include "run_thread.h"
#include "generator.h"

#define QT_POST_TASK	(QEvent::MaxUser-1)
#define	QT_UI_ACTOR_STACK_SIZE	(128 kB - STACK_RESERVED_SPACE_SIZE)

//开始在Actor中，嵌入一段在qt-ui线程中执行的连续逻辑
#define begin_RUN_IN_QT_UI_AT(__this_ui__, __host__) do {(__this_ui__)->send(__host__, [&]() {
#define begin_RUN_IN_QT_UI() begin_RUN_IN_QT_UI_AT(this, self)
//结束在qt-ui线程中执行的一段连续逻辑，只有当这段逻辑执行完毕后才会执行END后续代码
#define end_RUN_IN_QT_UI() });} while (false)
//////////////////////////////////////////////////////////////////////////
//在Actor中，嵌入一段在qt-ui线程中执行的语句
#define RUN_IN_QT_UI_AT(__this_ui__, __host__, ...)  do {(__this_ui__)->send(__host__, [&]{ option_pck(__VA_ARGS__) });} while (false)

//在Actor中，嵌入一段在qt-ui线程中执行的语句
#define RUN_IN_QT_UI(...) RUN_IN_QT_UI_AT(this, self, __VA_ARGS__)
//////////////////////////////////////////////////////////////////////////
//在Actor中，嵌入一段在qt-ui线程中执行的Actor逻辑（当该逻辑中包含异步操作时使用，否则建议用begin_RUN_IN_QT_UI_AT）
#define begin_ACTOR_RUN_IN_QT_UI_AT(__this_ui__, __host__, __ios__) do {\
	auto ___host = __host__; \
	my_actor::quit_guard ___qg(__host__); \
	auto ___tactor = (__this_ui__)->create_ui_actor(__ios__, [&](my_actor* __host__) {

//在Actor中，嵌入一段在qt-ui线程中执行的Actor逻辑（当该逻辑中包含异步操作时使用，否则建议用begin_RUN_IN_QT_UI）
#define begin_ACTOR_RUN_IN_QT_UI(__ios__) begin_ACTOR_RUN_IN_QT_UI_AT(this, self, __ios__)

//结束在qt-ui线程中执行的Actor，只有当Actor内逻辑执行完毕后才会执行END后续代码
#define end_ACTOR_RUN_IN_QT_UI()\
	}); \
	___tactor->run(); \
	___host->actor_wait_quit(___tactor); \
} while (false)
//////////////////////////////////////////////////////////////////////////
#define __NO_DELETE_FRAME(__frame__)
#define __DELETE_FRAME(__frame__) delete (__frame__)
#define __DESTROY_FRAME(__frame__) (__frame__).destroy();

#define __CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __delete__) do {\
	my_actor::quit_guard qg(__host__);\
	assert(!(__this_ui__)->run_in_ui_thread());\
	begin_RUN_IN_QT_UI_AT(__this_ui__, __host__);\
	if (!(__frame__)->is_wait_close())\
		(__frame__)->close();\
	end_RUN_IN_QT_UI();\
	bool __inside_loop = true;\
	do{\
		begin_RUN_IN_QT_UI_AT(__this_ui__, __host__);\
		__inside_loop = (__frame__)->is_wait_close();\
		if (!__inside_loop) { __delete__(__frame__); }\
		end_RUN_IN_QT_UI();\
	} while (__inside_loop);\
} while (false)

//在非UI线程Actor中，关闭一个qt-ui对象
#define close_qt_ui_at(__this_ui__, __host__, __frame__) __CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __NO_DELETE_FRAME)
#define close_qt_ui(__frame__) close_qt_ui_at(this, self, __frame__)
#define close_qt_ui_delete_at(__this_ui__, __host__, __frame__) __CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __DELETE_FRAME)
#define close_qt_ui_delete(__frame__) close_qt_ui_delete_at(this, self, __frame__)
#define close_qt_ui_destroy_at(__this_ui__, __host__, __frame__) __CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __DESTROY_FRAME)
#define close_qt_ui_destroy(__frame__) close_qt_ui_destroy_at(this, self, __frame__)

#ifdef ENABLE_QT_ACTOR
#define __IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __delete__) do {\
	my_actor::quit_guard qg(__host__);\
	assert((__this_ui__)->run_in_ui_thread());\
	assert(!(__frame__)->running_in_this_thread());\
	if (!(__frame__)->is_wait_close())\
		(__frame__)->close();\
	while ((__frame__)->is_wait_close())\
		(__host__)->yield();\
	__delete__(__frame__);\
} while (false)

//在UI线程Actor中，关闭一个qt-ui对象
#define in_close_qt_ui_at(__this_ui__, __host__, __frame__) __IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __NO_DELETE_FRAME)
#define in_close_qt_ui(__frame__) in_close_qt_ui_at(this, self, __frame__)
#define in_close_qt_ui_delete_at(__this_ui__, __host__, __frame__) __IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __DELETE_FRAME)
#define in_close_qt_ui_delete(__frame__) in_close_qt_ui_delete_at(this, self, __frame__)
#define in_close_qt_ui_destroy_at(__this_ui__, __host__, __frame__) __IN_CLOSE_QT_UI_AT(__this_ui__, __host__, __frame__, __DESTROY_FRAME)
#define in_close_qt_ui_destroy(__frame__) in_close_qt_ui_destroy_at(this, self, __frame__)
#endif

//////////////////////////////////////////////////////////////////////////

//closeEvent函数中准备关闭入口ui
#define BEGIN_CLOSE_QT_MAIN_UI if (!is_wait_close() && !in_close_scope()) { set_in_close_scope_sign(true);
//closeEvent函数中等待关闭入口ui
#define WAIT_CLOSE_QT_MAIN_UI enter_wait_close();
//closeEvent函数中结束关闭入口ui
#define END_CLOSE_QT_MAIN_UI set_in_close_scope_sign(false);}

//closeEvent函数中准备关闭ui
#define BEGIN_CLOSE_QT_UI if (!is_wait_close() && !in_close_scope()) { set_in_close_scope_sign(true);
//closeEvent函数中等待关闭ui
#define WAIT_CLOSE_QT_UI async_enter_wait_close([&]{
//closeEvent函数中结束关闭ui
#define END_CLOSE_QT_UI set_in_close_scope_sign(false);});}

class bind_qt_run_base
{
protected:
#ifdef ENABLE_QT_ACTOR
	struct ui_tls
	{
		ui_tls();
		~ui_tls();

		static ui_tls* push_stack(bind_qt_run_base*);
		static bind_qt_run_base* pop_stack();
		static bind_qt_run_base* pop_stack(ui_tls*);
		static bool running_in_this_thread(bind_qt_run_base*);
		static void init();
		static void reset();

		msg_list<bind_qt_run_base*, pool_alloc<mem_alloc2<> > > _uiStack;
		void* _tlsBuff[64];
		int _count;
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
			:_handler(std::forward<H>(h)) {}

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
			:_mutex(mutex), _deadSign(deadSign), _running(running), _handler(std::forward<H>(h)) {}

		void invoke()
		{
			bool run = false;
			_mutex.lock();
			if (!_deadSign)
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
		return new(reuMem.allocate(sizeof(handler_type)))handler_type(std::forward<Handler>(handler));
	}

	template <typename Handler>
	wrap_handler_face* make_wrap_timed_handler(reusable_mem_mt<>& reuMem, std::mutex& mutex, const shared_bool& deadSign, bool& running, Handler&& handler)
	{
		typedef wrap_timed_handler<RM_CREF(Handler)> handler_type;
		return new(reuMem.allocate(sizeof(handler_type)))handler_type(mutex, deadSign, running, std::forward<Handler>(handler));
	}

	template <typename Handler, typename R>
	struct wrap_run_in_ui_handler
	{
		template <typename H>
		wrap_run_in_ui_handler(bind_qt_run_base* ui, H&& h)
			:_this(ui), _handler(std::forward<H>(h)) {}

		template <typename... Args>
		R operator()(my_actor* host, Args&&... args)
		{
			if (_this->run_in_ui_thread())
			{
				return agent_result<R>::invoke(_handler, std::forward<Args>(args)...);
			}
			else
			{
				stack_obj<R> result;
				RUN_IN_QT_UI_AT(_this, host, stack_agent_result::invoke(result, _handler, (Args&&)args...));
				return stack_obj_move::move(result);
			}
		}

		bind_qt_run_base* _this;
		Handler _handler;
		RVALUE_COPY_CONSTRUCTION(wrap_run_in_ui_handler, _this, _handler);
	};

	struct task_event : public QEvent
	{
		template <typename... Args>
		task_event(Args&&... args)
			:QEvent(std::forward<Args>(args)...) {}
		void* operator new(size_t s);
		void operator delete(void* p);
		static mem_alloc_mt2<task_event>* _taskAlloc;
	};
private:
	friend my_actor;
	static void install();
	static void uninstall();
protected:
	bind_qt_run_base();
	~bind_qt_run_base();
public:
	/*!
	@brief 获取主线程ID
	*/
	run_thread::thread_id thread_id();

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
		append_task(make_wrap_handler(_reuMem, std::forward<Handler>(handler)));
	}

	/*!
	@brief 发送一个执行函数到UI消息队列中执行，完成后返回
	*/
	template <typename Handler>
	void send(my_actor* host, Handler&& handler)
	{
		host->trig_guard([&](trig_once_notifer<>&& cb)
		{
			append_task(make_wrap_handler(_reuMem, std::bind([&handler](const trig_once_notifer<>& cb)
			{
				CHECK_EXCEPTION(handler);
				CHECK_EXCEPTION(cb);
			}, std::move(cb))));
		});
	}

	/*!
	@brief generaotr下发送一个执行函数到UI消息队列中执行
	*/
	template <typename Handler>
	void co_send(co_generator, Handler&& handler)
	{
		post(std::bind([this](generator_handle& host, Handler& handler)
		{
			CHECK_EXCEPTION(handler);
			host->_revert_this(host)->_co_async_next();
		}, std::move(co_self.async_this()), std::forward<Handler>(handler)));
	}

	/*!
	@brief 发送一个超时执行函数到UI消息队列中执行，完成后返回
	*/
	template <typename Handler>
	bool timed_send(int tm, my_actor* host, Handler&& handler)
	{
		host->lock_quit();
		bool running = false;
		trig_handle<> ath;
		trig_notifer<> ntf = host->make_trig_notifer_to_self(ath);
		append_task(make_wrap_timed_handler(_reuMem, _checkTimedMutex, ath.dead_sign(), running, [&handler, &ntf]
		{
			CHECK_EXCEPTION(handler);
			CHECK_EXCEPTION(ntf);
		}));

		if (!host->timed_wait_trig(tm, ath))
		{
			_checkTimedMutex.lock();
			if (!running)
			{
				host->close_trig_notifer(ath);
				_checkTimedMutex.unlock();
				host->unlock_quit();
				return false;
			}
			_checkTimedMutex.unlock();
			host->wait_trig(ath);
		}
		host->unlock_quit();
		return true;
	}

	/*!
	@brief 发送到UI，执行一次yield操作
	*/
	void ui_yield(my_actor* host);

	/*!
	@brief 绑定一个函数到UI队列执行
	*/
	template <typename Handler>
	wrapped_post_handler<bind_qt_run_base, RM_CREF(Handler)> wrap(Handler&& handler)
	{
		return wrapped_post_handler<bind_qt_run_base, RM_CREF(Handler)>(this, std::forward<Handler>(handler));
	}

	template <typename Handler>
	std::function<void()> wrap_check_close(Handler&& handler)
	{
		assert(run_in_ui_thread());
		_waitCount++;
		return FUNCTION_ALLOCATOR(std::function<void()>, wrap(std::bind([this](Handler& handler)
		{
			CHECK_EXCEPTION(handler);
			check_close();
		}, std::forward<Handler>(handler))), (reusable_alloc<void, reusable_mem_mt<>>(_reuMem)));
	}

	std::function<void()> wrap_check_close();

	/*!
	@brief 绑定一个函数到UI线程中执行
	*/
	template <typename R, typename Handler>
	wrap_run_in_ui_handler<RM_CREF(Handler), R> wrap_run_in_ui(Handler&& handler)
	{
		return wrap_run_in_ui_handler<RM_CREF(Handler), R>(this, std::forward<Handler>(handler));
	}
#ifdef ENABLE_QT_ACTOR
	/*!
	@brief 开启shared_qt_strand
	*/
	const shared_qt_strand& start_qt_strand(io_engine& ios);

	/*!
	@brief 获取shared_qt_strand
	*/
	const shared_qt_strand& ui_strand();

	/*!
	@brief 在UI线程中创建一个Actor，先执行start_qt_strand
	*/
	actor_handle create_ui_actor(const my_actor::main_func& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE);
	actor_handle create_ui_actor(my_actor::main_func&& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE);

	/*!
	@brief 在UI线程中创建一个子Actor，先执行start_qt_strand
	*/
	child_handle create_ui_child_actor(my_actor* host, const my_actor::main_func& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE);
	child_handle create_ui_child_actor(my_actor* host, my_actor::main_func&& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE);
#endif
private:
	void append_task(wrap_handler_face*);
protected:
	virtual void post_task_event() = 0;
	virtual void enter_loop() = 0;
	virtual void close_now() = 0;
	void run_one_task();
	void check_close();
	void enter_wait_close();

	template <typename Handler>
	void async_enter_wait_close(Handler&& handler)
	{
		assert(!_checkCloseHandler);
		_waitClose = true;
		if (_waitCount)
		{
			_checkCloseHandler = make_wrap_handler(_reuMem, std::forward<Handler>(handler));
		}
		else
		{
			CHECK_EXCEPTION(handler);
			_waitClose = false;
		}
	}
private:
	run_thread::thread_id _threadID;
	reusable_mem_mt<> _reuMem;
	std::mutex _checkTimedMutex;
	std::mutex _queueMutex;
	wrap_handler_face* _checkCloseHandler;
	msg_queue<wrap_handler_face*>* _waitQueue;
	msg_queue<wrap_handler_face*>* _readyQueue;
#ifdef ENABLE_QT_ACTOR
	shared_qt_strand _qtStrand;
#endif
protected:
	DEBUG_OPERATION(std::atomic<size_t> _taskCount);
	QEventLoop* _eventLoop;
	int _waitCount;
private:
	bool _waitClose;
	bool _inCloseScope;
	bool _locked;
};

#ifdef ENABLE_QT_ACTOR
template <typename Handler>
void qt_strand::_dispatch_ui(Handler&& handler)
{
	_ui->post(std::forward<Handler>(handler));
}

template <typename Handler>
void qt_strand::_post_ui(Handler&& handler)
{
	_ui->post(std::forward<Handler>(handler));
}
#endif

template <typename FRAME>
class bind_qt_run : public FRAME, private bind_qt_run_base
{
protected:
	template <typename... Args>
	bind_qt_run(Args&&... args)
		: FRAME(std::forward<Args>(args)...) {}

	~bind_qt_run() {}
public:
	run_thread::thread_id thread_id()
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
		bind_qt_run_base::post(std::forward<Handler>(handler));
	}

	template <typename Handler>
	void send(my_actor* host, Handler&& handler)
	{
		bind_qt_run_base::send(host, std::forward<Handler>(handler));
	}

	template <typename Handler>
	void co_send(co_generator, Handler&& handler)
	{
		bind_qt_run_base::co_send(co_self, std::forward<Handler>(handler));
	}

	template <typename Handler>
	bool timed_send(int tm, my_actor* host, Handler&& handler)
	{
		return bind_qt_run_base::timed_send(tm, host, std::forward<Handler>(handler));
	}

	template <typename Handler>
	wrapped_post_handler<bind_qt_run_base, Handler> wrap(Handler&& handler)
	{
		return bind_qt_run_base::wrap(std::forward<Handler>(handler));
	}

	template <typename Handler>
	std::function<void()> wrap_check_close(Handler&& handler)
	{
		return bind_qt_run_base::wrap_check_close(std::forward<Handler>(handler));
	}

	std::function<void()> wrap_check_close()
	{
		return bind_qt_run_base::wrap_check_close();
	}

	template <typename R = void, typename Handler>
	wrap_run_in_ui_handler<RM_CREF(Handler), R> wrap_run_in_ui(Handler&& handler)
	{
		return bind_qt_run_base::wrap_run_in_ui<R>(std::forward<Handler>(handler));
	}

	void enter_wait_close()
	{
		bind_qt_run_base::enter_wait_close();
	}

	template <typename Handler>
	void async_enter_wait_close(Handler&& handler)
	{
		bind_qt_run_base::async_enter_wait_close(std::forward<Handler>(handler));
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

	void ui_yield(my_actor* host)
	{
		bind_qt_run_base::ui_yield(host);
	}
#ifdef ENABLE_QT_ACTOR
	const shared_qt_strand& start_qt_strand(io_engine& ios)
	{
		return bind_qt_run_base::start_qt_strand(ios);
	}

	const shared_qt_strand& ui_strand()
	{
		return bind_qt_run_base::ui_strand();
	}

	actor_handle create_ui_actor(const my_actor::main_func& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE)
	{
		return bind_qt_run_base::create_ui_actor(mainFunc, stackSize);
	}

	actor_handle create_ui_actor(my_actor::main_func&& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE)
	{
		return bind_qt_run_base::create_ui_actor(std::move(mainFunc), stackSize);
	}

	child_handle create_ui_child_actor(my_actor* host, const my_actor::main_func& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE)
	{
		return bind_qt_run_base::create_ui_child_actor(host, mainFunc, stackSize);
	}

	child_handle create_ui_child_actor(my_actor* host, my_actor::main_func&& mainFunc, size_t stackSize = QT_UI_ACTOR_STACK_SIZE)
	{
		return bind_qt_run_base::create_ui_child_actor(host, std::move(mainFunc), stackSize);
	}
#endif
private:
	void post_task_event() override final
	{
		DEBUG_OPERATION(_taskCount++);
		QCoreApplication::postEvent(this, new task_event(QEvent::Type(QT_POST_TASK)));
	}

	void customEvent(QEvent* e) override final
	{
		if (e->type() == QEvent::Type(QT_POST_TASK))
		{
			assert(dynamic_cast<task_event*>(e));
			DEBUG_OPERATION(_taskCount--);
			bind_qt_run_base::run_one_task();
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

struct ui_sync_lost_exception{};

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
		_res.create(std::forward<Args>(args)...);
		_eventLoop.exit();
	}

	bool has()
	{
		return _res.has();
	}

	stack_obj<R>& result()
	{
		return _res;
	}
private:
	DEBUG_OPERATION(bind_qt_run_base* _runBase);
	QEventLoop& _eventLoop;
	stack_obj<R> _res;
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
		_handler(&res, std::forward<Args>(args)...);
		eventLoop.exec();
		DEBUG_OPERATION(_notifyCount--);
		if (!res.has())
		{
			throw ui_sync_closed_exception();
		}
		return stack_obj_move::move(res.result());
	}

	template <typename H>
	void set_notifer(H&& h)
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		_handler = std::forward<H>(h);
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
//////////////////////////////////////////////////////////////////////////

template <typename R>
struct check_lost_qt_ui_sync_result;

template <typename R>
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
		_res.create(std::forward<Args>(args)...);
		_eventLoop.exit();
	}

	bool has()
	{
		return _res.has();
	}

	stack_obj<R>& result()
	{
		return _res;
	}
private:
	bind_qt_run_base* _runBase;
	QEventLoop& _eventLoop;
	stack_obj<R> _res;
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
			if (!_lostSign)
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
						if (!lostSign)
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
		_res->return_(std::forward<Args>(args)...);
		_res = NULL;
	}
private:
	check_lost_qt_ui_sync_result(const check_lost_qt_ui_sync_result&) = delete;
	void operator=(const check_lost_qt_ui_sync_result&) = delete;
	void operator=(check_lost_qt_ui_sync_result&&) = delete;
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
		_handler(check_lost_qt_ui_sync_result<R>(&res, sign, _mutex), std::forward<Args>(args)...);
		int rcd = eventLoop.exec();
		_mutex->lock();
		sign = true;
		_mutex->unlock();
		DEBUG_OPERATION(_notifyCount--);
		if (-1 == rcd)
		{
			assert(!res.has());
			throw ui_sync_lost_exception();
		}
		if (!res.has())
		{
			throw ui_sync_closed_exception();
		}
		return stack_obj_move::move(res.result());
	}

	template <typename H>
	void set_notifer(H&& h)
	{
		assert(_runBase->run_in_ui_thread());
		assert(0 == _notifyCount);
		_handler = std::forward<H>(h);
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
#endif

#endif