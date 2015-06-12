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
	_notified = true;
	_strand->post([=]
	{
		trigHandle_->push_msg();
	});
}

mutex_trig_notifer::mutex_trig_notifer(mutex_trig_handle* trigHandle)
:_trigHandle(trigHandle),
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
			_hostActor->push_yield_as_mutex();
		}
	}
}

bool mutex_trig_handle::timed_wait(int tm)
{
	assert(!_outActor);
	if (!read_msg())
	{
		bool timeout = false;
		LAMBDA_REF2(ref2, _hostActor, timeout);
		if (tm >= 0)
		{
			_hostActor->delay_trig(tm, [&ref2]
			{
				ref2.timeout = true;
				ref2._hostActor->pull_yield();
			});
		}
		_hostActor->push_yield();
		if (!timeout)
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
			_hostActor->pull_yield_as_mutex();
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

class _actor_mutex
{
	struct wait_node
	{
		mutex_trig_notifer& ntf;
		my_actor::id _waitHostID;
	};
public:
	_actor_mutex(shared_strand strand)
		:_strand(strand), _waitQueue(4), _lockActorID(0), _recCount(0)
	{

	}

	~_actor_mutex()
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
				wait_node wn = { ntf_, host_->self_id() };
				ref4->_waitQueue.push_front(wn);
				ref4.complete = false;
			}
		});

		if (!complete)
		{
			ath.wait();
		}
	}

	void quited_lock(my_actor* host)
	{
		assert(host->self_strand()->running_in_this_thread());
		assert(host->in_actor());
		assert(host->is_quited());
		host->check_stack();
		bool complete = false;
		mutex_trig_handle ath(host);
		mutex_trig_notifer ntf;
		LAMBDA_THIS_REF4(ref4, host, complete, ath, ntf);
		auto h = [&ref4]
		{
			if (!ref4->_lockActorID || ref4.host->self_id() == ref4->_lockActorID)
			{
				ref4->_lockActorID = ref4.host->self_id();
				ref4->_recCount++;
				ref4.complete = true;
			}
			else
			{
				ref4.ntf = ref4.ath.make_notifer();
				wait_node wn = { ref4.ntf, ref4.host->self_id() };
				ref4->_waitQueue.push_back(wn);
				ref4.complete = false;
			}
		};

		if (_strand != host->self_strand())
		{
			actor_handle shared_host = host->shared_from_this();
			_strand->asyncInvokeVoid(h, [shared_host]
			{
				auto& shared_host_ = shared_host;
				shared_host->self_strand()->post([=]
				{
					shared_host_->pull_yield_as_mutex();
				});
			});
			host->push_yield_as_mutex();
		}
		else
		{
			h();
		}

		if (!complete)
		{
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
				wait_node wn = { ntf_, host_->self_id() };
				ref5->_waitQueue.push_front(wn);
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
					return false;
				}
				ath.wait();
			}
		}
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
	}
	
	void quited_unlock(my_actor* host)
	{
		assert(host->self_strand()->running_in_this_thread());
		assert(host->in_actor());
		assert(host->is_quited());
		host->check_stack();
		auto h = [&]
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
		};

		if (_strand != host->self_strand())
		{
			actor_handle shared_host = host->shared_from_this();
			_strand->asyncInvokeVoid(h, [shared_host]
			{
				auto& shared_host_ = shared_host;
				shared_host->self_strand()->post([=]
				{
					shared_host_->pull_yield_as_mutex();
				});
			});
			host->push_yield_as_mutex();
		}
		else
		{
			h();
		}
	}
private:
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
	my_actor::id _lockActorID;
	size_t _recCount;
};
//////////////////////////////////////////////////////////////////////////

actor_mutex::actor_mutex(shared_strand strand)
:_amutex(new _actor_mutex(strand))
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

class _actor_condition_variable
{
	struct wait_node 
	{
		mutex_trig_notifer& ntf;
	};
public:
	_actor_condition_variable(shared_strand strand)
		:_strand(strand), _waitQueue(4)
	{

	}

	~_actor_condition_variable()
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
			wait_node wn = { ref2.ntf };
			ref2->_waitQueue.push_back(wn);
		});
		mutex.unlock();
		ath.wait();
		mutex.lock();
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
			wait_node wn = { ref4.ntf };
			ref4->_waitQueue.push_front(wn);
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
		return count;
	}
private:
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
};
//////////////////////////////////////////////////////////////////////////

actor_condition_variable::actor_condition_variable(shared_strand strand)
:_aconVar(new _actor_condition_variable(strand))
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
//////////////////////////////////////////////////////////////////////////

class _actor_shared_mutex
{
	struct wait_node
	{
		mutex_trig_notifer& ntf;
		my_actor::id _waitHostID;
		bool _isShared;
	};
public:
	_actor_shared_mutex(shared_strand strand)
		:_strand(strand), _waitQueue(4), _inSet(4), _shared(true)
	{

	}

