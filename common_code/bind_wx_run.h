#ifndef __BIND_WX_RUN_H
#define __BIND_WX_RUN_H

#ifdef ENABLE_WX_ACTOR

#include "wrapped_post_handler.h"
#include "actor_framework.h"
#include "wx_strand.h"
#include "msg_queue.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <functional>
#include <memory>
#include <WinSock2.h>
#include <wx/event.h>

using namespace std;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "rpcrt4.lib")
#ifdef _DEBUG
#pragma comment(lib, "wxbase30ud.lib")
#pragma comment(lib, "wxbase30ud_net.lib")
#pragma comment(lib, "wxbase30ud_xml.lib")
#pragma comment(lib, "wxexpatd.lib")
#pragma comment(lib, "wxjpegd.lib")
#pragma comment(lib, "wxmsw30ud_adv.lib")
#pragma comment(lib, "wxmsw30ud_aui.lib")
#pragma comment(lib, "wxmsw30ud_core.lib")
#pragma comment(lib, "wxmsw30ud_gl.lib")
#pragma comment(lib, "wxmsw30ud_html.lib")
#pragma comment(lib, "wxmsw30ud_media.lib")
#pragma comment(lib, "wxmsw30ud_propgrid.lib")
#pragma comment(lib, "wxmsw30ud_qa.lib")
#pragma comment(lib, "wxmsw30ud_ribbon.lib")
#pragma comment(lib, "wxmsw30ud_richtext.lib")
#pragma comment(lib, "wxmsw30ud_stc.lib")
#pragma comment(lib, "wxmsw30ud_webview.lib")
#pragma comment(lib, "wxmsw30ud_xrc.lib")
#pragma comment(lib, "wxpngd.lib")
#pragma comment(lib, "wxregexud.lib")
#pragma comment(lib, "wxscintillad.lib")
#pragma comment(lib, "wxtiffd.lib")
#pragma comment(lib, "wxzlibd.lib")
#else
#pragma comment(lib, "wxbase30u.lib")
#pragma comment(lib, "wxbase30u_net.lib")
#pragma comment(lib, "wxbase30u_xml.lib")
#pragma comment(lib, "wxexpat.lib")
#pragma comment(lib, "wxjpeg.lib")
#pragma comment(lib, "wxmsw30u_adv.lib")
#pragma comment(lib, "wxmsw30u_aui.lib")
#pragma comment(lib, "wxmsw30u_core.lib")
#pragma comment(lib, "wxmsw30u_gl.lib")
#pragma comment(lib, "wxmsw30u_html.lib")
#pragma comment(lib, "wxmsw30u_media.lib")
#pragma comment(lib, "wxmsw30u_propgrid.lib")
#pragma comment(lib, "wxmsw30u_qa.lib")
#pragma comment(lib, "wxmsw30u_ribbon.lib")
#pragma comment(lib, "wxmsw30u_richtext.lib")
#pragma comment(lib, "wxmsw30u_stc.lib")
#pragma comment(lib, "wxmsw30u_webview.lib")
#pragma comment(lib, "wxmsw30u_xrc.lib")
#pragma comment(lib, "wxpng.lib")
#pragma comment(lib, "wxregexu.lib")
#pragma comment(lib, "wxscintilla.lib")
#pragma comment(lib, "wxtiff.lib")
#pragma comment(lib, "wxzlib.lib")
#endif

#define 	WX_USER_BEGIN		(0x8000)
#define 	WX_USER_POST		(0x8001)
#define		WX_USER_END			(0x8002)

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_LOCAL_EVENT_TYPE(wxEVT_POST, WX_USER_POST)
END_DECLARE_EVENT_TYPES()

