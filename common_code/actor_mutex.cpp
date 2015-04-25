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
	void operator()() const;
public:
	mutex_trig_handle* _trigHandle;
	shared_strand _strand;
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
public:
	bool _hasMsg;
	bool _waiting;
	bool _outActor;
	shared_strand _strand;
	my_actor* _hostActor;
};

//////////////////////////////////////////////////////////////////////////

void mutex_trig_notifer::operator()() const
{
	auto& trigHandle_ = _trigHandle;
	_strand->post([=]()
	{
		trigHandle_->push_msg();
	});
}

mutex_trig_notifer::mutex_trig_notifer(mutex_trig_handle* trigHandle)
:_trigHandle(trigHandle),
_strand(trigHandle->_strand)
{

}

mutex_trig_notifer::mutex_trig_notifer() :_trigHandle(NULL)
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
	struct check_quit
	{
		check_quit() :_sign(true){}

		~check_quit()
		{
			assert(!_sign);
		}

		void reset()
		{
			_sign = false;
		}

		bool _sign;
	};

	struct wait_node
	{
		mutex_trig_notifer ntf;
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
		std::shared_ptr<_actor_mutex> res(new _actor_mutex(strand));
		res->_weakThis = res;
		return res;
	}
public:
	void lock(my_actor* self)
	{
		assert(self->self_strand()->running_in_this_thread());
		assert(self->in_actor());
		self->check_stack();

		DEBUG_OPERATION(check_quit cq);

		mutex_trig_handle ath(self->shared_from_this());
		auto shared_this = _weakThis.lock();
		auto h = [shared_this, self, &ath]()->bool
		{
			auto& lockActor_ = shared_this->_lockActor;
			if (!lockActor_ || self == lockActor_)
			{
				lockActor_ = self;
				shared_this->_recCount++;
				return true;
			}
			wait_node wn = { ath.make_notifer(), self };
			shared_this->_waitQueue.push_back(wn);
			return false;
		};

		bool complete;
		if (_strand != self->_strand)
		{
			if (self->is_quited())
			{
				actor_handle shared_this = self->shared_from_this();
				_strand->asyncInvoke(h, [shared_this, &complete](bool b)
				{
					complete = b;
					auto& shared_this_ = shared_this;
					shared_this->_strand->post([=]()
					{
						shared_this_->pull_yield_as_mutex();
					});
				});
				self->push_yield_as_mutex();
			} 
			else
			{
				complete = self->send<bool>(_strand, h);
			}
		}
		else
		{
			complete = h();
		}

		if (!complete)
		{
			ath.wait(self);
			assert(_lockActor == self && 1 == _recCount);
		}
		DEBUG_OPERATION(cq.reset());
	}

	bool try_lock(my_actor* self)
	{
		self->assert_enter();

		DEBUG_OPERATION(check_quit cq);
		auto shared_this = _weakThis.lock();
		bool complete = self->send<bool>(_strand, [shared_this, self]()->bool
		{
			auto& lockActor_ = shared_this->_lockActor;
			if (!lockActor_ || self == lockActor_)
			{
				lockActor_ = self;
				shared_this->_recCount++;
				return true;
			}
			return false;
		});
		DEBUG_OPERATION(cq.reset());
		return complete;
	}

	void unlock(my_actor* self)
	{
		assert(self->self_strand()->running_in_this_thread());
		assert(self->in_actor());
		self->check_stack();

		DEBUG_OPERATION(check_quit cq);

		auto shared_this = _weakThis.lock();
		auto h = [shared_this, self]()
		{
			auto& recCount_ = shared_this->_recCount;
			auto& lockActor_ = shared_this->_lockActor;
			auto& waitQueue_ = shared_this->_waitQueue;
			assert(lockActor_ == self);
			if (0 == --recCount_)
			{
				if (!waitQueue_.empty())
				{
					recCount_ = 1;
					lockActor_ = waitQueue_.front()._waitSelf;
					waitQueue_.front().ntf();
					waitQueue_.pop_front();
				}
				else
				{
					lockActor_ = NULL;
				}
			}
		};

		if (_strand != self->self_strand())
		{
			if (self->is_quited())
			{
				actor_handle shared_this = self->shared_from_this();
				_strand->asyncInvokeVoid(h, [shared_this]()
				{
					auto& shared_this_ = shared_this;
					shared_this->_strand->post([=]()
					{
						shared_this_->pull_yield_as_mutex();
					});
				});
				self->push_yield_as_mutex();
			} 
			else
			{
				self->send(_strand, h);
			}
		} 
		else
		{
			h();
		}

		DEBUG_OPERATION(cq.reset());
	}

	void reset_mutex(my_actor* self)
	{
		DEBUG_OPERATION(check_quit cq);
		auto shared_this = _weakThis.lock();
		self->send(_strand, [shared_this]()
		{
			shared_this->_lockActor = NULL;
			shared_this->_recCount = 0;
			auto& waitQueue_ = shared_this->_waitQueue;
			while (!waitQueue_.empty())
			{
				waitQueue_.front().ntf();
				waitQueue_.pop_front();
			}
		});
		DEBUG_OPERATION(cq.reset());
	}
private:
	shared_strand _strand;
	my_actor* _lockActor;
	size_t _recCount;
	std::weak_ptr<_actor_mutex> _weakThis;
	msg_queue<wait_node> _waitQueue;
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

void actor_mutex::unlock(my_actor* self) const
{
	_amutex->unlock(self);
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