	~_actor_shared_mutex()
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
				ref4->_shared = false;
				inSet_[ref4.host->self_id()] = false;
			}
			else
			{
				auto& ntf_ = ref4.ntf;
				ref4.complete = false;
				ntf_ = ref4.ath.make_notifer();
				wait_node wn = { ntf_, ref4.host->self_id(), false };
				ref4->_waitQueue.push_front(wn);
			}
		});
		if (!complete)
		{
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
			assert(ref2->_inSet.find(ref2.host->self_id()) == ref2->_inSet.end());
			if (ref2->_inSet.empty())
			{
				ref2.complete = true;
				ref2->_shared = false;
				ref2->_inSet[ref2.host->self_id()] = false;
			}
		});
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
				ref5->_shared = false;
				inSet_[ref5.host->self_id()] = false;
			}
			else
			{
				auto& ntf_ = ref5.ntf;
				auto& waitQueue_ = ref5->_waitQueue;
				ref5.complete = false;
				ntf_ = ref5.ath.make_notifer();
				wait_node wn = { ntf_, ref5.host->self_id(), false };
				waitQueue_.push_front(wn);
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
					return false;
				}
				ath.wait();
			}
		}
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
			if (ref4->_shared)
			{
				ref4.complete = true;
				ref4->_inSet[ref4.host->self_id()] = true;
			}
			else
			{
				auto& ntf_ = ref4.ntf;
				ref4.complete = false;
				ntf_ = ref4.ath.make_notifer();
				wait_node wn = { ntf_, ref4.host->self_id(), true };
				ref4->_waitQueue.push_front(wn);
			}
		});
		if (!complete)
		{
			ath.wait();
		}
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
			if (ref2->_shared)
			{
				ref2.complete = true;
				ref2->_inSet[ref2.host->self_id()] = true;
			}
		});
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
			if (ref5->_shared)
			{
				ref5.complete = true;
				ref5->_inSet[ref5.host->self_id()] = true;
			}
			else
			{
				auto& ntf_ = ref5.ntf;
				auto& waitQueue_ = ref5->_waitQueue;
				ref5.complete = false;
				ntf_ = ref5.ath.make_notifer();
				wait_node wn = { ntf_, ref5.host->self_id(), true };
				waitQueue_.push_front(wn);
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
					return false;
				}
				ath.wait();
			}
		}
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
			assert(ref4->_shared);
			assert(ref4->_inSet.find(ref4.host->self_id()) != ref4->_inSet.end());
			assert(ref4->_inSet.find(ref4.host->self_id())->second);
			auto& inSet_ = ref4->_inSet;
			if (inSet_.size() == 1)
			{
				ref4.complete = true;
				inSet_[ref4.host->self_id()] = true;
			}
			else
			{
				auto& ntf_ = ref4.ntf;
				ref4.complete = false;
				ntf_ = ref4.ath.make_notifer();
				wait_node wn = { ntf_, ref4.host->self_id(), false };
				ref4->_waitQueue.push_front(wn);
			}
		});
		if (!complete)
		{
			ath.wait();
		}
	}

	bool try_lock_upgrade(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		bool complete = false;
		LAMBDA_THIS_REF2(ref2, host, complete);
		host->send(_strand, [&ref2]
		{
			assert(ref2->_shared);
			assert(ref2->_inSet.find(ref2.host->self_id()) != ref2->_inSet.end());
			assert(ref2->_inSet.find(ref2.host->self_id())->second);
			if (ref2->_inSet.size() == 1)
			{
				ref2.complete = true;
				ref2->_inSet[ref2.host->self_id()] = true;
			}
		});
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
			assert(ref5->_shared);
			assert(ref5->_inSet.find(ref5.host->self_id()) != ref5->_inSet.end());
			assert(ref5->_inSet.find(ref5.host->self_id())->second);
			auto& inSet_ = ref5->_inSet;
			if (inSet_.size() == 1)
			{
				ref5.complete = true;
				inSet_[ref5.host->self_id()] = true;
			}
			else
			{
				auto& ntf_ = ref5.ntf;
				auto& waitQueue_ = ref5->_waitQueue;
				ref5.complete = false;
				ntf_ = ref5.ath.make_notifer();
				wait_node wn = { ntf_, ref5.host->self_id(), false };
				waitQueue_.push_front(wn);
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
					return false;
				}
				ath.wait();
			}
		}
		return true;
	}

	void unlock(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		host->send(_strand, [&]
		{
			assert(!_shared);
			auto it = _inSet.find(host->self_id());
			assert(it != _inSet.end());
			assert(!it->second);
			_inSet.erase(it);
			if (!_waitQueue.empty())
			{
				_shared = _waitQueue.back()._isShared;
				_inSet[_waitQueue.back()._waitHostID] = _shared;
				_waitQueue.back().ntf();
				_waitQueue.pop_back();
			}
		});
	}

	void unlock_shared(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		host->send(_strand, [&]
		{
			assert(_shared);
			auto it = _inSet.find(host->self_id());
			assert(it != _inSet.end());
			assert(it->second);
			_inSet.erase(it);
			if (!_waitQueue.empty())
			{
				_shared = _waitQueue.back()._isShared;
				_inSet[_waitQueue.back()._waitHostID] = _shared;
				_waitQueue.back().ntf();
				_waitQueue.pop_back();
			}
		});
	}

	void unlock_upgrade(my_actor* host)
	{
		host->assert_enter();
		my_actor::quit_guard qg(host);
		host->send(_strand, [&]
		{
			assert(_inSet.find(host->self_id()) != _inSet.end());
			assert(_inSet.find(host->self_id())->second);
			assert(_inSet.size() == 1);
			_inSet[host->self_id()] = true;
			if (!_waitQueue.empty())
			{
				_shared = _waitQueue.back()._isShared;
				_inSet[_waitQueue.back()._waitHostID] = _shared;
				_waitQueue.back().ntf();
				_waitQueue.pop_back();
			}
		});
	}
private:
	shared_strand _strand;
	msg_list<wait_node> _waitQueue;
	msg_map<my_actor::id, bool> _inSet;
	bool _shared;
};
//////////////////////////////////////////////////////////////////////////

actor_shared_mutex::actor_shared_mutex(shared_strand strand)
:_amutex(new _actor_shared_mutex(strand))
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

bool actor_shared_mutex::try_lock_shared(my_actor* host)
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