#ifndef __ACTOR_MUTEX_H
#define __ACTOR_MUTEX_H

#include "shared_strand.h"

class _actor_mutex;
class my_actor;

/*!
@brief Actor锁，可递归，使用前与 my_actor::quit_guard 配合，防止 lock 后被强制退出造成无法 unlock
*/
class actor_mutex
{
public:
	actor_mutex(shared_strand strand);
	~actor_mutex();
public:
	/*!
	@brief 锁定资源，如果被别的Actor持有，等待，直到被调度，用 unlock 解除持有；可递归调用
	@warning 所在Actor如果被强制退出，有可能造成 lock 后，无法 unlock
	*/
	void lock(my_actor* self) const;

	/*!
	@brief 测试当前是否已经被占用，如果已经被别的持有（被自己持有不算）就放弃；如果没持有，则自己持有，用 unlock 解除持有
	@return 已经被别的Actor返回 false，成功持有返回true
	*/
	bool try_lock(my_actor* self) const;

	/*!
	@brief 解除当前Actor对其持有，递归 lock 几次，就需要 unlock 几次
	*/
	void unlock(my_actor* self) const;

	/*!
	@brief 重置mutex，使用前确保当前没有一个Actor依赖该mutex了
	*/
	void reset_mutex(my_actor* self);
private:
	std::shared_ptr<_actor_mutex> _amutex;
};
//////////////////////////////////////////////////////////////////////////

class actor_lock_guard
{
public:
	actor_lock_guard(const actor_mutex& amutex, my_actor* self);
	~actor_lock_guard();
private:
	actor_lock_guard(const actor_lock_guard&);
	void operator=(const actor_lock_guard&);
public:
	void unlock();
	void lock();
private:
	actor_mutex _amutex;
	my_actor* _self;
	bool _isUnlock;
};

#endif