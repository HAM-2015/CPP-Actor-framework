#include "actor_mutex.h"
#include "msg_queue.h"
#include "actor_framework.h"
#include "scattered.h"

class mutex_trig_notifer
{
public:
	mutex_trig_notifer();
	mutex_trig_notifer(mutex_trig_handle* trigHandle);
public:
	void operator()();
public:
	mutex_trig_handle* _trigHandle;
	actor_handle _lockActor;
	shared_strand _strand;
	bool _notified;
};
//////////////////////////////////////////////////////////////////////////

class mutex_trig_handle
{
public:
	mutex_trig_handle(my_actor* hostActor);
	~mutex_trig_handle();
public:
	mutex_trig_notifer make_notifer();
	void push_msg();
	bool read_msg();
	void wait();
	bool timed_wait(int tm);
public:
	bool _hasMsg;
	bool _waiting;
	bool _outActor;
	shared_strand _strand;
	my_actor* _hostActor;
};

//////////////////////////////////////////////////////////////////////////

void mutex_trig_notifer::operator()()
{
	assert(!_notified);
	auto& trigHandle_ = _trigHandle;
	auto& lockActor_ = _lockActor;
	_notified = true;
	_strand->try_tick([trigHandle_, lockActor_]
	{
		trigHandle_->push_msg();
	});
}

mutex_trig_notifer::mutex_trig_notifer(mutex_trig_handle* trigHandle)
:_trigHandle(trigHandle),
_lockActor(trigHandle->_hostActor->shared_from_this()),
_strand(trigHandle->_strand),
_notified(false)
{

}

mutex_trig_notifer::mutex_trig_notifer()
:_trigHandle(NULL), _notified(false)
{

}
//////////////////////////////////////////////////////////////////////////

void mutex_trig_handle::wait()
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

bool mutex_trig_handle::timed_wait(int tm)
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

bool mutex_trig_handle::read_msg()
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

void mutex_trig_handle::push_msg()
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

mutex_trig_notifer mutex_trig_handle::make_notifer()
{
	return mutex_trig_notifer(this);
}

mutex_trig_handle::~mutex_trig_handle()
{

}

mutex_trig_handle::mutex_trig_handle(my_actor* hostActor)
:_hasMsg(false), _waiting(false), _outActor(hostActor->is_quited())
{
	_hostActor = hostActor;
	_strand = hostActor->self_strand();
}
//////////////////////////////////////////////////////////////////////////

class ActorMutex_
{
	struct wait_node
	{
		mutex_trig_notifer& ntf;
		my_actor::id _waitHostID;
	};
public:
	ActorMutex_(const shared_strand& strand)
		:_strand(strand), _waitQueue(4), _lockActorID(0), _recCount(0)
	{

	}

	~ActorMutex_()
	{
		assert(!_lockActorID);
	}
public:
	void lock(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
		qg.unlock();
	}

	void quited_lock(my_actor* host)
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
			mutex_trig_handle ath(host);
			mutex_trig_notifer ntf = ath.make_notifer();
			_waitQueue.push_back({ ntf, host->self_id() });
			ath.wait();
		}
	}

	bool try_lock(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
		return complete;
	}

	bool timed_lock(int tm, my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		msg_list<wait_node>::iterator nit;
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
					qg.unlock();
					return false;
				}
				ath.wait();
			}
		}
		qg.unlock();
		return true;
	}

	void unlock(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
	}
	
	void quited_unlock(my_actor* host)
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
private:
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
	my_actor::id _lockActorID;
	size_t _recCount;
};
//////////////////////////////////////////////////////////////////////////

actor_mutex::actor_mutex(const shared_strand& strand)
:_amutex(new ActorMutex_(strand)) {}

actor_mutex::actor_mutex(const actor_mutex& s)
: _amutex(s._amutex) {}

actor_mutex::actor_mutex(actor_mutex&& s)
: _amutex(std::move(s._amutex)) {}

actor_mutex::actor_mutex()
{

}

actor_mutex::~actor_mutex()
{

}

void actor_mutex::lock(my_actor* host) const
{
	_amutex->lock(host);
}

bool actor_mutex::try_lock(my_actor* host) const
{
	return _amutex->try_lock(host);
}

bool actor_mutex::timed_lock(int tm, my_actor* host) const
{
	return _amutex->timed_lock(tm, host);
}

