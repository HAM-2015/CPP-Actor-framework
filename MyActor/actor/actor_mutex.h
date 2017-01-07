#ifndef __ACTOR_MUTEX_H
#define __ACTOR_MUTEX_H

#include "run_strand.h"
#include "msg_queue.h"
#include "scattered.h"
#include "lambda_ref.h"
#include "generator.h"

class my_actor;

/*!
@brief Actor锁，可递归
*/
class actor_mutex
{
	friend my_actor;
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
	@brief 锁定资源，如果已经被锁定调用lockNtf
	*/
	template <typename Ntf>
	void lock(my_actor* host, Ntf&& lockNtf)
	{
		lock(host, (wrap_local_handler_face<void()>&&)wrap_local_handler(lockNtf));
	}

	/*!
	@brief 测试当前是否已经被占用，如果已经被别的持有（被自己持有不算）就放弃；如果没持有，则自己持有，用 unlock 解除持有
	@return 已经被别的Actor返回 false，成功持有返回true
	*/
	bool try_lock(my_actor* host);

	/*!
	@brief 在一段时间内尝试锁定资源
	@return 成功返回true，超时失败返回false
	*/
	bool timed_lock(int ms, my_actor* host);

	/*!
	@brief 在一段时间内尝试锁定资源，如果已经被锁定调用lockNtf
	*/
	template <typename Ntf>
	bool timed_lock(int ms, my_actor* host, Ntf&& lockNtf)
	{
		return timed_lock(ms, host, (wrap_local_handler_face<void()>&&)wrap_local_handler(lockNtf));
	}

	/*!
	@brief 解除当前Actor对其持有，递归 lock 几次，就需要 unlock 几次
	*/
	void unlock(my_actor* host);

	/*!
	@brief 当前依赖的strand
	*/
	const shared_strand& self_strand();

	/*!
	@brief 
	*/
	co_mutex& co_ref();
private:
	void quited_lock(my_actor* host);
	void quited_unlock(my_actor* host);
	void lock(my_actor*, wrap_local_handler_face<void()>&& lockNtf);
	bool timed_lock(int ms, my_actor* host, wrap_local_handler_face<void()>&& lockNtf);
private:
	co_mutex _mutex;
	NONE_COPY(actor_mutex)
};
//////////////////////////////////////////////////////////////////////////

class actor_condition_variable;
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
@brief 在Actor下运行的读写锁，不可递归
*/
class actor_shared_mutex
{
public:
	actor_shared_mutex(const shared_strand& strand);
	~actor_shared_mutex();
public:
	/*!
	@brief 独占锁
	*/
	void lock(my_actor* host);
	bool try_lock(my_actor* host);
	bool timed_lock(int ms, my_actor* host);

	template <typename Ntf>
	void lock(my_actor* host, Ntf&& lockNtf)
	{
		lock(host, (wrap_local_handler_face<void()>&&)wrap_local_handler(lockNtf));
	}

	template <typename Ntf>
	bool timed_lock(int ms, my_actor* host, Ntf&& lockNtf)
	{
		return timed_lock(ms, host, (wrap_local_handler_face<void()>&&)wrap_local_handler(lockNtf));
	}

	/*!
	@brief 共享锁
	*/
	void lock_shared(my_actor* host);
	bool try_lock_shared(my_actor* host);
	bool timed_lock_shared(int ms, my_actor* host);
	void lock_pess_shared(my_actor* host);

	template <typename Ntf>
	void lock_shared(my_actor* host, Ntf&& lockNtf)
	{
		lock_shared(host, (wrap_local_handler_face<void()>&&)wrap_local_handler(lockNtf));
	}

	template <typename Ntf>
	bool timed_lock_shared(int ms, my_actor* host, Ntf&& lockNtf)
	{
		return timed_lock_shared(ms, host, (wrap_local_handler_face<void()>&&)wrap_local_handler(lockNtf));
	}

	template <typename Ntf>
	void lock_pess_shared(my_actor* host, Ntf&& lockNtf)
	{
		lock_pess_shared(host, (wrap_local_handler_face<void()>&&)wrap_local_handler(lockNtf));
	}

	/*!
	@brief 共享锁提升为独占锁
	*/
	void lock_upgrade(my_actor* host);
	bool try_lock_upgrade(my_actor* host);

	template <typename Ntf>
	void lock_upgrade(my_actor* host, Ntf&& lockNtf)
	{
		lock_upgrade(host, (wrap_local_handler_face<void()>&&)wrap_local_handler(lockNtf));
	}

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

	/*!
	@brief
	*/
	co_shared_mutex& co_ref();
private:
	void lock(my_actor* host, wrap_local_handler_face<void()>&& lockNtf);
	bool timed_lock(int ms, my_actor* host, wrap_local_handler_face<void()>&& lockNtf);
	void lock_shared(my_actor* host, wrap_local_handler_face<void()>&& lockNtf);
	bool timed_lock_shared(int ms, my_actor* host, wrap_local_handler_face<void()>&& lockNtf);
	void lock_pess_shared(my_actor* host, wrap_local_handler_face<void()>&& lockNtf);
	void lock_upgrade(my_actor* host, wrap_local_handler_face<void()>&& lockNtf);
private:
	co_shared_mutex _mutex;
	NONE_COPY(actor_shared_mutex)
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 在一定范围内锁定shared_mutex私有锁，同时运行的Actor也会被锁定强制退出
*/
class actor_unique_lock
{
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
//////////////////////////////////////////////////////////////////////////

/*!
@brief 条件变量
*/
class actor_condition_variable
{
public:
	actor_condition_variable(const shared_strand& strand);
	~actor_condition_variable();
public:
	/*!
	@brief 等待通知
	*/
	void wait(my_actor* host, actor_lock_guard& lg);
	void wait(my_actor* host, actor_mutex& mtx);
	void wait(my_actor* host, co_mutex& mtx);

	/*!
	@brief 延时等待通知
	*/
	bool timed_wait(my_actor* host, actor_lock_guard& lg, int ms);
	bool timed_wait(my_actor* host, actor_mutex& mtx, int ms);
	bool timed_wait(my_actor* host, co_mutex& mtx, int ms);

	/*!
	@brief 通知一个等待
	*/
	void notify_one();

	/*!
	@brief 通知所有在等待的
	*/
	void notify_all();

	/*!
	@brief 
	*/
	co_condition_variable& co_ref();
private:
	co_condition_variable _conVar;
};
#endif