#include "actor_mutex.h"
#include "my_actor.h"

class MutexTrigHandle_;

class MutexTrigNotifer_
{
public:
	MutexTrigNotifer_();
	MutexTrigNotifer_(MutexTrigHandle_* trigHandle);
public:
	void operator()();
public:
	MutexTrigHandle_* _trigHandle;
	actor_handle _lockActor;
	shared_strand _strand;
	bool _notified;
	NONE_COPY(MutexTrigNotifer_);
	RVALUE_MOVE4(MutexTrigNotifer_, _trigHandle, _lockActor, _strand, _notified);
};
//////////////////////////////////////////////////////////////////////////

class MutexTrigHandle_
{
public:
	MutexTrigHandle_(my_actor* hostActor);
	~MutexTrigHandle_();
public:
	MutexTrigNotifer_ make_notifer();
	void push_msg();
	bool read_msg();
	void wait();
	bool timed_wait(int tm);
public:
	shared_strand _strand;
	my_actor* _hostActor;
	bool _hasMsg;
	bool _waiting;
	bool _outActor;
	NONE_COPY(MutexTrigHandle_);
};
//////////////////////////////////////////////////////////////////////////

void MutexTrigNotifer_::operator()()
{
	assert(!_notified);
	assert(_lockActor);
	auto& trigHandle_ = _trigHandle;
	_notified = true;
	_strand->try_tick(std::bind([trigHandle_](const actor_handle&)
	{
		trigHandle_->push_msg();
	}, std::move(_lockActor)));
}

MutexTrigNotifer_::MutexTrigNotifer_(MutexTrigHandle_* trigHandle)
:_trigHandle(trigHandle),
_lockActor(trigHandle->_hostActor->shared_from_this()),
_strand(trigHandle->_strand),
_notified(false)
{

}

MutexTrigNotifer_::MutexTrigNotifer_()
:_trigHandle(NULL), _notified(false)
{

}
//////////////////////////////////////////////////////////////////////////

void MutexTrigHandle_::wait()
{
	if (!read_msg())
	{
		if (!_outActor)
		{
			ActorFunc_::push_yield(_hostActor);
		} 
		else
		{
			ActorFunc_::push_yield_after_quited(_hostActor);
		}
	}
}

bool MutexTrigHandle_::timed_wait(int tm)
{
	assert(!_outActor);
	if (!read_msg())
	{
		bool overtime = false;
		if (tm >= 0)
		{
			_hostActor->delay_trig(tm, [&]
			{
				overtime = true;
				ActorFunc_::pull_yield(_hostActor);
			});
		}
		ActorFunc_::push_yield(_hostActor);
		if (!overtime)
		{
			if (tm >= 0)
			{
				_hostActor->cancel_delay_trig();
			}
			return true;
		}
		_waiting = false;
		return false;
	}
	return true;
}

bool MutexTrigHandle_::read_msg()
{
	assert(_strand->running_in_this_thread());
	if (_hasMsg)
	{
		_hasMsg = false;
		return true;
	}
	_waiting = true;
	return false;
}

void MutexTrigHandle_::push_msg()
{
	assert(_strand->running_in_this_thread());
	if (!_outActor)
	{
		if (!ActorFunc_::is_quited(_hostActor))
		{
			if (_waiting)
			{
				_waiting = false;
				ActorFunc_::pull_yield(_hostActor);
				return;
			}
			_hasMsg = true;
		}
	} 
	else
	{
		if (_waiting)
		{
			_waiting = false;
			ActorFunc_::pull_yield_after_quited(_hostActor);
			return;
		}
		_hasMsg = true;
	}
}

MutexTrigNotifer_ MutexTrigHandle_::make_notifer()
{
	return MutexTrigNotifer_(this);
}

MutexTrigHandle_::~MutexTrigHandle_()
{

}

MutexTrigHandle_::MutexTrigHandle_(my_actor* hostActor)
:_hasMsg(false), _waiting(false), _outActor(ActorFunc_::is_quited(hostActor))
{
	_hostActor = hostActor;
	_strand = hostActor->self_strand();
}
//////////////////////////////////////////////////////////////////////////

actor_mutex::actor_mutex(const shared_strand& strand)
:_strand(strand), _waitQueue(4), _lockActorID(0), _recCount(0)
{

}

