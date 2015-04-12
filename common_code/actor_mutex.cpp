#include "actor_mutex.h"
#include "msg_queue.h"
#include "actor_framework.h"

class _actor_mutex
{
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
		self->assert_enter();
		actor_trig_handle<> ath;
		auto shared_this = _weakThis.lock();
		bool complete = self->send<bool>(_strand, [shared_this, self, &ath]()->bool
		{
			auto& lockActor_ = shared_this->_lockActor;
			if (!lockActor_ || self == lockActor_)
			{
				lockActor_ = self;
				shared_this->_recCount++;
				return true;
			}
			shared_this->_waitQueue.push_back(self->make_trig_notifer(ath));
			return false;
		});
		if (!complete)
		{
			self->wait_trig(ath);
			assert(_lockActor == self && 1 == _recCount);
		}
	}

	bool try_lock(my_actor* self)
	{
		self->assert_enter();
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
		return complete;
	}

	void unlock(my_actor* self)
	{
		self->assert_enter();
		auto shared_this = _weakThis.lock();
		self->send(_strand, [shared_this, self]()
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
					lockActor_ = waitQueue_.front().host_actor().get();
					waitQueue_.front()();
					waitQueue_.pop_front();
				}
				else
				{
					lockActor_ = NULL;
				}
			}
		});
	}

	void reset_mutex(my_actor* self)
	{
		auto shared_this = _weakThis.lock();
		self->send(_strand, [shared_this]()
		{
			shared_this->_lockActor = NULL;
			shared_this->_recCount = 0;
			auto& waitQueue_ = shared_this->_waitQueue;
			while (!waitQueue_.empty())
			{
				waitQueue_.front()();
				waitQueue_.pop_front();
			}
		});
	}
private:
	shared_strand _strand;
	my_actor* _lockActor;
	size_t _recCount;
	std::weak_ptr<_actor_mutex> _weakThis;
	msg_queue<actor_trig_notifer<> > _waitQueue;
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

void actor_mutex::reset_mutex(my_actor* self)
{
	_amutex->reset_mutex(self);
}
//////////////////////////////////////////////////////////////////////////

actor_lock_guard::actor_lock_guard(const actor_mutex& amutex, my_actor* self)
:_amutex(amutex), _self(self)
{
	_amutex.lock(_self);
}

actor_lock_guard::actor_lock_guard(const actor_lock_guard&)
: _amutex(_amutex), _self(0)
{

}

actor_lock_guard::~actor_lock_guard()
{
	if (!_self->is_quited())
	{
		_amutex.unlock(_self);
	}
}

void actor_lock_guard::operator=(const actor_lock_guard&)
{

}