//////////////////////////////////////////////////////////////////////////
//开始在Actor中，嵌入一段在wx线程中执行的连续逻辑
#define begin_RUN_IN_WX_FOR(__WX__, __host__) (__WX__)->send(__host__, [&]() {
#define begin_RUN_IN_WX() begin_RUN_IN_WX_FOR(this, self)
//结束在wx线程中执行的一段连续逻辑，只有当这段逻辑执行完毕后才会执行END后续代码
#define end_RUN_IN_WX() })
//////////////////////////////////////////////////////////////////////////
//在Actor中，嵌入一段在wx线程中执行的语句
#define RUN_IN_WX_FOR(__WX__, __host__, __exp__) (__WX__)->send(__host__, [&]() {__exp__;})
//在Actor中，嵌入一段在wx线程中执行的语句
#define RUN_IN_WX(__exp__) RUN_IN_WX_FOR(this, self, __exp__)
//////////////////////////////////////////////////////////////////////////
//在Actor中，嵌入一段在wx线程中执行的Actor逻辑（当该逻辑中包含异步操作时使用，否则建议用begin_RUN_IN_WX_FOR）
#define begin_ACTOR_RUN_IN_WX_FOR(__WX__, __host__, __ios__) {\
	auto ___host = __host__;\
	my_actor::quit_guard ___qg(__host__); \
	auto ___tactor = (__WX__)->create_wx_actor(__ios__, [&](my_actor* __host__) {

//在Actor中，嵌入一段在wx线程中执行的Actor逻辑（当该逻辑中包含异步操作时使用，否则建议用begin_RUN_IN_WX）
#define begin_ACTOR_RUN_IN_WX(__ios__) begin_ACTOR_RUN_IN_WX_FOR(this, self, __ios__)

//结束在wx线程中执行的Actor，只有当Actor内逻辑执行完毕后才会执行END后续代码
#define end_ACTOR_RUN_IN_WX()\
}); \
	___tactor->notify_run(); \
	___host->actor_wait_quit(___tactor); \
}

//运行一个模态对话框（当前界面如果Disable，将Enable后启动）
#define SHOW_MODAL(__dlg__)\
{\
	bool __isEnabled = IsEnabled();\
	if (!__isEnabled)\
	{\
		Enable(); \
	}\
	__dlg__; \
	if (!__isEnabled)\
	{\
		Disable();\
	}\
}

//////////////////////////////////////////////////////////////////////////
class bind_wx_run_base
{
protected:
	bind_wx_run_base();
	virtual ~bind_wx_run_base();
public:
	shared_wx_strand make_wx_strand();
	shared_wx_strand make_wx_strand(ios_proxy& ios);

	/*!
	@brief 等待对象关闭
	*/
	void wait_closed(my_actor* host);

	/*!
	@brief 获取主线程ID
	*/
	boost::thread::id thread_id();

	/*!
	@brief 在wx线程中调用，将关闭该对象，之后该对象将不能进行任何操作(包括post，所以不要在wx_actor内调用)
	*/
	void wx_close();

	/*!
	@brief 扩充队列池长度
	*/
	void post_queue_size(size_t fixedSize);

	/*!
	@brief 发送一个执行函数到UI消息队列中执行
	*/
	void post(const std::function<void()>& h);

	/*!
	@brief 发送一个执行函数到UI消息队列中执行，完成后返回
	*/
	template <typename H>
	void send(my_actor* self, const H& h)
	{
		my_actor::quit_guard qg(self);
		self->trig([&](const trig_once_notifer<>& cb)
		{
			boost::shared_lock<boost::shared_mutex> sl(_postMutex);
			if (!_isClosed)
			{
				auto& h_ = h;
				_mutex.lock();
				_postOptions.push_back([&h_, cb]()
				{
					h_();
					cb();
				});
				_mutex.unlock();
				post_event();
			}
		});
	}

	/*!
	@brief 发送一个带返回值函数到UI消息队列中执行，完成后返回
	*/
	template <typename T, typename H>
	T send(my_actor* self, const H& h)
	{
		my_actor::quit_guard qg(self);
		return self->trig<T>([&](const trig_once_notifer<T>& cb)
		{
			boost::shared_lock<boost::shared_mutex> sl(_postMutex);
			if (!_isClosed)
			{
				auto& h_ = h;
				_mutex.lock();
				_postOptions.push_back([&h_, cb]()
				{
					cb(h_());
				});
				_mutex.unlock();
				post_event();
			}
		});
	}