actor_mutex::~actor_mutex()
{
	assert(!_lockActorID);
}

bool actor_mutex::check_self_err_call(my_actor* host)
{
	for (wait_node& ele : _waitQueue)
	{
		if (host->self_id() == ele._waitHostID)
		{
			return false;
		}
	}
	return true;
}

void actor_mutex::lock(my_actor* host)
{
	lock(host, []{});
}

void actor_mutex::lock(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		if (!_lockActorID || host->self_id() == _lockActorID)
		{
			_lockActorID = host->self_id();
			_recCount++;
			complete = true;
		}
		else
		{
			ntf = ath.make_notifer();
			_waitQueue.push_front({ ntf, host->self_id() });
			complete = false;
		}
	});

	if (!complete)
	{
		lockNtf();
		ath.wait();
	}
	host->unlock_quit();
}

void actor_mutex::quited_lock(my_actor* host)
{
	assert(host->self_strand()->running_in_this_thread());
	assert(_strand->running_in_this_thread());
	assert(host->in_actor());
	assert(ActorFunc_::is_quited(host));
	host->check_stack();

	if (!_lockActorID || host->self_id() == _lockActorID)
	{
		_lockActorID = host->self_id();
		_recCount++;
	}
	else
	{
		MutexTrigHandle_ ath(host);
		MutexTrigNotifer_ ntf = ath.make_notifer();
		_waitQueue.push_back({ ntf, host->self_id() });
		ath.wait();
	}
}

bool actor_mutex::try_lock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		if (!_lockActorID || host->self_id() == _lockActorID)
		{
			_lockActorID = host->self_id();
			_recCount++;
			complete = true;
		}
		else
		{
			complete = false;
		}
	});
	host->unlock_quit();
	return complete;
}

bool actor_mutex::timed_lock(int tm, my_actor* host)
{
	return timed_lock(tm, host, []{});
}

bool actor_mutex::timed_lock(int tm, my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	msg_list<wait_node>::iterator nit;
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		if (!_lockActorID || host->self_id() == _lockActorID)
		{
			_lockActorID = host->self_id();
			_recCount++;
			complete = true;
		}
		else
		{
			ntf = ath.make_notifer();
			_waitQueue.push_front({ ntf, host->self_id() });
			nit = _waitQueue.begin();
			complete = false;
		}
	});

	if (!complete)
	{
		lockNtf();
		if (!ath.timed_wait(tm))
		{
			host->send(_strand, [&]
			{
				complete = ntf._notified;
				ntf._notified = true;
				if (!complete)
				{
					_waitQueue.erase(nit);
				}
			});
			if (!complete)
			{
				host->unlock_quit();
				return false;
			}
			ath.wait();
		}
	}
	host->unlock_quit();
	return true;
}

void actor_mutex::unlock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(host->self_id() == _lockActorID);
		if (0 == --_recCount)
		{
			if (!_waitQueue.empty())
			{
				_recCount = 1;
				_lockActorID = _waitQueue.back()._waitHostID;
				_waitQueue.back().ntf();
				_waitQueue.pop_back();
			}
			else
			{
				_lockActorID = 0;
			}
		}
	});
	host->unlock_quit();
}

void actor_mutex::quited_unlock(my_actor* host)
{
	assert(host->self_strand()->running_in_this_thread());
	assert(_strand->running_in_this_thread());
	assert(host->in_actor());
	assert(ActorFunc_::is_quited(host));
	host->check_stack();

	assert(host->self_id() == _lockActorID);
	if (0 == --_recCount)
	{
		if (!_waitQueue.empty())
		{
			_recCount = 1;
			_lockActorID = _waitQueue.back()._waitHostID;
			_waitQueue.back().ntf();
			_waitQueue.pop_back();
		}
		else
		{
			_lockActorID = 0;
		}
	}
}

