#ifndef __ACTOR_TIMER_H
#define __ACTOR_TIMER_H

#include "shared_strand.h"
#include "msg_queue.h"
#include <boost/thread/mutex.hpp>

class boost_strand;
class mfc_strand;
class wx_strand;
class my_actor;

/*!
@brief Actor 内部使用的定时器
*/
class actor_timer
{
	typedef std::shared_ptr<my_actor> actor_handle;
	typedef msg_list_shared_alloc<actor_handle, null_mutex>::shared_node_alloc list_alloc;
	typedef std::shared_ptr<msg_list_shared_alloc<actor_handle, null_mutex> > handler_list;
	typedef msg_map<unsigned long long, handler_list>::node_alloc map_alloc;
	typedef msg_map<unsigned long long, handler_list, map_alloc> handler_table;
	typedef shared_obj_pool<msg_list_shared_alloc<actor_handle, null_mutex> > handler_list_pool;

	friend boost_strand;
	friend mfc_strand;
	friend wx_strand;
	friend my_actor;

	class timer_handle 
	{
		friend actor_timer;

		std::weak_ptr<msg_list_shared_alloc<actor_handle, null_mutex> > _handlerList;
		msg_list_shared_alloc<actor_handle, null_mutex>::iterator _handlerNode;
		handler_table::iterator _tableNode;
	};
private:
	actor_timer(shared_strand strand);
	~actor_timer();
private:
	/*!
	@brief 开始计时
	@param us 微秒
	@param host 准备计时的Actor
	@return 计时句柄，用于cancel
	*/
	timer_handle timeout(unsigned long long us, const actor_handle& host);

	/*!
	@brief 取消计时
	*/
	void cancel(timer_handle& th);

	/*!
	@brief timer循环
	*/
	void timer_loop(unsigned long long us);
private:
	ios_proxy& _ios;
	void* _timer;
	bool _looping;
	int _timerCount;
	list_alloc _listAlloc;
	shared_strand _strand;
	handler_table _handlerTable;
	handler_list_pool* _listPool;
	unsigned long long _extFinishTime;
	std::weak_ptr<boost_strand> _weakStrand;
};

#endif