void actor_mutex::unlock(my_actor* host) const
{
	_amutex->unlock(host);
}

void actor_mutex::quited_lock(my_actor* host) const
{
	_amutex->quited_lock(host);
}

void actor_mutex::quited_unlock(my_actor* host) const
{
	_amutex->quited_unlock(host);
}

void actor_mutex::operator=(const actor_mutex& s)
{
	_amutex = s._amutex;
}

void actor_mutex::operator=(actor_mutex&& s)
{
	_amutex = std::move(s._amutex);
}

//////////////////////////////////////////////////////////////////////////

actor_lock_guard::actor_lock_guard(const actor_mutex& amutex, my_actor* host)
:_amutex(amutex), _host(host), _isUnlock(false)
{
	_amutex.lock(_host);
	_host->lock_quit();
}

actor_lock_guard::actor_lock_guard(const actor_lock_guard&)
: _amutex(_amutex), _host(0)
{

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

void actor_lock_guard::operator=(const actor_lock_guard&)
{

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

class ActorConditionVariable_
{
	struct wait_node 
	{
		mutex_trig_notifer& ntf;
	};
public:
	ActorConditionVariable_(const shared_strand& strand)
		:_strand(strand), _waitQueue(4)
	{

	}

	~ActorConditionVariable_()
	{

	}
public:
	void wait(my_actor* host, actor_lock_guard& mutex)
	{
		host->assert_enter();
		assert(mutex._host == host);
		my_actor::quit_guard qg(host);
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
		LAMBDA_THIS_REF2(ref2, ath, ntf);
		host->send(_strand, [&ref2]
		{
			ref2.ntf = ref2.ath.make_notifer();
			ref2->_waitQueue.push_back({ ref2.ntf });
		});
		mutex.unlock();
		ath.wait();
		mutex.lock();
		qg.unlock();
	}

	bool timed_wait(int tm, my_actor* host, actor_lock_guard& mutex)
	{
		host->assert_enter();
		assert(mutex._host == host);
		my_actor::quit_guard qg(host);
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
		qg.unlock();
		return !timed;
	}

	bool notify_one(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
		return complete;
	}

	size_t notify_all(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
		return count;
	}
private:
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
};
//////////////////////////////////////////////////////////////////////////

actor_condition_variable::actor_condition_variable(const shared_strand& strand)
:_aconVar(new ActorConditionVariable_(strand)) {}

actor_condition_variable::actor_condition_variable(const actor_condition_variable& s)
: _aconVar(s._aconVar) {}

actor_condition_variable::actor_condition_variable(actor_condition_variable&& s)
: _aconVar(std::move(s._aconVar)) {}

actor_condition_variable::actor_condition_variable()
{

}

actor_condition_variable::~actor_condition_variable()
{

}

void actor_condition_variable::wait(my_actor* host, actor_lock_guard& mutex) const
{
	_aconVar->wait(host, mutex);
}

bool actor_condition_variable::timed_wait(int tm, my_actor* host, actor_lock_guard& mutex) const
{
	return _aconVar->timed_wait(tm, host, mutex);
}

bool actor_condition_variable::notify_one(my_actor* host) const
{
	return _aconVar->notify_one(host);
}

size_t actor_condition_variable::notify_all(my_actor* host) const
{
	return _aconVar->notify_all(host);
}

void actor_condition_variable::operator=(const actor_condition_variable& s)
{
	_aconVar = s._aconVar;
}

void actor_condition_variable::operator=(actor_condition_variable&& s)
{
	_aconVar = std::move(s._aconVar);
}

//////////////////////////////////////////////////////////////////////////

class ActorSharedMutex
{
	enum lock_status
	{
		st_shared,
		st_unique,
		st_upgrade
	};

	struct wait_node
	{
		mutex_trig_notifer& ntf;
		my_actor::id _waitHostID;
		lock_status _status;
	};
public:
	ActorSharedMutex(const shared_strand& strand)
		:_strand(strand), _waitQueue(4), _inSet(4), _status(st_shared)
	{

	}

	~ActorSharedMutex()
	{

	}
public:
	void lock(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
		qg.unlock();
	}

	bool try_lock(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
		return complete;
	}

	bool timed_lock(int tm, my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
					qg.unlock();
					return false;
				}
				ath.wait();
			}
		}
		assert(complete && st_unique == _status);
		qg.unlock();
		return true;
	}

	void lock_shared(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
		qg.unlock();
	}

	bool try_lock_shared(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
		return complete;
	}

	bool timed_lock_shared(int tm, my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
					qg.unlock();
					return false;
				}
				ath.wait();
			}
		}
		assert(complete && st_shared == _status);
		qg.unlock();
		return true;
	}

	void lock_upgrade(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
		qg.unlock();
	}

	bool try_lock_upgrade(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
		return complete;
	}

	bool timed_lock_upgrade(int tm, my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
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
					qg.unlock();
					return false;
				}
				ath.wait();
			}
		}
		assert(complete && st_upgrade == _status);
		qg.unlock();
		return true;
	}

	void unlock(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
	}

	void unlock_shared(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
	}

	void unlock_upgrade(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
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
		qg.unlock();
		assert(st_shared == _status);
	}
