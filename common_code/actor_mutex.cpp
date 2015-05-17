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
	mutex_trig_handle(const actor_handle& hostActor);
	~mutex_trig_handle();
public:
	mutex_trig_notifer make_notifer();
	void push_msg();
	bool read_msg();
	void wait(my_actor* self);
	bool timed_wait(int tm, my_actor* self);
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

void mutex_trig_handle::wait(my_actor* self)
{
	if (!read_msg())
	{
		if (!_outActor)
		{
			self->push_yield();
		} 
		else
		{
			self->push_yield_as_mutex();
		}
	}
}

bool mutex_trig_handle::timed_wait(int tm, my_actor* self)
{
	assert(!_outActor);
	if (!read_msg())
	{
		bool timeout = false;
		LAMBDA_REF2(ref2, self, timeout);
		if (tm >= 0)
		{
			self->delay_trig(tm, [&ref2]
			{
				ref2.timeout = true;
				ref2.self->pull_yield();
			});
		}
		self->push_yield();
		if (!timeout)
		{
			if (tm >= 0)
			{
				self->cancel_delay_trig();
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
	_waiting = false;
	_hasMsg = false;
	return mutex_trig_notifer(this);
}

mutex_trig_handle::~mutex_trig_handle()
{

}

mutex_trig_handle::mutex_trig_handle(const actor_handle& hostActor)
:_hasMsg(false), _outActor(hostActor->is_quited())
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
		my_actor* _waitSelf;
	};
private:
	_actor_mutex(shared_strand strand)
		:_strand(strand), _lockActor(NULL), _recCount(0), _waitQueue(4)
	{

	}
public:
	~_actor_mutex()
	{
		assert(!_lockActor);
	}

	static std::shared_ptr<_actor_mutex> make(shared_strand strand)
	{
		return std::shared_ptr<_actor_mutex>(new _actor_mutex(strand));
	}
public:
	void lock(my_actor* self)
	{
		self->assert_enter();

		my_actor::quit_guard qg(self);
		bool complete;
		mutex_trig_handle ath(self->shared_from_this());
		mutex_trig_notifer ntf;
		LAMBDA_THIS_REF4(ref4, self, complete, ath, ntf);
		self->send(_strand, [&ref4]
		{
			if (!ref4->_lockActor || ref4.self == ref4->_lockActor)
			{
				ref4->_lockActor = ref4.self;
				ref4->_recCount++;
				ref4.complete = true;
			}
			else
			{
				ref4.ntf = ref4.ath.make_notifer();
				wait_node wn = { ref4.ntf, ref4.self };
				ref4->_waitQueue.push_back(wn);
				ref4.complete = false;
			}
		});

		if (!complete)
		{
			ath.wait(self);
			assert(ntf._notified && _lockActor == self && 1 == _recCount);
		}
	}

	void quited_lock(my_actor* self)
	{
		assert(self->self_strand()->running_in_this_thread());
		assert(self->in_actor());
		assert(self->is_quited());
		self->check_stack();
		bool complete;
		mutex_trig_handle ath(self->shared_from_this());
		mutex_trig_notifer ntf;
		LAMBDA_THIS_REF4(ref4, self, complete, ath, ntf);
		auto h = [&ref4]
		{
			if (!ref4->_lockActor || ref4.self == ref4->_lockActor)
			{
				ref4->_lockActor = ref4.self;
				ref4->_recCount++;
				ref4.complete = true;
			}
			else
			{
				ref4.ntf = ref4.ath.make_notifer();
				wait_node wn = { ref4.ntf, ref4.self };
				ref4->_waitQueue.push_back(wn);
				ref4.complete = false;
			}
		};

		if (_strand != self->self_strand())
		{
			actor_handle shared_self = self->shared_from_this();
			_strand->asyncInvokeVoid(h, [shared_self]
			{
				auto& shared_self_ = shared_self;
				shared_self->self_strand()->post([=]
				{
					shared_self_->pull_yield_as_mutex();
				});
			});
			self->push_yield_as_mutex();
		}
		else
		{
			h();
		}

		if (!complete)
		{
			ath.wait(self);
			assert(ntf._notified && _lockActor == self && 1 == _recCount);
		}
	}

	bool try_lock(my_actor* self)
	{
		self->assert_enter();

		my_actor::quit_guard qg(self);
		bool complete;
		LAMBDA_THIS_REF2(ref2, self, complete);
		self->send(_strand, [&ref2]
		{
			if (!ref2->_lockActor || ref2.self == ref2->_lockActor)
			{
				ref2->_lockActor = ref2.self;
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

	bool timed_lock(int tm, my_actor* self)
	{
		self->assert_enter();

		my_actor::quit_guard qg(self);
		msg_list<wait_node>::node_it nit;
		bool complete;
		mutex_trig_handle ath(self->shared_from_this());
		mutex_trig_notifer ntf;
		LAMBDA_THIS_REF5(ref5, self, nit, complete, ath, ntf);
		self->send(_strand, [&ref5]
		{
			if (!ref5->_lockActor || ref5.self == ref5->_lockActor)
			{
				ref5->_lockActor = ref5.self;
				ref5->_recCount++;
				ref5.complete = true;
			}
			else
			{
				ref5.ntf = ref5.ath.make_notifer();
				wait_node wn = { ref5.ntf, ref5.self };
				ref5.nit = ref5->_waitQueue.push_back(wn);
				ref5.complete = false;
			}
		});

		if (!complete)
		{
			if (!ath.timed_wait(tm, self))
			{
				self->send(_strand, [&ref5]
				{
					if (!ref5.ntf._notified)
					{
						ref5.ntf._notified = true;
						ref5->_waitQueue.erase(ref5.nit);
					}
				});
				return false;
			}
			assert(ntf._notified && _lockActor == self && 1 == _recCount);
		}
		return true;
	}

	void unlock(my_actor* self)
	{
		self->assert_enter();

		my_actor::quit_guard qg(self);
		self->send(_strand, [&]
		{
			assert(_lockActor == self);
			if (0 == --_recCount)
			{
				if (!_waitQueue.empty())
				{
					_recCount = 1;
					_lockActor = _waitQueue.front()._waitSelf;
					_waitQueue.front().ntf();
					_waitQueue.pop_front();
				}
				else
				{
					_lockActor = NULL;
				}
			}
		});
	}
	
	void quited_unlock(my_actor* self)
	{
		assert(self->self_strand()->running_in_this_thread());
		assert(self->in_actor());
		assert(self->is_quited());
		self->check_stack();
		auto h = [&]
		{
			assert(_lockActor == self);
			if (0 == --_recCount)
			{
				if (!_waitQueue.empty())
				{
					_recCount = 1;
					_lockActor = _waitQueue.front()._waitSelf;
					_waitQueue.front().ntf();
					_waitQueue.pop_front();
				}
				else
				{
					_lockActor = NULL;
				}
			}
		};

		if (_strand != self->self_strand())
		{
			actor_handle shared_self = self->shared_from_this();
			_strand->asyncInvokeVoid(h, [shared_self]
			{
				auto& shared_self_ = shared_self;
				shared_self->self_strand()->post([=]
				{
					shared_self_->pull_yield_as_mutex();
				});
			});
			self->push_yield_as_mutex();
		}
		else
		{
			h();
		}
	}

	void unlock()
	{
		assert(_lockActor);
		_lockActor->check_self();
		unlock(_lockActor);
	}

	void reset_mutex(my_actor* self)
	{
		self->assert_enter();

		my_actor::quit_guard qg(self);
		self->send(_strand, [this]
		{
			_lockActor = NULL;
			_recCount = 0;
			auto& waitQueue_ = _waitQueue;
			while (!waitQueue_.empty())
			{
				waitQueue_.front().ntf();
				waitQueue_.pop_front();
			}
		});
	}
private:
	shared_strand _strand;
	my_actor* _lockActor;
	size_t _recCount;
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

void actor_mutex::lock(my_actor* self) const
{
	_amutex->lock(self);
}

bool actor_mutex::try_lock(my_actor* self) const
{
	return _amutex->try_lock(self);
}

bool actor_mutex::timed_lock(int tm, my_actor* self) const
{
	return _amutex->timed_lock(tm, self);
}

void actor_mutex::unlock(my_actor* self) const
{
	_amutex->unlock(self);
}

void actor_mutex::unlock() const
{
	_amutex->unlock();
}

void actor_mutex::quited_lock(my_actor* self) const
{
	_amutex->quited_lock(self);
}

void actor_mutex::quited_unlock(my_actor* self) const
{
	_amutex->quited_unlock(self);
}

// void actor_mutex::reset_mutex(my_actor* self)
// {
// 	_amutex->reset_mutex(self);
// }
//////////////////////////////////////////////////////////////////////////

actor_lock_guard::actor_lock_guard(const actor_mutex& amutex, my_actor* self)
:_amutex(amutex), _self(self), _isUnlock(false)
{
	_amutex.lock(_self);
}

actor_lock_guard::actor_lock_guard(const actor_lock_guard&)
: _amutex(_amutex), _self(0)
{

}

actor_lock_guard::~actor_lock_guard()
{
	assert(!_self->is_quited());
	if (!_isUnlock)
	{
		_amutex.unlock(_self);
	}
}

void actor_lock_guard::operator=(const actor_lock_guard&)
{

}

void actor_lock_guard::unlock()
{
	_isUnlock = true;
	_amutex.unlock(_self);
}

void actor_lock_guard::lock()
{
	_isUnlock = false;
	_amutex.lock(_self);
}
