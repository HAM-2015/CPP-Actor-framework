#include "actor_mutex.h"
#include "actor_framework.h"

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
			_hostActor->push_yield();
		} 
		else
		{
			_hostActor->push_yield_after_quited();
		}
	}
}

bool MutexTrigHandle_::timed_wait(int tm)
{
	assert(!_outActor);
	if (!read_msg())
	{
		bool timed = false;
		LAMBDA_REF2(ref2, _hostActor, timed);
		if (tm >= 0)
		{
			_hostActor->delay_trig(tm, [&ref2]
			{
				ref2.timed = true;
				ref2._hostActor->pull_yield();
			});
		}
		_hostActor->push_yield();
		if (!timed)
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
		if (!_hostActor->is_quited())
		{
			if (_waiting)
			{
				_waiting = false;
				_hostActor->pull_yield();
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
			_hostActor->pull_yield_after_quited();
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
:_hasMsg(false), _waiting(false), _outActor(hostActor->is_quited())
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

void actor_mutex::lock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	LAMBDA_THIS_REF4(ref4, host, complete, ath, ntf);
	host->send(_strand, [&ref4]
	{
		auto& lockActorID_ = ref4->_lockActorID;
		auto& host_ = ref4.host;
		if (!lockActorID_ || host_->self_id() == lockActorID_)
		{
			lockActorID_ = host_->self_id();
			ref4->_recCount++;
			ref4.complete = true;
		}
		else
		{
			auto& ntf_ = ref4.ntf;
			ntf_ = ref4.ath.make_notifer();
			ref4->_waitQueue.push_front({ ntf_, host_->self_id() });
			ref4.complete = false;
		}
	});

	if (!complete)
	{
		ath.wait();
	}
	host->unlock_quit();
}

void actor_mutex::quited_lock(my_actor* host)
{
	assert(host->self_strand()->running_in_this_thread());
	assert(_strand->running_in_this_thread());
	assert(host->in_actor());
	assert(host->is_quited());
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
	LAMBDA_THIS_REF2(ref2, host, complete);
	host->send(_strand, [&ref2]
	{
		auto& lockActorID_ = ref2->_lockActorID;
		auto& host_ = ref2.host;
		if (!lockActorID_ || host_->self_id() == lockActorID_)
		{
			lockActorID_ = host_->self_id();
			ref2->_recCount++;
			ref2.complete = true;
		}
		else
		{
			ref2.complete = false;
		}
	});
	host->unlock_quit();
	return complete;
}

bool actor_mutex::timed_lock(int tm, my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	msg_list<wait_node>::iterator nit;
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	LAMBDA_THIS_REF5(ref5, host, nit, complete, ath, ntf);
	host->send(_strand, [&ref5]
	{
		auto& lockActorID_ = ref5->_lockActorID;
		auto& host_ = ref5.host;
		if (!lockActorID_ || host_->self_id() == lockActorID_)
		{
			lockActorID_ = host_->self_id();
			ref5->_recCount++;
			ref5.complete = true;
		}
		else
		{
			auto& ntf_ = ref5.ntf;
			ntf_ = ref5.ath.make_notifer();
			ref5->_waitQueue.push_front({ ntf_, host_->self_id() });
			ref5.nit = ref5->_waitQueue.begin();
			ref5.complete = false;
		}
	});

	if (!complete)
	{
		if (!ath.timed_wait(tm))
		{
			host->send(_strand, [&ref5]
			{
				auto& notified_ = ref5.ntf._notified;
				auto& complete_ = ref5.complete;
				complete_ = notified_;
				notified_ = true;
				if (!complete_)
				{
					ref5->_waitQueue.erase(ref5.nit);
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
	assert(host->is_quited());
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
	assert(!_host->is_quited());
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
	LAMBDA_THIS_REF2(ref2, ath, ntf);
	host->send(_strand, [&ref2]
	{
		ref2.ntf = ref2.ath.make_notifer();
		ref2->_waitQueue.push_back({ ref2.ntf });
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
	bool timed = false;
	LAMBDA_THIS_REF4(ref4, ath, nit, ntf, timed);
	host->send(_strand, [&ref4]
	{
		ref4.ntf = ref4.ath.make_notifer();
		ref4->_waitQueue.push_front({ ref4.ntf });
		ref4.nit = ref4->_waitQueue.begin();
	});
	mutex.unlock();
	if (!ath.timed_wait(tm))
	{
		host->send(_strand, [&ref4]
		{
			if (!ref4.ntf._notified)
			{
				ref4.timed = true;
				ref4->_waitQueue.erase(ref4.nit);
			}
		});
		if (!timed)
		{
			ath.wait();
		}
	}
	mutex.lock();
	host->unlock_quit();
	return !timed;
}

bool actor_condition_variable::notify_one(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	LAMBDA_THIS_REF1(ref1, complete);
	host->send(_strand, [&ref1]
	{
		auto& complete_ = ref1.complete;
		auto& waitQueue_ = ref1->_waitQueue;
		complete_ = !waitQueue_.empty();
		if (complete_)
		{
			waitQueue_.back().ntf();
			waitQueue_.pop_back();
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
	LAMBDA_THIS_REF1(ref1, count);
	host->send(_strand, [&ref1]
	{
		auto& waitQueue_ = ref1->_waitQueue;
		ref1.count = waitQueue_.size();
		while (!waitQueue_.empty())
		{
			waitQueue_.back().ntf();
			waitQueue_.pop_back();
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
:_strand(strand), _waitQueue(4), _inSet(4), _status(st_shared)
{

}

actor_shared_mutex::~actor_shared_mutex()
{

}

void actor_shared_mutex::lock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	LAMBDA_THIS_REF4(ref4, host, complete, ath, ntf);
	host->send(_strand, [&ref4]
	{
		assert(ref4->_inSet.find(ref4.host->self_id()) == ref4->_inSet.end());
		auto& inSet_ = ref4->_inSet;
		if (inSet_.empty())
		{
			ref4.complete = true;
			ref4->_status = st_unique;
			inSet_[ref4.host->self_id()] = st_unique;
		}
		else
		{
			auto& ntf_ = ref4.ntf;
			ref4.complete = false;
			ntf_ = ref4.ath.make_notifer();
			ref4->_waitQueue.push_front({ ntf_, ref4.host->self_id(), st_unique });
		}
	});
	if (!complete)
	{
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
	LAMBDA_THIS_REF2(ref2, host, complete);
	host->send(_strand, [&ref2]
	{
		assert(ref2->_inSet.find(ref2.host->self_id()) == ref2->_inSet.end());
		if (ref2->_inSet.empty())
		{
			ref2.complete = true;
			ref2->_status = st_unique;
			ref2->_inSet[ref2.host->self_id()] = st_unique;
		}
	});
	assert(!complete || st_unique == _status);
	host->unlock_quit();
	return complete;
}

bool actor_shared_mutex::timed_lock(int tm, my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	msg_list<wait_node>::iterator nit;
	LAMBDA_THIS_REF5(ref5, host, complete, ath, ntf, nit);
	host->send(_strand, [&ref5]
	{
		assert(ref5->_inSet.find(ref5.host->self_id()) == ref5->_inSet.end());
		auto& inSet_ = ref5->_inSet;
		if (inSet_.empty())
		{
			ref5.complete = true;
			ref5->_status = st_unique;
			inSet_[ref5.host->self_id()] = st_unique;
		}
		else
		{
			auto& ntf_ = ref5.ntf;
			auto& waitQueue_ = ref5->_waitQueue;
			ntf_ = ref5.ath.make_notifer();
			waitQueue_.push_front({ ntf_, ref5.host->self_id(), st_unique });
			ref5.nit = waitQueue_.begin();
		}
	});
	if (!complete)
	{
		if (!ath.timed_wait(tm))
		{
			host->send(_strand, [&ref5]
			{
				auto& complete_ = ref5.complete;
				auto& notified_ = ref5.ntf._notified;
				complete_ = notified_;
				notified_ = true;
				if (!complete_)
				{
					ref5->_waitQueue.erase(ref5.nit);
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
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	LAMBDA_THIS_REF4(ref4, host, complete, ath, ntf);
	host->send(_strand, [&ref4]
	{
		assert(ref4->_inSet.find(ref4.host->self_id()) == ref4->_inSet.end());
		if (st_shared == ref4->_status)
		{
			ref4.complete = true;
			ref4->_inSet[ref4.host->self_id()] = st_shared;
		}
		else
		{
			auto& ntf_ = ref4.ntf;
			ntf_ = ref4.ath.make_notifer();
			ref4->_waitQueue.push_front({ ntf_, ref4.host->self_id(), st_shared });
		}
	});
	if (!complete)
	{
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
	LAMBDA_THIS_REF2(ref2, host, complete);
	host->send(_strand, [&ref2]
	{
		assert(ref2->_inSet.find(ref2.host->self_id()) == ref2->_inSet.end());
		if (st_shared == ref2->_status)
		{
			ref2.complete = true;
			ref2->_inSet[ref2.host->self_id()] = st_shared;
		}
	});
	assert(!complete || st_shared == _status);
	host->unlock_quit();
	return complete;
}

bool actor_shared_mutex::timed_lock_shared(int tm, my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	msg_list<wait_node>::iterator nit;
	LAMBDA_THIS_REF5(ref5, host, complete, ath, ntf, nit);
	host->send(_strand, [&ref5]
	{
		assert(ref5->_inSet.find(ref5.host->self_id()) == ref5->_inSet.end());
		if (st_shared == ref5->_status)
		{
			ref5.complete = true;
			ref5->_inSet[ref5.host->self_id()] = st_shared;
		}
		else
		{
			auto& ntf_ = ref5.ntf;
			auto& waitQueue_ = ref5->_waitQueue;
			ntf_ = ref5.ath.make_notifer();
			waitQueue_.push_front({ ntf_, ref5.host->self_id(), st_shared });
			ref5.nit = waitQueue_.begin();
		}
	});
	if (!complete)
	{
		if (!ath.timed_wait(tm))
		{
			host->send(_strand, [&ref5]
			{
				auto& complete_ = ref5.complete;
				auto& notified_ = ref5.ntf._notified;
				complete_ = notified_;
				notified_ = true;
				if (!complete_)
				{
					ref5->_waitQueue.erase(ref5.nit);
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
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	LAMBDA_THIS_REF4(ref4, host, complete, ath, ntf);
	host->send(_strand, [&ref4]
	{
		auto& inSet_ = ref4->_inSet;
		auto it = inSet_.find(ref4.host->self_id());
		assert(st_shared == ref4->_status);
		assert(it != ref4->_inSet.end());
		assert(st_shared == it->second);
		if (inSet_.size() == 1)
		{
			ref4.complete = true;
			ref4->_status = st_upgrade;
			inSet_[ref4.host->self_id()] = st_upgrade;
		}
		else
		{
			inSet_.erase(it);
			auto& ntf_ = ref4.ntf;
			ntf_ = ref4.ath.make_notifer();
			ref4->_waitQueue.push_front({ ntf_, ref4.host->self_id(), st_upgrade });
		}
	});
	if (!complete)
	{
		ath.wait();
	}
	assert(st_upgrade == _status);
	host->unlock_quit();
}

bool actor_shared_mutex::try_lock_upgrade(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	LAMBDA_THIS_REF2(ref2, host, complete);
	host->send(_strand, [&ref2]
	{
		assert(st_shared == ref2->_status);
		assert(ref2->_inSet.find(ref2.host->self_id()) != ref2->_inSet.end());
		assert(st_shared == ref2->_inSet.find(ref2.host->self_id())->second);
		if (ref2->_inSet.size() == 1)
		{
			ref2.complete = true;
			ref2->_status = st_upgrade;
			ref2->_inSet[ref2.host->self_id()] = st_upgrade;
		}
	});
	assert(!complete || st_upgrade == _status);
	host->unlock_quit();
	return complete;
}

bool actor_shared_mutex::timed_lock_upgrade(int tm, my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	bool complete = false;
	MutexTrigHandle_ ath(host);
	MutexTrigNotifer_ ntf;
	msg_list<wait_node>::iterator nit;
	LAMBDA_THIS_REF5(ref5, host, complete, ath, ntf, nit);
	host->send(_strand, [&ref5]
	{
		auto& inSet_ = ref5->_inSet;
		auto it = inSet_.find(ref5.host->self_id());
		assert(st_shared == ref5->_status);
		assert(it != inSet_.end());
		assert(st_shared == it->second);
		if (inSet_.size() == 1)
		{
			ref5.complete = true;
			ref5->_status = st_upgrade;
			inSet_[ref5.host->self_id()] = st_upgrade;
		}
		else
		{
			inSet_.erase(it);
			auto& waitQueue_ = ref5->_waitQueue;
			auto& ntf_ = ref5.ntf;
			ntf_ = ref5.ath.make_notifer();
			waitQueue_.push_front({ ntf_, ref5.host->self_id(), st_upgrade });
			ref5.nit = waitQueue_.begin();
		}
	});
	if (!complete)
	{
		if (!ath.timed_wait(tm))
		{
			host->send(_strand, [&ref5]
			{
				auto& complete_ = ref5.complete;
				auto& notified_ = ref5.ntf._notified;
				complete_ = notified_;
				notified_ = true;
				if (!complete_)
				{
					ref5->_waitQueue.erase(ref5.nit);
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
	assert(complete && st_upgrade == _status);
	host->unlock_quit();
	return true;
}

void actor_shared_mutex::unlock(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->send(_strand, [&]
	{
		assert(st_unique == _status);
		auto it = _inSet.find(host->self_id());
		assert(it != _inSet.end());
		assert(st_unique == it->second);
		_inSet.erase(it);
		assert(_inSet.empty());
		if (!_waitQueue.empty())
		{
			auto& queueBack_ = _waitQueue.back();
			_status = queueBack_._status;
			_inSet[queueBack_._waitHostID] = _status;
			queueBack_.ntf();
			_waitQueue.pop_back();
			if (st_shared == _status)
			{
				for (auto it = _waitQueue.begin(); it != _waitQueue.end();)
				{
					if (st_shared == it->_status)
					{
						_inSet[it->_waitHostID] = st_shared;
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
		assert(st_shared == _status);
		auto it = _inSet.find(host->self_id());
		assert(it != _inSet.end());
		assert(st_shared == it->second);
		_inSet.erase(it);
		if (_inSet.empty())
		{
			if (!_waitQueue.empty())
			{
				auto& queueBack_ = _waitQueue.back();
				_status = queueBack_._status;
				_inSet[queueBack_._waitHostID] = _status;
				queueBack_.ntf();
				_waitQueue.pop_back();
				assert(st_shared != _status);
			}
		}
	});
	host->unlock_quit();
}

void actor_shared_mutex::unlock_upgrade(my_actor* host)
{
	host->assert_enter();
	host->lock_quit();
	host->send(_strand, [&]
	{
		assert(_inSet.size() == 1);
		assert(_inSet.find(host->self_id()) != _inSet.end());
		assert(st_upgrade == _inSet.find(host->self_id())->second);
		assert(st_upgrade == _status);
		_status = st_shared;
		_inSet[host->self_id()] = st_shared;
		for (auto it = _waitQueue.begin(); it != _waitQueue.end();)
		{
			if (st_shared == it->_status)
			{
				_inSet[it->_waitHostID] = st_shared;
				it->ntf();
				_waitQueue.erase(it++);
			}
			else
			{
				it++;
			}
		}
	});
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
	assert(!_host->is_quited());
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
	assert(!_host->is_quited());
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