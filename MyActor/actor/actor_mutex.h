#ifndef __ACTOR_MUTEX_H
#define __ACTOR_MUTEX_H

#include "shared_strand.h"
#include "msg_queue.h"
#include "scattered.h"

class my_actor;
class MutexTrigNotifer_;
class actor_condition_variable;

/*!
@brief Actor锁，可递归
*/
class actor_mutex
{
	friend my_actor;

	struct wait_node
	{
		MutexTrigNotifer_& ntf;
		long long _waitHostID;
	};
public:
	actor_mutex(const shared_strand& strand);
	~actor_mutex();
public:
	/*!
	@brief 锁定资源，如果被别的Actor持有，等待，直到被调度，用 unlock 解除持有；可递归调用；lock期间Actor将锁定强制退出
	@warning 所在Actor如果被强制退出，有可能造成 lock 后，无法 unlock
	*/
	void lock(my_actor* host);

	/*!
	@brief 测试当前是否已经被占用，如果已经被别的持有（被自己持有不算）就放弃；如果没持有，则自己持有，用 unlock 解除持有
	@return 已经被别的Actor返回 false，成功持有返回true
	*/
	bool try_lock(my_actor* host);

	/*!
	@brief 在一段时间内尝试锁定资源
	@return 成功返回true，超时失败返回false
	*/
	bool timed_lock(int tm, my_actor* host);

	/*!
	@brief 解除当前Actor对其持有，递归 lock 几次，就需要 unlock 几次
	*/
	void unlock(my_actor* host);

	/*!
	@brief 当前依赖的strand
	*/
	const shared_strand& self_strand();
private:
	void quited_lock(my_actor* host);
	void quited_unlock(my_actor* host);
private:
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
	long long _lockActorID;
	size_t _recCount;
	NONE_COPY(actor_mutex)
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 在一定范围内锁定mutex，同时运行的Actor也会被锁定强制退出
*/
class actor_lock_guard
{
	friend actor_condition_variable;
public:
	actor_lock_guard(actor_mutex& amutex, my_actor* host);
	~actor_lock_guard();
public:
	void unlock();
	void lock();
private:
	actor_mutex& _amutex;
	my_actor* _host;
	bool _isUnlock;
	NONE_COPY(actor_lock_guard)
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 在Actor运行的条件变量，配合actor_lock_guard使用
*/
class actor_condition_variable
{
	struct wait_node
	{
		MutexTrigNotifer_& ntf;
	};
public:
	actor_condition_variable(const shared_strand& strand);
	~actor_condition_variable();
public:
	/*!
	@brief 等待一个通知
	*/
	void wait(my_actor* host, actor_lock_guard& mutex);

	/*!
	@brief 超时等待要给通知
	*/
	bool timed_wait(int tm, my_actor* host, actor_lock_guard& mutex);

	/*!
	@brief 通知一个等待
	*/
	bool notify_one(my_actor* host);

	/*!
	@brief 通知所有等待
	*/
	size_t notify_all(my_actor* host);

	/*!
	@brief 当前依赖的strand
	*/
	const shared_strand& self_strand();
private:
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
	NONE_COPY(actor_condition_variable)
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 在Actor下运行的读写锁，不可递归
*/
class actor_shared_mutex
{
	enum lock_status
	{
		st_shared,
		st_unique,
		st_upgrade
	};

	struct wait_node
	{
		MutexTrigNotifer_& ntf;
		long long _waitHostID;
		lock_status _status;
	};
public:
	actor_shared_mutex(const shared_strand& strand);
	~actor_shared_mutex();
public:
	/*!
	@brief 独占锁
	*/
	void lock(my_actor* host);
	bool try_lock(my_actor* host);
	bool timed_lock(int tm, my_actor* host);

	/*!
	@brief 共享锁
	*/
	void lock_shared(my_actor* host);
	bool try_lock_shared(my_actor* host);
	bool timed_lock_shared(int tm, my_actor* host);

	/*!
	@brief 共享锁提升为独占锁
	*/
	void lock_upgrade(my_actor* host);
	bool try_lock_upgrade(my_actor* host);
	bool timed_lock_upgrade(int tm, my_actor* host);

	/*!
	@brief 解除独占锁定
	*/
	void unlock(my_actor* host);

	/*!
	@brief 解除共享锁定
	*/
	void unlock_shared(my_actor* host);

	/*!
	@brief 解除独占锁定，恢复为共享锁定
	*/
	void unlock_upgrade(my_actor* host);

	/*!
	@brief 当前依赖的strand
	*/
	const shared_strand& self_strand();
private:
	lock_status _status;
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
	msg_map<long long, lock_status> _inSet;
	NONE_COPY(actor_shared_mutex)
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 在一定范围内锁定shared_mutex私有锁，同时运行的Actor也会被锁定强制退出
*/
class actor_unique_lock
{
	friend actor_condition_variable;
public:
	actor_unique_lock(actor_shared_mutex& amutex, my_actor* host);
	~actor_unique_lock();
public:
	void unlock();
	void lock();
private:
	actor_shared_mutex& _amutex;
	my_actor* _host;
	bool _isUnlock;
	NONE_COPY(actor_unique_lock)
};

/*!
@brief 在一定范围内锁定shared_mutex共享锁，同时运行的Actor也会被锁定强制退出
*/
class actor_shared_lock
{
	friend actor_condition_variable;
public:
	actor_shared_lock(actor_shared_mutex& amutex, my_actor* host);
	~actor_shared_lock();
public:
	void unlock_shared();
	void lock_shared();
private:
	actor_shared_mutex& _amutex;
	my_actor* _host;
	bool _isUnlock;
	NONE_COPY(actor_shared_lock)
};
#endif