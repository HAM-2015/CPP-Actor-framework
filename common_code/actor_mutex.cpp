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
	void operator()(bool closed);
public:
	mutex_trig_handle* _trigHandle;
	shared_strand _strand;
	bool _notified;
};
//////////////////////////////////////////////////////////////////////////

class mutex_trig_handle
{
public:
	mutex_trig_handle(const actor_handle& hostActor);
	~mutex_trig_handle();
public:
	mutex_trig_notifer make_notifer();
	void push_msg(bool closed);
	bool read_msg();
	void wait(my_actor* host);
	bool timed_wait(int tm, my_actor* host);
public:
	bool _hasMsg;
	bool _waiting;
	bool _closed;
	bool _outActor;
	shared_strand _strand;
	my_actor* _hostActor;
};

//////////////////////////////////////////////////////////////////////////

void mutex_trig_notifer::operator()(bool closed)
{
	assert(!_notified);
	auto& trigHandle_ = _trigHandle;
	_notified = true;
	_strand->post([=]
	{
		trigHandle_->push_msg(closed);
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

void mutex_trig_handle::wait(my_actor* host)
{
	if (!read_msg())
	{
		if (!_outActor)
		{
			host->push_yield();
		} 
		else
		{
			host->push_yield_as_mutex();
		}
	}
}

bool mutex_trig_handle::timed_wait(int tm, my_actor* host)
{
	assert(!_outActor);
	if (!read_msg())
	{
		bool timeout = false;
		LAMBDA_REF2(ref2, host, timeout);
		if (tm >= 0)
		{
			host->delay_trig(tm, [&ref2]
			{
				ref2.timeout = true;
				ref2.host->pull_yield();
			});
		}
		host->push_yield();
		if (!timeout)
		{
			if (tm >= 0)
			{
				host->cancel_delay_trig();
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

void mutex_trig_handle::push_msg(bool closed)
{
	assert(_strand->running_in_this_thread());
	_closed = closed;
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

mutex_trig_handle::mutex_trig_handle(const actor_handle& hostActor)
:_hasMsg(false), _waiting(false), _closed(false), _outActor(hostActor->is_quited())
{
	_hostActor = hostActor.get();
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
private:
	_actor_mutex(shared_strand strand)
		:_strand(strand), _lockActorID(0), _recCount(0), _closed(false), _waitQueue(4)
	{

	}
public:
	~_actor_mutex()
	{
		assert(!_lockActorID);
	}

	static std::shared_ptr<_actor_mutex> make(shared_strand strand)
	{
		return std::shared_ptr<_actor_mutex>(new _actor_mutex(strand));
	}
public:
	void lock(my_actor* host)
	{
		host->assert_enter();

		my_actor::quit_guard qg(host);
		bool complete;
		mutex_trig_handle ath(host->shared_from_this());
		mutex_trig_notifer ntf;
		LAMBDA_THIS_REF4(ref4, host, complete, ath, ntf);
		host->send(_strand, [&ref4]
		{
			if (ref4->_closed)
			{
				ref4.complete = true;
				ref4.ath._closed = true;
			}
			else
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
					ref4->_waitQueue.push_back(wn);
					ref4.complete = false;
				}
			}
		});

		if (!complete)
		{
			ath.wait(host);
		}
		if (ath._closed)
		{
			qg.unlock();
			throw actor_mutex::close_exception();
		}
	}

	void quited_lock(my_actor* host)
	{
		assert(host->self_strand()->running_in_this_thread());
		assert(host->in_actor());
		assert(host->is_quited());
		host->check_stack();
		bool complete;
		mutex_trig_handle ath(host->shared_from_this());
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
			ath.wait(host);
		}
	}

	bool try_lock(my_actor* host)
	{
		host->assert_enter();

		my_actor::quit_guard qg(host);
		bool complete;
		bool closed = false;
		LAMBDA_THIS_REF3(ref3, host, complete, closed);
		host->send(_strand, [&ref3]
		{
			if (ref3->_closed)
			{
				ref3.closed = true;
			}
			else
			{
				auto& lockActorID_ = ref3->_lockActorID;
				auto& host_ = ref3.host;
				if (!lockActorID_ || host_->self_id() == lockActorID_)
				{
					lockActorID_ = host_->self_id();
					ref3->_recCount++;
					ref3.complete = true;
				}
				else
				{
					ref3.complete = false;
				}
			}
		});
		if (closed)
		{
			qg.unlock();
			throw actor_mutex::close_exception();
		}
		return complete;
	}

	bool timed_lock(int tm, my_actor* host)
	{
		host->assert_enter();

		my_actor::quit_guard qg(host);
		msg_list<wait_node>::node_it nit;
		bool complete;
		mutex_trig_handle ath(host->shared_from_this());
		mutex_trig_notifer ntf;
		LAMBDA_THIS_REF5(ref5, host, nit, complete, ath, ntf);
		host->send(_strand, [&ref5]
		{
			if (ref5->_closed)
			{
				ref5.complete = true;
				ref5.ath._closed = true;
			}
			else
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
					ref5.nit = ref5->_waitQueue.push_back(wn);
					ref5.complete = false;
				}
			}
		});

		if (!complete)
		{
			if (!ath.timed_wait(tm, host))
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
				ath.wait(host);
			}
		}
		if (ath._closed)
		{
			qg.unlock();
			throw actor_mutex::close_exception();
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
					_lockActorID = _waitQueue.front()._waitHostID;
					_waitQueue.front().ntf(false);
					_waitQueue.pop_front();
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
					_lockActorID = _waitQueue.front()._waitHostID;
					_waitQueue.front().ntf(false);
					_waitQueue.pop_front();
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

	void close(my_actor* host)
	{
		host->assert_enter();

		my_actor::quit_guard qg(host);
		host->send(_strand, [this]
		{
			_lockActorID = 0;
			_recCount = 0;
			_closed = true;
			while (!_waitQueue.empty())
			{
				_waitQueue.front().ntf(true);
				_waitQueue.pop_front();
			}
		});
	}

	void reset()
	{
		_lockActorID = 0;
		_recCount = 0;
		_closed = false;
		_waitQueue.clear();
	}
private:
	shared_strand _strand;
	my_actor::id _lockActorID;
	size_t _recCount;
	bool _closed;
	msg_list<wait_node> _waitQueue;
};
//////////////////////////////////////////////////////////////////////////

actor_mutex::actor_mutex(shared_strand strand)
:_amutex(_actor_mutex::make(strand))
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

void actor_mutex::close(my_actor* host) const
{
	_amutex->close(host);
}

void actor_mutex::reset() const
{
	_amutex->reset();
}
//////////////////////////////////////////////////////////////////////////

actor_lock_guard::actor_lock_guard(const actor_mutex& amutex, my_actor* host)
:_amutex(amutex), _host(host), _isUnlock(false)
{
	_amutex.lock(_host);
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