const shared_strand& actor_mutex::self_strand()
{
	return _strand;
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

actor_condition_variable::actor_condition_variable(const shared_strand& strand)
:_strand(strand), _waitQueue(4)
{

}

actor_condition_variable::~actor_condition_variable()
{

}

void actor_condition_variable::wait(my_actor* host, actor_lock_guard& mutex)
{
	host->assert_enter();
	assert(mutex._host == host);
	host->lock_quit();
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	host->send(_strand, [&]
	{
		ntf = ath.make_notifer();
		_waitQueue.push_back({ ntf });
	});
	mutex.unlock();
	ath.wait();
	mutex.lock();
	host->unlock_quit();
}

bool actor_condition_variable::timed_wait(int tm, my_actor* host, actor_lock_guard& mutex)
{
	host->assert_enter();
	assert(mutex._host == host);
	host->lock_quit();
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	msg_list<wait_node>::iterator nit;
	bool overtime = false;
	host->send(_strand, [&]
	{
		ntf = ath.make_notifer();
		_waitQueue.push_front({ ntf });
		nit = _waitQueue.begin();
	});
	mutex.unlock();
	if (!ath.timed_wait(tm))
	{
		host->send(_strand, [&]
		{
			if (!ntf._notified)
			{
				overtime = true;
				_waitQueue.erase(nit);
			}
		});
		if (!overtime)
		{
			ath.wait();
		}
	}
	mutex.lock();
	host->unlock_quit();
	return !overtime;
}

bool actor_condition_variable::notify_one(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	host->send(_strand, [&]
	{
		complete = !_waitQueue.empty();
		if (complete)
		{
			_waitQueue.back().ntf();
			_waitQueue.pop_back();
		}
	});
	host->unlock_quit();
	return complete;
}

size_t actor_condition_variable::notify_all(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	size_t count = 0;
	host->send(_strand, [&]
	{
		count = _waitQueue.size();
		while (!_waitQueue.empty())
		{
			_waitQueue.back().ntf();
			_waitQueue.pop_back();
		}
	});
	host->unlock_quit();
	return count;
}

const shared_strand& actor_condition_variable::self_strand()
{
	return _strand;
}
//////////////////////////////////////////////////////////////////////////

actor_shared_mutex::actor_shared_mutex(const shared_strand& strand)
:_strand(strand), _waitQueue(4), _upgradeQueue(4), _status(st_shared), _insideCount(0)
#if (_DEBUG || DEBUG)
, _inSet(4)
#endif
{

}

actor_shared_mutex::~actor_shared_mutex()
{

}

bool actor_shared_mutex::check_self_err_call(my_actor* host)
{
	for (wait_node& ele : _waitQueue)
	{
		if (host->self_id() == ele._waitHostID)
		{
			return false;
		}
	}
	return true;
}

void actor_shared_mutex::lock(my_actor* host)
{
	lock(host, []{});
}

void actor_shared_mutex::lock(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(_inSet.find(host->self_id()) == _inSet.end());
		assert(_inSet.size() == _insideCount);
		if (0 == _insideCount)
		{
			complete = true;
			_status = st_unique;
			_insideCount++;
			DEBUG_OPERATION(_inSet[host->self_id()] = st_unique);
		}
		else
		{
			complete = false;
			ntf = ath.make_notifer();
			_waitQueue.push_back({ ntf, host->self_id(), st_unique });
		}
	});
	if (!complete)
	{
		lockNtf();
		ath.wait();
	}
	assert(st_unique == _status);
	host->unlock_quit();
}

bool actor_shared_mutex::try_lock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(_inSet.find(host->self_id()) == _inSet.end());
		assert(_inSet.size() == _insideCount);
		if (0 == _insideCount)
		{
			complete = true;
			_status = st_unique;
			_insideCount++;
			DEBUG_OPERATION(_inSet[host->self_id()] = st_unique);
		}
	});
	assert(!complete || st_unique == _status);
	host->unlock_quit();
	return complete;
}

bool actor_shared_mutex::timed_lock(int tm, my_actor* host)
{
	return timed_lock(tm, host, []{});
}

bool actor_shared_mutex::timed_lock(int tm, my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	msg_list<wait_node>::iterator nit;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(_inSet.find(host->self_id()) == _inSet.end());
		assert(_inSet.size() == _insideCount);
		if (0 == _insideCount)
		{
			complete = true;
			_status = st_unique;
			_insideCount++;
			DEBUG_OPERATION(_inSet[host->self_id()] = st_unique);
		}
		else
		{
			ntf = ath.make_notifer();
			_waitQueue.push_back({ ntf, host->self_id(), st_unique });
			nit = --_waitQueue.end();
		}
	});
	if (!complete)
	{
		lockNtf();
		if (!ath.timed_wait(tm))
		{
			host->send(_strand, [&]
			{
				complete = ntf._notified;
				ntf._notified = true;
				if (!complete)
				{
					_waitQueue.erase(nit);
				}
			});
			if (!complete)
			{
				host->unlock_quit();
				return false;
			}
			ath.wait();
		}
	}
	assert(complete && st_unique == _status);
	host->unlock_quit();
	return true;
}

