#ifndef __BIND_MODAL_RUN_H
#define __BIND_MODAL_RUN_H

#include <list>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/shared_ptr.hpp>
#include "wrapped_post_handler.h"

class bind_modal_run;

using namespace std;

#define 	WM_USER_DO_IT_1		WM_USER+100
#define 	WM_USER_DO_IT_1_V2	WM_USER+101
#define 	WM_USER_DO_IT_2		WM_USER+102

#define		BIND_MODAL_RUN() \
void post(const boost::function<void ()>& h)\
{\
	boost::shared_ptr<boost::function<void ()> > ph(new boost::function<void ()>(h));\
	_mutex1.lock();\
	_options1.push_back(ph);\
	_mutex1.unlock();\
	PostMessage(WM_USER_DO_IT_1);\
}\
void post_v2(const boost::function<void ()>& h)\
{\
	boost::shared_ptr<boost::function<void ()> > ph(new boost::function<void ()>(h));\
	bool bpost = true;\
	_mutex1_v2.lock();\
	if (_options1_v2.size() > _maxQueueLength)\
{\
	_options1_v2.pop_front();\
	bpost = false;\
}\
	_options1_v2.push_back(ph);\
	_mutex1_v2.unlock();\
	if (bpost)\
	{\
		PostMessage(WM_USER_DO_IT_1_V2);\
	}\
}\
\
void exeOption(const boost::function<void ()>& h)\
{\
	boost::shared_ptr<opt2Pck> pk(new opt2Pck);\
	pk->_h = h;\
	_mutex2.lock();\
	_options2.push_front(pk);\
	_mutex2.unlock();\
	\
	boost::unique_lock<boost::mutex> ul(pk->_mutex);\
	PostMessage(WM_USER_DO_IT_2);\
	pk->_con.wait(ul);\
}\
\
LRESULT doRun123456789_1(WPARAM wp, LPARAM lp)\
{\
	boost::shared_ptr<boost::function<void ()> > h;\
	_mutex1.lock();\
	h = _options1.front();\
	_options1.pop_front();\
	_mutex1.unlock();\
	assert(*h);\
	(*h)();\
	return 0;\
}\
\
LRESULT doRun123456789_1_V2(WPARAM wp, LPARAM lp)\
{\
	boost::shared_ptr<boost::function<void ()> > h;\
	_mutex1_v2.lock();\
	h = _options1_v2.front();\
	_options1_v2.pop_front();\
	_mutex1_v2.unlock();\
	assert(*h);\
	(*h)();\
	return 0;\
}\
\
LRESULT doRun123456789_2(WPARAM wp, LPARAM lp)\
{\
	_mutex2.lock();\
	boost::shared_ptr<opt2Pck> pk = _options2.front();\
	_options2.pop_front();\
	_mutex2.unlock();\
	assert(pk);\
	\
	pk->_h();\
	\
	pk->_mutex.lock();\
	pk->_con.notify_one();\
	pk->_mutex.unlock();\
	return 0;\
}

#define REGIEST_MODAL_RUN(dlg) \
ON_MESSAGE(WM_USER_DO_IT_1, &dlg::doRun123456789_1)\
ON_MESSAGE(WM_USER_DO_IT_1_V2, &dlg::doRun123456789_1_V2)\
ON_MESSAGE(WM_USER_DO_IT_2, &dlg::doRun123456789_2)


#define CLEAR_MSG()\
{\
	MSG msg;\
	while (PeekMessage(&msg, m_hWnd, WM_USER_DO_IT_1, WM_USER_DO_IT_2+1, PM_REMOVE))\
	{\
	}\
	_options1.clear();\
	_options2.clear();\
}

class bind_modal_run
{
protected:
	struct opt2Pck 
	{
		boost::function<void ()> _h;
		boost::mutex _mutex;
		boost::condition_variable _con;
	};
public:
	bind_modal_run()
		: _maxQueueLength(100) {};
	~bind_modal_run() {};
public:
	/*!
	@brief 绑定一个函数到MFC队列执行
	*/
	template <typename Handler>
	wrapped_post_handler<bind_modal_run, Handler> wrap(const Handler& handler)
	{
		return wrapped_post_handler<bind_modal_run, Handler>(this, handler);
	}
public:
	/*!
	@brief 发送一个执行函数到MFC消息队列中执行
	*/
	virtual void post(const boost::function<void ()>& h) = 0;
	
	/*!
	@brief 发送一个执行函数到MFC消息队列中执行，如果超过队列长度超过_maxQueueLength就丢弃第一个
	*/
	virtual void post_v2(const boost::function<void ()>& h) = 0;
	
	/*!
	@brief 发送一个执行函数到MFC消息队列中执行，执行完成后返回
	*/
	virtual void exeOption(const boost::function<void ()>& h) = 0;
protected:
	list<boost::shared_ptr<boost::function<void ()> > > _options1;
	list<boost::shared_ptr<boost::function<void ()> > > _options1_v2;
	list<boost::shared_ptr<opt2Pck> > _options2;
	boost::mutex _mutex1;
	boost::mutex _mutex1_v2;
	boost::mutex _mutex2;
	size_t _maxQueueLength;
};

#endif