	/*!
	@brief 绑定一个函数到UI队列执行
	*/
	template <typename Handler>
	wrapped_post_handler<bind_wx_run_base, Handler> wrap(const Handler& handler)
	{
		return wrapped_post_handler<bind_wx_run_base, Handler>(this, handler);
	}

#ifdef ENABLE_WX_ACTOR
	/*!
	@brief 在UI线程中创建一个Actor
	@param ios Actor内部timer使用的调度器，没有就不能用timer
	*/
	actor_handle create_wx_actor(ios_proxy& ios, const my_actor::main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	actor_handle create_wx_actor(const my_actor::main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);
#endif
protected:
	/*!
	@brief 继承该函数，将收到wx对象关闭消息
	*/
	virtual void OnClose();
	virtual void post_event() = 0;
	virtual void disconnect() = 0;
	void postRun(wxEvent& ue);
protected:
	msg_queue<std::function<void()> > _postOptions;
	list<actor_trig_notifer<>> _closedCallback;
	boost::mutex _mutex;
	boost::shared_mutex _postMutex;
	boost::thread::id _threadID;
	bool _isClosed;
};

/*!
@brief 作为wx对象基类使用，将使wx对象支持Actor，方法函数、构造和析构函数必须在本wx对象运行线程中执行
*/
template <typename FRAME>
class bind_wx_run: public FRAME, public bind_wx_run_base
{
protected:
	template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
	bind_wx_run(const T0& p0, const T1& p1, const T2& p2, const T3& p3, const T4& p4, const T5& p5)
		: FRAME((T0&)p0, (T1&)p1, (T2&)p2, (T3&)p3, (T4&)p4, (T5&)p5)
	{
		connect();
	}

	template <typename T0, typename T1, typename T2, typename T3, typename T4>
	bind_wx_run(const T0& p0, const T1& p1, const T2& p2, const T3& p3, const T4& p4)
		: FRAME((T0&)p0, (T1&)p1, (T2&)p2, (T3&)p3, (T4&)p4)
	{
		connect();
	}

	template <typename T0, typename T1, typename T2, typename T3>
	bind_wx_run(const T0& p0, const T1& p1, const T2& p2, const T3& p3)
		: FRAME((T0&)p0, (T1&)p1, (T2&)p2, (T3&)p3)
	{
		connect();
	}

	template <typename T0, typename T1, typename T2>
	bind_wx_run(const T0& p0, const T1& p1, const T2& p2)
		: FRAME((T0&)p0, (T1&)p1, (T2&)p2)
	{
		connect();
	}

	template <typename T0, typename T1>
	bind_wx_run(const T0& p0, const T1& p1)
		: FRAME((T0&)p0, (T1&)p1)
	{
		connect();
	}

	template <typename T0>
	bind_wx_run(const T0& p0)
		: FRAME((T0&)p0)
	{
		connect();
	}

	bind_wx_run()
	{
		connect();
	}

public:
	virtual ~bind_wx_run()
	{
		assert(boost::this_thread::get_id() == _threadID);
	}
private:
	void connect()
	{
		Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(bind_wx_run::closeEventHandler));
		Bind(wxEVT_POST, &bind_wx_run::postRun, this);
	}

	void disconnect()
	{
		Disconnect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(bind_wx_run::closeEventHandler));
		Unbind(wxEVT_POST, &bind_wx_run::postRun, this);
		Close();
	}

	void closeEventHandler(wxCloseEvent& event)
	{
		OnClose();
	}

	void post_event()
	{
		wxPostEvent(this, wxCommandEvent(wxEVT_POST));
	}
public:
	/*!
	@brief 设置当前窗口为焦点
	*/
	virtual void SetFocus()
	{
		assert(boost::this_thread::get_id() == _threadID);
		if (IsEnabled())
		{
			FRAME::SetFocus();
		}
		else
		{
			Enable();
			FRAME::SetFocus();
			Disable();
		}
	}
};
#endif// ENABLE_WX_ACTOR

#endif