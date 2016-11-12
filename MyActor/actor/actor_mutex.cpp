#include "actor_mutex.h"
#include "my_actor.h"

actor_mutex::actor_mutex(const shared_strand& strand)
:_mutex(strand)
{
}

actor_mutex::~actor_mutex()
{
}

void actor_mutex::lock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

void actor_mutex::lock(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock(host->self_id(), std::move(ntf), wrap_ref_handler(lockNtf));
	});
	host->unlock_quit();
}

void actor_mutex::quited_lock(my_actor* host)
{
	assert(host->self_strand()->running_in_this_thread());
	assert(host->in_actor());
	assert(ActorFunc_::is_quited(host));
	host->check_stack();
	bool sign = false;
	_mutex.lock(host->self_id(), wrap_bind([&](actor_handle& host)
	{
		if (sign)
		{
			ActorFunc_::pull_yield_after_quited(host.get());
		}
		else
		{
			sign = true;
		}
	}, host->shared_from_this()));
	if (!sign)
	{
		sign = true;
		ActorFunc_::push_yield_after_quited(host);
	}
}

bool actor_mutex::try_lock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.try_lock(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

bool actor_mutex::timed_lock(int ms, my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.timed_lock(host->self_id(), ms, std::move(ntf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

bool actor_mutex::timed_lock(int ms, my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.timed_lock(host->self_id(), ms, std::move(ntf), wrap_ref_handler(lockNtf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

void actor_mutex::unlock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.unlock(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

void actor_mutex::quited_unlock(my_actor* host)
{
	assert(host->self_strand()->running_in_this_thread());
	assert(host->in_actor());
	assert(ActorFunc_::is_quited(host));
	host->check_stack();
	bool sign = false;
	_mutex.unlock(host->self_id(), wrap_bind([&](actor_handle& host)
	{
		if (sign)
		{
			ActorFunc_::pull_yield_after_quited(host.get());
		}
		else
		{
			sign = true;
		}
	}, host->shared_from_this()));
	if (!sign)
	{
		sign = true;
		ActorFunc_::push_yield_after_quited(host);
	}
}

const shared_strand& actor_mutex::self_strand()
{
	return _mutex.self_strand();
}

co_mutex& actor_mutex::comutex()
{
	return _mutex;
}
//////////////////////////////////////////////////////////////////////////

actor_lock_guard::actor_lock_guard(actor_mutex& amutex, my_actor* host)
:_amutex(amutex), _host(host), _isUnlock(false)
{
	_amutex.lock(_host);
	_host->lock_quit();
}

actor_lock_guard::~actor_lock_guard()
{
	assert(!ActorFunc_::is_quited(_host));
	if (!_isUnlock)
	{
		_amutex.unlock(_host);
	}
	_host->unlock_quit();
}

void actor_lock_guard::unlock()
{
	_isUnlock = true;
	_amutex.unlock(_host);
}

void actor_lock_guard::lock()
{
	_isUnlock = false;
	_amutex.lock(_host);
}
//////////////////////////////////////////////////////////////////////////

actor_shared_mutex::actor_shared_mutex(const shared_strand& strand)
:_mutex(strand)
{
}

actor_shared_mutex::~actor_shared_mutex()
{
}

void actor_shared_mutex::lock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

void actor_shared_mutex::lock(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock(host->self_id(), std::move(ntf), wrap_ref_handler(lockNtf));
	});
	host->unlock_quit();
}

bool actor_shared_mutex::try_lock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.try_lock(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

bool actor_shared_mutex::timed_lock(int ms, my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.timed_lock(host->self_id(), ms, std::move(ntf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

bool actor_shared_mutex::timed_lock(int ms, my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.timed_lock(host->self_id(), ms, std::move(ntf), wrap_ref_handler(lockNtf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

void actor_shared_mutex::lock_shared(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock_shared(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

void actor_shared_mutex::lock_shared(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock_shared(host->self_id(), std::move(ntf), wrap_ref_handler(lockNtf));
	});
	host->unlock_quit();
}

bool actor_shared_mutex::try_lock_shared(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.try_lock_shared(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

bool actor_shared_mutex::timed_lock_shared(int ms, my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.timed_lock_shared(host->self_id(), ms, std::move(ntf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

bool actor_shared_mutex::timed_lock_shared(int ms, my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.timed_lock_shared(host->self_id(), ms, std::move(ntf), wrap_ref_handler(lockNtf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

void actor_shared_mutex::lock_pess_shared(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock_pess_shared(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

void actor_shared_mutex::lock_pess_shared(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock_pess_shared(host->self_id(), std::move(ntf), wrap_ref_handler(lockNtf));
	});
	host->unlock_quit();
}

void actor_shared_mutex::lock_upgrade(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock_upgrade(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

void actor_shared_mutex::lock_upgrade(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.lock_upgrade(host->self_id(), std::move(ntf), wrap_ref_handler(lockNtf));
	});
	host->unlock_quit();
}

bool actor_shared_mutex::try_lock_upgrade(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	co_async_state state = host->trig<co_async_state>([&](trig_once_notifer<co_async_state>&& ntf)
	{
		_mutex.try_lock_upgrade(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
	return co_async_state::co_async_ok == state;
}

void actor_shared_mutex::unlock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.unlock(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

void actor_shared_mutex::unlock_shared(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.unlock_shared(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

void actor_shared_mutex::unlock_upgrade(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->trig([&](trig_once_notifer<>&& ntf)
	{
		_mutex.unlock_upgrade(host->self_id(), std::move(ntf));
	});
	host->unlock_quit();
}

const shared_strand& actor_shared_mutex::self_strand()
{
	return _mutex.self_strand();
}

co_shared_mutex& actor_shared_mutex::comutex()
{
	return _mutex;
}
//////////////////////////////////////////////////////////////////////////

actor_unique_lock::actor_unique_lock(actor_shared_mutex& amutex, my_actor* host)
:_amutex(amutex), _host(host), _isUnlock(false)
{
	_amutex.lock(_host);
	_host->lock_quit();
}

actor_unique_lock::~actor_unique_lock()
{
	assert(!ActorFunc_::is_quited(_host));
	if (!_isUnlock)
	{
		_amutex.unlock(_host);
	}
	_host->unlock_quit();
}

void actor_unique_lock::unlock()
{
	_isUnlock = true;
	_amutex.unlock(_host);
}

void actor_unique_lock::lock()
{
	_isUnlock = false;
	_amutex.lock(_host);
}
//////////////////////////////////////////////////////////////////////////

actor_shared_lock::actor_shared_lock(actor_shared_mutex& amutex, my_actor* host)
:_amutex(amutex), _host(host), _isUnlock(false)
{
	_amutex.lock_shared(_host);
	_host->lock_quit();
}

actor_shared_lock::~actor_shared_lock()
{
	assert(!ActorFunc_::is_quited(_host));
	if (!_isUnlock)
	{
		_amutex.unlock_shared(_host);
	}
	_host->unlock_quit();
}

void actor_shared_lock::unlock_shared()
{
	_isUnlock = true;
	_amutex.unlock_shared(_host);
}

void actor_shared_lock::lock_shared()
{
	_isUnlock = false;
	_amutex.lock_shared(_host);
}