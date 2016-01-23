#ifndef __BIND_QT_RUN_H
#define __BIND_QT_RUN_H

#ifdef ENABLE_QT_UI
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <functional>
#include <mutex>
#include <QtGui/qevent.h>
#include "wrapped_post_handler.h"
#include "actor_framework.h"
#include "qt_strand.h"
#include "msg_queue.h"

#define QT_POST_TASK	(QEvent::MaxUser-1)

//开始在Actor中，嵌入一段在qt-ui线程中执行的连续逻辑
#define begin_RUN_IN_QT_UI_FOR(__qt__, __host__) (__qt__)->send(__host__, [&]() {
#define begin_RUN_IN_QT_UI() begin_RUN_IN_QT_UI_FOR(this, self)
//结束在qt-ui线程中执行的一段连续逻辑，只有当这段逻辑执行完毕后才会执行END后续代码
#define end_RUN_IN_QT_UI() })
//////////////////////////////////////////////////////////////////////////
//在Actor中，嵌入一段在qt-ui线程中执行的语句
#define RUN_IN_QT_UI_FOR(__qt__, __host__, __exp__) (__qt__)->send(__host__, [&]() {__exp__;})
#define RUN_IN_QT_UI(__exp__) RUN_IN_QT_UI_FOR(this, self, __exp__)
//////////////////////////////////////////////////////////////////////////
//在Actor中，嵌入一段在qt-ui线程中执行的Actor逻辑（当该逻辑中包含异步操作时使用，否则建议用begin_RUN_IN_QT_UI_FOR）
#define begin_ACTOR_RUN_IN_QT_UI_FOR(__qt__, __host__, __ios__) {\
	auto ___host = __host__; \
	my_actor::quit_guard ___qg(__host__); \
	auto ___tactor = (__qt__)->create_qt_actor(__ios__, [&](my_actor* __host__) {

//在Actor中，嵌入一段在qt-ui线程中执行的Actor逻辑（当该逻辑中包含异步操作时使用，否则建议用begin_RUN_IN_QT_UI）
#define begin_ACTOR_RUN_IN_QT_UI(__ios__) begin_ACTOR_RUN_IN_QT_UI_FOR(this, self, __ios__)

//结束在qt-ui线程中执行的Actor，只有当Actor内逻辑执行完毕后才会执行END后续代码
#define end_ACTOR_RUN_IN_QT_UI()\
	}); \
	___tactor->notify_run(); \
	___host->actor_wait_quit(___tactor); \
}
//////////////////////////////////////////////////////////////////////////

//在qt-ui中执行一段绘制代码
#define begin_QT_UI_PAINT_FOR(__qt__, __host__) (__qt__)->paint(__host__, [&]() {
#define begin_QT_UI_PAINT() begin_QT_UI_PAINT_FOR(this, self)
//结束在qt-ui中执行一段绘制代码，只有当绘制代码执行完毕后才会执行END后续代码
#define end_QT_PAINT() })
//////////////////////////////////////////////////////////////////////////
//在qt-ui中执行一段绘制语句
#define QT_UI_PAINT_FOR(__qt__, __host__, __exp__) (__qt__)->paint(__host__, [&]() {__exp__;})
#define QT_UI_PAINT(__exp__) QT_UI_PAINT_FOR(this, self, __exp__)

struct qt_ui_closed_exception {};

class bind_qt_run_base
{
	struct wrap_handler_face
	{
		virtual void invoke(reusable_mem_mt<>& reuMem) = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_handler_face
	{
		template <typename H>
		wrap_handler(H&& h)
			:_handler(TRY_MOVE(h)) {}

		void invoke(reusable_mem_mt<>& reuMem)
		{
			_handler();
			this->~wrap_handler();
			reuMem.deallocate(this);
		}

		Handler _handler;
	};

	template <typename Handler>
	wrap_handler_face* make_wrap_handler(reusable_mem_mt<>& reuMem, Handler&& handler)
	{
		typedef wrap_handler<RM_CREF(Handler)> handler_type;
		return new(reuMem.allocate(sizeof(handler_type)))handler_type(TRY_MOVE(handler));
	}
protected:
	struct task_event : public QEvent
	{
		template <typename... Args>
		task_event(Args&&... args)
			:QEvent(TRY_MOVE(args)...) {}
		void* operator new(size_t s);
		void operator delete(void* p);
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
	@brief 扩充队列池长度
	*/
	void post_queue_size(size_t fixedSize);

	/*!
	@brief 发送一个执行函数到UI消息队列中执行
	*/
	template <typename Handler>
	void post(Handler&& handler)
	{
		{
			boost::shared_lock<boost::shared_mutex> sl(_postMutex);
			if (!_isClosed)
			{
				wrap_handler_face* ph = make_wrap_handler(_reuMem, TRY_MOVE(handler));
				_mutex.lock();
				_tasksQueue.push_back(ph);
				_mutex.unlock();
				postTaskEvent();
				return;
			}
		}
		assert(false);
	}

