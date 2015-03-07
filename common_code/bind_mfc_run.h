#ifndef __BIND_MODAL_RUN_H
#define __BIND_MODAL_RUN_H

#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <list>
#include "wrapped_post_handler.h"

using namespace std;

#define 	WM_USER_BEGIN		(WM_USER+0x8000)
#define 	WM_USER_POST		(WM_USER+0x8001)
#define 	WM_USER_SEND		(WM_USER+0x8002)
#define		WM_USER_END			(WM_USER+0x8003)

#define		BIND_MFC_RUN() \
public:\
void post(const boost::function<void ()>& h)\
{\
	boost::shared_lock<boost::shared_mutex> sl(_postMutex);\
	if (!_isClosed)\
	{\
		_mutex1.lock();\
		_postOptions.push_back(h);\
		_mutex1.unlock();\
		PostMessage(WM_USER_POST);\
	}\
}\
\
void send(const boost::function<void ()>& cb, const boost::function<void ()>& h)\
{\
	boost::shared_lock<boost::shared_mutex> sl(_postMutex);\
	if (!_isClosed)\
	{\
		_mutex2.lock();\
		_sendOptions.push_back(bind_run_pck(cb, h));\
		_mutex2.unlock();\
		PostMessage(WM_USER_SEND);\
	}\
}\
\
private:\
LRESULT _postRun(WPARAM wp, LPARAM lp)\
{\
	_mutex1.lock();\
	assert(!_postOptions.empty());\
	boost::function<void ()> h = _postOptions.front();\
	_postOptions.pop_front();\
	_mutex1.unlock();\
	assert(h);\
	h();\
	return 0;\
}\
\
LRESULT _sendRun(WPARAM wp, LPARAM lp)\
{\
	_mutex2.lock();\
	assert(!_sendOptions.empty());\
	bind_run_pck pck = _sendOptions.front();\
	_sendOptions.pop_front();\
	_mutex2.unlock();\
	pck._h();\
	pck._cb();\
	return 0;\
}\
\
afx_msg void OnCancel();

#define BIND_ACTOR_SEND()\
public:\
void send(boost_actor* actor, const boost::function<void ()>& h)\
{\
	actor->trig(boost::bind(&bind_mfc_run::send, (bind_mfc_run*)this, _1, h));\
}

#ifdef ENABLE_MFC_ACTOR
#define BIND_MFC_ACTOR(__dlg__, __base__)\
private:\
actor_handle create_mfc_actor(ios_proxy& ios, const boost_actor::main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE)\
{\
	return boost_actor::create(mfc_strand::create(ios, this), mainFunc, stackSize);\
}\
\
actor_handle create_mfc_actor(const boost_actor::main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE)\
{\
	return boost_actor::create(mfc_strand::create(this), mainFunc, stackSize);\
}\
\
boost::function<void (bool)> close_mfc_handler()\
{\
	_closeCount++;\
	return wrap(boost::bind(&__dlg__::_mfcClose, this, _1));\
}\
\
void _mfcClose(bool ok)\
{\
	if (0 == --_closeCount)\
	{\
		_isClosed = true;\
		CLEAR_MSG()\
		__base__::OnCancel();\
	}\
}
#endif

#define REGIEST_MFC_RUN(__dlg__) \
ON_WM_CLOSE()\
ON_MESSAGE(WM_USER_POST, &__dlg__::_postRun)\
ON_MESSAGE(WM_USER_SEND, &__dlg__::_sendRun)


#define CLEAR_MSG()\
{\
	boost::unique_lock<boost::shared_mutex> ul(_postMutex);\
	MSG msg;\
	while (PeekMessage(&msg, m_hWnd, WM_USER_BEGIN, WM_USER_END, PM_REMOVE))\
	{}\
	_postOptions.clear();\
	_sendOptions.clear();\
}

class bind_mfc_run
{
protected:
	struct bind_run_pck
	{
		bind_run_pck() {}

		bind_run_pck(const boost::function<void ()>& cb, const boost::function<void ()>& h)
			: _cb(cb), _h(h) {}

		boost::function<void ()> _cb;
		boost::function<void ()> _h;
	};
public:
	bind_mfc_run():_closeCount(0), _isClosed(false) {};
	~bind_mfc_run() {};
public:
	/*!
	@brief 绑定一个函数到MFC队列执行
	*/
	template <typename Handler>
	wrapped_post_handler<bind_mfc_run, Handler> wrap(const Handler& handler)
	{
		return wrapped_post_handler<bind_mfc_run, Handler>(this, handler);
	}
public:
	/*!
	@brief 发送一个执行函数到MFC消息队列中执行
	*/
	virtual void post(const boost::function<void ()>& h) = 0;

	/*!
	@brief 发送一个执行函数到MFC消息队列中执行，完成后回调
	*/
	virtual void send(const boost::function<void ()>& cb, const boost::function<void ()>& h) = 0;
protected:
	list<boost::function<void ()> > _postOptions;
	list<bind_run_pck> _sendOptions;
	boost::mutex _mutex1;
	boost::mutex _mutex2;
	boost::shared_mutex _postMutex;
	bool _isClosed;
	size_t _closeCount;
};

#endif