void actor_shared_mutex::lock_shared(my_actor* host)
{
	lock_shared(host, []{});
}

void actor_shared_mutex::lock_shared(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(_inSet.find(host->self_id()) == _inSet.end());
		assert(_inSet.size() == _insideCount);
		if (st_shared == _status)
		{
			complete = true;
			_insideCount++;
			DEBUG_OPERATION(_inSet[host->self_id()] = st_shared);
		}
		else
		{
			ntf = ath.make_notifer();
			_waitQueue.push_back({ ntf, host->self_id(), st_shared });
		}
	});
	if (!complete)
	{
		lockNtf();
		ath.wait();
	}
	assert(st_shared == _status);
	host->unlock_quit();
}

bool actor_shared_mutex::try_lock_shared(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(_inSet.find(host->self_id()) == _inSet.end());
		assert(_inSet.size() == _insideCount);
		if (st_shared == _status)
		{
			complete = true;
			_insideCount++;
			DEBUG_OPERATION(_inSet[host->self_id()] = st_shared);
		}
	});
	assert(!complete || st_shared == _status);
	host->unlock_quit();
	return complete;
}

bool actor_shared_mutex::timed_lock_shared(int tm, my_actor* host)
{
	return timed_lock_shared(tm, host, []{});
}

bool actor_shared_mutex::timed_lock_shared(int tm, my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	msg_list<wait_node>::iterator nit;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(_inSet.find(host->self_id()) == _inSet.end());
		assert(_inSet.size() == _insideCount);
		if (st_shared == _status)
		{
			complete = true;
			_insideCount++;
			DEBUG_OPERATION(_inSet[host->self_id()] = st_shared);
		}
		else
		{
			ntf = ath.make_notifer();
			_waitQueue.push_back({ ntf, host->self_id(), st_shared });
			nit = --_waitQueue.end();
		}
	});
	if (!complete)
	{
		lockNtf();
		if (!ath.timed_wait(tm))
		{
			host->send(_strand, [&]
			{
				complete = ntf._notified;
				ntf._notified = true;
				if (!complete)
				{
					_waitQueue.erase(nit);
				}
			});
			if (!complete)
			{
				host->unlock_quit();
				return false;
			}
			ath.wait();
		}
	}
	assert(complete && st_shared == _status);
	host->unlock_quit();
	return true;
}


void actor_shared_mutex::lock_upgrade(my_actor* host)
{
	lock_upgrade(host, []{});
}

void actor_shared_mutex::lock_upgrade(my_actor* host, wrap_local_handler_face<void()>&& lockNtf)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(_inSet.find(host->self_id()) != _inSet.end());
		assert(st_shared == _inSet.find(host->self_id())->second);
		assert(_inSet.size() == _insideCount);
		_status = st_upgrade;
		if (1 == _insideCount)
		{
			complete = true;
			DEBUG_OPERATION(_inSet[host->self_id()] = st_upgrade);
		}
		else
		{
			ntf = ath.make_notifer();
			_upgradeQueue.push_back(wait_node{ ntf, host->self_id(), st_upgrade });
			if (_upgradeQueue.size() == _insideCount)
			{
				wait_node& queueFront_ = _upgradeQueue.front();
				DEBUG_OPERATION(_inSet[queueFront_._waitHostID] = st_upgrade);
				queueFront_.ntf();
				_upgradeQueue.pop_front();
			}
		}
	});
	if (!complete)
	{
		lockNtf();
		ath.wait();
	}
	assert(st_upgrade == _status);
	host->unlock_quit();
}