private:
	lock_status _status;
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
	msg_map<my_actor::id, lock_status> _inSet;
};
//////////////////////////////////////////////////////////////////////////

actor_shared_mutex::actor_shared_mutex(const shared_strand& strand)
:_amutex(new ActorSharedMutex(strand))
{

}

actor_shared_mutex::actor_shared_mutex(const actor_shared_mutex& s)
: _amutex(s._amutex) {}

actor_shared_mutex::actor_shared_mutex(actor_shared_mutex&& s)
: _amutex(std::move(s._amutex)) {}

actor_shared_mutex::actor_shared_mutex()
{

}

actor_shared_mutex::~actor_shared_mutex()
{

}

void actor_shared_mutex::lock(my_actor* host) const
{
	_amutex->lock(host);
}

bool actor_shared_mutex::try_lock(my_actor* host) const
{
	return _amutex->try_lock(host);
}

bool actor_shared_mutex::timed_lock(int tm, my_actor* host) const
{
	return _amutex->timed_lock(tm, host);
}

void actor_shared_mutex::lock_shared(my_actor* host) const
{
	_amutex->lock_shared(host);
}

bool actor_shared_mutex::try_lock_shared(my_actor* host) const
{
	return _amutex->try_lock_shared(host);
}

bool actor_shared_mutex::timed_lock_shared(int tm, my_actor* host) const
{
	return _amutex->timed_lock_shared(tm, host);
}

void actor_shared_mutex::lock_upgrade(my_actor* host) const
{
	_amutex->lock_upgrade(host);
}

bool actor_shared_mutex::try_lock_upgrade(my_actor* host) const
{
	return _amutex->try_lock_upgrade(host);
}

bool actor_shared_mutex::timed_lock_upgrade(int tm, my_actor* host) const
{
	return _amutex->timed_lock_upgrade(tm, host);
}

void actor_shared_mutex::unlock(my_actor* host) const
{
	_amutex->unlock(host);
}

void actor_shared_mutex::unlock_shared(my_actor* host) const
{
	_amutex->unlock_shared(host);
}

void actor_shared_mutex::unlock_upgrade(my_actor* host) const
{
	_amutex->unlock_upgrade(host);
}

void actor_shared_mutex::operator=(const actor_shared_mutex& s)
{
	_amutex = s._amutex;
}

void actor_shared_mutex::operator=(actor_shared_mutex&& s)
{
	_amutex = std::move(s._amutex);
}

//////////////////////////////////////////////////////////////////////////

actor_unique_lock::actor_unique_lock(const actor_shared_mutex& amutex, my_actor* host)
:_amutex(amutex), _host(host), _isUnlock(false)
{
	_amutex.lock(_host);
	_host->lock_quit();
}

actor_unique_lock::actor_unique_lock(const actor_unique_lock&)
: _amutex(_amutex), _host(0)
{

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

void actor_unique_lock::operator=(const actor_unique_lock&)
{

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

actor_shared_lock::actor_shared_lock(const actor_shared_mutex& amutex, my_actor* host)
:_amutex(amutex), _host(host), _isUnlock(false)
{
	_amutex.lock_shared(_host);
	_host->lock_quit();
}

actor_shared_lock::actor_shared_lock(const actor_shared_lock&)
: _amutex(_amutex), _host(0)
{

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

void actor_shared_lock::operator=(const actor_shared_lock&)
{

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