	/*!
	@brief 发送一个执行函数到UI消息队列中执行，完成后返回
	*/
	template <typename Handler>
	void send(my_actor* host, Handler&& handler)
	{
		host->lock_quit();
		bool closed = false;
		host->trig([&](trig_once_notifer<>&& cb)
		{
			boost::shared_lock<boost::shared_mutex> sl(_postMutex);
			if (!_isClosed)
			{
				wrap_handler_face* ph = make_wrap_handler(_reuMem, std::bind([&handler](const trig_once_notifer<>& cb)
				{
					handler();
					cb();
				}, std::move(cb)));
				_mutex.lock();
				_tasksQueue.push_back(ph);
				_mutex.unlock();
				sl.unlock();
				postTaskEvent();
			}
			else
			{
				sl.unlock();
				closed = true;
				cb();
			}
		});
		host->unlock_quit();
		if (closed)
		{
			throw qt_ui_closed_exception();
		}
	}

	/*!
	@brief 启动一段代码绘制图形
	*/
	template <typename Handler>
	void paint(my_actor* host, Handler&& handler)
	{
		host->lock_quit();
		bool closed = false;
		host->trig([&](trig_once_notifer<>&& cb)
		{
			boost::shared_lock<boost::shared_mutex> sl(_postMutex);
			if (!_isClosed)
			{
				wrap_handler_face* ph = make_wrap_handler(_reuMem, std::bind([&handler](const trig_once_notifer<>& cb)
				{
					handler();
					cb();
				}, std::move(cb)));
				_mutex.lock();
				_paintTasks.push_back(ph);
				_mutex.unlock();
				sl.unlock();
				paintUpdate();
			}
			else
			{
				sl.unlock();
				closed = true;
				cb();
			}
		});
		host->unlock_quit();
		if (closed)
		{
			throw qt_ui_closed_exception();
		}
	}

	/*!
	@brief 绑定一个函数到UI队列执行
	*/
	template <typename Handler>
	wrapped_post_handler<bind_qt_run_base, Handler> wrap(Handler&& handler)
	{
		return wrapped_post_handler<bind_qt_run_base, RM_CREF(Handler)>(this, TRY_MOVE(handler));
	}

#ifdef ENABLE_QT_ACTOR
	/*!
	@brief 
	*/
	shared_qt_strand make_qt_strand();
	shared_qt_strand make_qt_strand(io_engine& ios);

	/*!
	@brief 在UI线程中创建一个Actor
	@param ios Actor内部timer使用的调度器，没有就不能用timer
	*/
	actor_handle create_qt_actor(io_engine& ios, const my_actor::main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);
	actor_handle create_qt_actor(const my_actor::main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);
#endif
protected:
	virtual void postTaskEvent() = 0;
	virtual void paintUpdate() = 0;
	void runOneTask();
	void paintTask();
	void qt_ui_closed();
private:
	msg_queue<wrap_handler_face*> _tasksQueue;
	msg_queue<wrap_handler_face*> _paintTasks;
	boost::shared_mutex _postMutex;
	boost::thread::id _threadID;
	reusable_mem_mt<> _reuMem;
	std::mutex _mutex;
protected:
	bool _isClosed;
	bool _updated;
};

template <typename FRAME>
class bind_qt_run : public FRAME, private bind_qt_run_base
{
protected:
	template <typename... Args>
	bind_qt_run(Args&&... args)
		: FRAME(TRY_MOVE(args)...)
	{
	}

	virtual ~bind_qt_run()
	{

	}
public:
	boost::thread::id thread_id()
	{
		return bind_qt_run_base::thread_id();
	}

	void post_queue_size(size_t fixedSize)
	{
		return bind_qt_run_base::post_queue_size(fixedSize);
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
	void paint(my_actor* host, Handler&& handler)
	{
		bind_qt_run_base::paint(host, TRY_MOVE(handler));
	}

	template <typename Handler>
	wrapped_post_handler<bind_qt_run_base, Handler> wrap(Handler&& handler)
	{
		return bind_qt_run_base::wrap(TRY_MOVE(handler));
	}
#ifdef ENABLE_QT_ACTOR
	shared_qt_strand make_qt_strand()
	{
		return bind_qt_run_base::make_qt_strand();
	}

	shared_qt_strand make_qt_strand(io_engine& ios)
	{
		return bind_qt_run_base::make_qt_strand(ios);
	}

	actor_handle create_qt_actor(io_engine& ios, const my_actor::main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE)
	{
		return bind_qt_run_base::create_qt_actor(ios, mainFunc, stackSize);
	}

	actor_handle create_qt_actor(const my_actor::main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE)
	{
		return bind_qt_run_base::create_qt_actor(mainFunc, stackSize);
	}
#endif
private:
	void postTaskEvent() override final
	{
		QCoreApplication::postEvent(this, new task_event(QEvent::Type(QT_POST_TASK)));
	}

	void paintUpdate() override final
	{
		_updated = true;
		FRAME::update();
	}

	void paintEvent(QPaintEvent* e) override final
	{
		if (_updated)
		{
			_updated = false;
			bind_qt_run_base::paintTask();
		} 
		else
		{
			paintEvent_(e);
		}
	}

	void customEvent(QEvent* e) override final
	{
		if (e->type() == QEvent::Type(QT_POST_TASK))
		{
			if (!_isClosed)
			{
				assert(dynamic_cast<task_event*>(e));
				bind_qt_run_base::runOneTask();
			}
		}
		else
		{
			customEvent_(e);
		}
	}
protected:
	virtual void customEvent_(QEvent*)
	{

	}

	virtual void paintEvent_(QPaintEvent*)
	{

	}

	void qt_ui_closed()
	{
		bind_qt_run_base::qt_ui_closed();
	}
};
#endif

#endif