void actor_shared_mutex::unlock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(st_unique == _status);
		assert(1 == _inSet.size());
		assert(_inSet.find(host->self_id()) != _inSet.end());
		assert(st_unique == _inSet.find(host->self_id())->second);
		assert(_inSet.size() == _insideCount);
		_insideCount--;
		DEBUG_OPERATION(_inSet.erase(host->self_id()));
		if (!_waitQueue.empty())
		{
			auto& queueFront_ = _waitQueue.front();
			DEBUG_OPERATION(_inSet[queueFront_._waitHostID] = queueFront_._status);
			_status = queueFront_._status;
			_insideCount++;
			queueFront_.ntf();
			_waitQueue.pop_front();
			if (st_shared == _status)
			{
				for (auto it = _waitQueue.begin(); it != _waitQueue.end();)
				{
					if (st_shared == it->_status)
					{
						assert(_inSet.find(it->_waitHostID) == _inSet.end());
						DEBUG_OPERATION(_inSet[it->_waitHostID] = st_shared);
						_insideCount++;
						it->ntf();
						_waitQueue.erase(it++);
					}
					else
					{
						it++;
					}
				}
			}
		}
	});
	host->unlock_quit();
}

void actor_shared_mutex::unlock_shared(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(st_unique != _status);
		assert(_inSet.find(host->self_id()) != _inSet.end());
		assert(st_shared == _inSet.find(host->self_id())->second);
		assert(_inSet.size() == _insideCount);
		_insideCount--;
		DEBUG_OPERATION(_inSet.erase(host->self_id()));
		if (st_upgrade == _status)
		{
			assert(0 != _insideCount);
			if (_upgradeQueue.size() == _insideCount)
			{
				wait_node& queueFront_ = _upgradeQueue.front();
				DEBUG_OPERATION(_inSet[queueFront_._waitHostID] = st_upgrade);
				queueFront_.ntf();
				_upgradeQueue.pop_front();
			}
		}
		else
		{
			assert(st_shared == _status);
			if (!_insideCount)
			{
				if (!_waitQueue.empty())
				{
					auto& queueFront_ = _waitQueue.front();
					assert(st_unique == queueFront_._status);
					DEBUG_OPERATION(_inSet[queueFront_._waitHostID] = queueFront_._status);
					_status = queueFront_._status;
					_insideCount++;
					queueFront_.ntf();
					_waitQueue.pop_front();
				}
			}
		}
	});
	host->unlock_quit();
}

void actor_shared_mutex::unlock_upgrade(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	host->send(_strand, [&]
	{
		assert(check_self_err_call(host));
		assert(st_upgrade == _status);
		assert(_inSet.find(host->self_id()) != _inSet.end());
		assert(st_upgrade == _inSet.find(host->self_id())->second);
		assert(_inSet.size() == _insideCount);
		if (1 == _insideCount)
		{
			complete = true;
			_status = st_shared;
			DEBUG_OPERATION(_inSet[host->self_id()] = st_shared);
		}
		else
		{
			assert(!_upgradeQueue.empty());
			wait_node queueFront = _upgradeQueue.front();
			_upgradeQueue.pop_front();
			queueFront.ntf();
			if (st_upgrade == queueFront._status)
			{
				DEBUG_OPERATION(_inSet[queueFront._waitHostID] = st_upgrade);
				ntf = ath.make_notifer();
				_upgradeQueue.push_back(wait_node{ ntf, host->self_id(), st_shared });
			}
			else
			{
				DEBUG_OPERATION(_inSet[host->self_id()] = st_shared);
				DEBUG_OPERATION(_inSet[queueFront._waitHostID] = st_shared);
				complete = true;
				_status = st_shared;
				while (!_upgradeQueue.empty())
				{
					wait_node queueFront = _upgradeQueue.front();
					_upgradeQueue.pop_front();
					assert(st_shared == queueFront._status);
					DEBUG_OPERATION(_inSet[queueFront._waitHostID] = st_shared);
					queueFront.ntf();
				}
			}
		}
		if (st_shared == _status)
		{
			for (auto it = _waitQueue.begin(); it != _waitQueue.end();)
			{
				if (st_shared == it->_status)
				{
					assert(_inSet.find(it->_waitHostID) == _inSet.end());
					DEBUG_OPERATION(_inSet[it->_waitHostID] = st_shared);
					_insideCount++;
					it->ntf();
					_waitQueue.erase(it++);
				}
				else
				{
					it++;
				}
			}
		}
	});
	if (!complete)
	{
		ath.wait();
	}
	host->unlock_quit();
	assert(st_shared == _status);
}

const shared_strand& actor_shared_mutex::self_strand()
{
	return _strand;
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