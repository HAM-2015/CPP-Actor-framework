#ifndef __ASYNC_BUFFER_H
#define __ASYNC_BUFFER_H

#include <boost/circular_buffer.hpp>
#include <functional>
#include <memory>
#include "actor_framework.h"

struct async_buffer_close_exception
{
};

/*!
@brief 异步缓冲队列，多写/多读
*/
template <typename T>
class async_buffer
{
	struct push_pck 
	{
		void operator ()(bool closed)
		{
			notified = true;
			ntf(closed);
		}

		bool& notified;
		actor_trig_notifer<bool> ntf;
	};

	struct pop_pck
	{
		void operator ()(bool closed)
		{
			notified = true;
			ntf(closed);
		}

		bool& notified;
		actor_trig_notifer<bool> ntf;
	};
public:
	struct close_exception : public async_buffer_close_exception
	{
	};
public:
	async_buffer(size_t maxLength, const shared_strand& strand)
		:_buffer(maxLength)//, _pushWait(4), _popWait(4)
	{
		_closed = false;
		_strand = strand;
	}
public:
	template <typename TM>
	void push(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		while (true)
		{
			actor_trig_handle<bool> ath;
			bool notified = false;
			bool isFull = false;
			bool closed = false;
			LAMBDA_THIS_REF6(ref6, host, msg, ath, notified, isFull, closed);
			host->send(_strand, [&ref6]
			{
				ref6.closed = ref6->_closed;
				if (!ref6->_closed)
				{
					ref6.isFull = ref6->_buffer.full();
					if (!ref6.isFull)
					{
						auto& _popWait = ref6->_popWait;
						ref6->_buffer.push_back(try_move<TM&&>::move(ref6.msg));
						if (!_popWait.empty())
						{
							_popWait.back()(false);
							_popWait.pop_back();
						}
					}
					else
					{
						push_pck pck = { ref6.notified, ref6.host->make_trig_notifer(ref6.ath) };
						ref6->_pushWait.push_front(pck);
					}
				}
			});
			if (closed)
			{
				qg.unlock();
				throw close_exception();
			}
			if (!isFull)
			{
				return;
			}
			if (host->wait_trig(ath))
			{
				qg.unlock();
				throw close_exception();
			}
		}
	}

	template <typename TM>
	bool try_push(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		bool isFull = false;
		bool closed = false;
		LAMBDA_THIS_REF4(ref4, host, msg, isFull, closed);
		host->send(_strand, [&ref4]
		{
			ref4.closed = ref4->_closed;
			if (!ref4->_closed)
			{
				ref4.isFull = ref4->_buffer.full();
				if (!ref4.isFull)
				{
					auto& _popWait = ref4->_popWait;
					ref4->_buffer.push_back(try_move<TM&&>::move(ref4.msg));
					if (!_popWait.empty())
					{
						_popWait.back()(false);
						_popWait.pop_back();
					}
				}
			}
		});
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return !isFull;
	}

	template <typename TM>
	bool timed_push(int tm, my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		long long utm = (long long)tm * 1000;
		while (true)
		{
			actor_trig_handle<bool> ath;
			msg_list<push_pck>::iterator mit;
			bool notified = false;
			bool isFull = false;
			bool closed = false;
			LAMBDA_THIS_REF7(ref7, notified, isFull, closed, host, msg, ath, mit);
			host->send(_strand, [&ref7]
			{
				ref7.closed = ref7->_closed;
				if (!ref7->_closed)
				{
					ref7.isFull = ref7->_buffer.full();
					if (!ref7.isFull)
					{
						auto& _popWait = ref7->_popWait;
						ref7->_buffer.push_back(try_move<TM&&>::move(ref7.msg));
						if (!_popWait.empty())
						{
							_popWait.back()(false);
							_popWait.pop_back();
						}
					}
					else
					{
						push_pck pck = { ref7.notified, ref7.host->make_trig_notifer(ref7.ath) };
						ref7->_pushWait.push_front(pck);
						ref7.mit = ref7->_pushWait.begin();
					}
				}
			});
			if (!closed)
			{
				if (!isFull)
				{
					return true;
				}
				long long bt = get_tick_us();
				if (!host->timed_wait_trig(utm / 1000, ath, closed))
				{
					host->send(_strand, [&ref7]
					{
						if (!ref7.notified)
						{
							ref7->_pushWait.erase(ref7.mit);
						}
					});
					if (notified)
					{
						closed = host->wait_trig(ath);
					}
				}
				utm -= get_tick_us() - bt;
				if (utm < 1000)
				{
					return false;
				}
			}
			if (closed)
			{
				qg.unlock();
				throw close_exception();
			}
		}
		return false;
	}

	T pop(my_actor* host)
	{
		my_actor::quit_guard qg(host);
		char resBuf[sizeof(T)];
		while (true)
		{
			actor_trig_handle<bool> ath;
			bool notified = false;
			bool isEmpty = false;
			bool closed = false;
			LAMBDA_THIS_REF6(ref6, host, resBuf, ath, notified, isEmpty, closed);
			host->send(_strand, [&ref6]
			{
				ref6.closed = ref6->_closed;
				if (!ref6->_closed)
				{
					auto& _buffer = ref6->_buffer;
					auto& _pushWait = ref6->_pushWait;
					ref6.isEmpty = _buffer.empty();
					if (!ref6.isEmpty)
					{
						new(ref6.resBuf)T(std::move(_buffer.front()));
						_buffer.pop_front();
					}
					else
					{
						pop_pck pck = { ref6.notified, ref6.host->make_trig_notifer(ref6.ath) };
						ref6->_popWait.push_front(pck);
					}
					if (_buffer.size() <= _buffer.capacity() / 2 && !_pushWait.empty())
					{
						_pushWait.back()(false);
						_pushWait.pop_back();
					}
				}
			});
			if (closed)
			{
				qg.unlock();
				throw close_exception();
			}
			if (!isEmpty)
			{
				AUTO_CALL(
				{
					typedef T TP_;
					((TP_*)resBuf)->~TP_();
				});
				return std::move(*(T*)resBuf);
			}
			if (host->wait_trig(ath))
			{
				qg.unlock();
				throw close_exception();
			}
		}
	}

	bool try_pop(my_actor* host, T& out)
	{
		return _try_pop(host, [&](T& s)
		{
			out = std::move(s);
		});
	}

	bool try_pop(my_actor* host, stack_obj<T>& out)
	{
		return _try_pop(host, [&](T& s)
		{
			out.create(std::move(s));
		});
	}

	bool timed_pop(int tm, my_actor* host, T& out)
	{
		return _timed_pop(tm, host, [&](T& s)
		{
			out = std::move(s);
		});
	}

	bool timed_pop(int tm, my_actor* host, stack_obj<T>& out)
	{
		return _timed_pop(tm, host, [&](T& s)
		{
			out.create(std::move(s));
		});
	}

	void close(my_actor* host)
	{
		my_actor::quit_guard qg(host);
		host->send(_strand, [this]
		{
			_closed = true;
			_buffer.clear();
			while (!_pushWait.empty())
			{
				_pushWait.front()(true);
				_pushWait.pop_front();
			}
			while (!_popWait.empty())
			{
				_popWait.front()(true);
				_popWait.pop_front();
			}
		});
	}

	void reset()
	{
		_closed = false;
		_buffer.clear();
		_pushWait.clear();
		_popWait.clear();
	}
private:
	async_buffer(const async_buffer&){};
	void operator=(const async_buffer&){};

	template <typename H>
	bool _try_pop(my_actor* host, H& out)
	{
		my_actor::quit_guard qg(host);
		bool isEmpty = false;
		bool closed = false;
		LAMBDA_THIS_REF4(ref4, host, out, isEmpty, closed);
		host->send(_strand, [&ref4]
		{
			ref4.closed = ref4->_closed;
			if (!ref4->_closed)
			{
				auto& _buffer = ref4->_buffer;
				auto& _pushWait = ref4->_pushWait;
				ref4.isEmpty = _buffer.empty();
				if (!ref4.isEmpty)
				{
					ref4.out(_buffer.front());
					_buffer.pop_front();
				}
				if (_buffer.size() <= _buffer.capacity() / 2 && !_pushWait.empty())
				{
					_pushWait.back()(false);
					_pushWait.pop_back();
				}
			}
		});
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return !isEmpty;
	}

	template <typename H>
	bool _timed_pop(int tm, my_actor* host, H& out)
	{
		my_actor::quit_guard qg(host);
		long long utm = (long long)tm * 1000;
		while (true)
		{
			actor_trig_handle<bool> ath;
			msg_list<pop_pck>::iterator mit;
			bool notified = false;
			bool isEmpty = false;
			bool closed = false;
			LAMBDA_THIS_REF7(ref7, notified, isEmpty, closed, host, out, ath, mit);
			host->send(_strand, [&ref7]
			{
				ref7.closed = ref7->_closed;
				if (!ref7->_closed)
				{
					auto& _buffer = ref7->_buffer;
					auto& _pushWait = ref7->_pushWait;
					ref7.isEmpty = _buffer.empty();
					if (!ref7.isEmpty)
					{
						ref7.out(_buffer.front());
						_buffer.pop_front();
					}
					else
					{
						pop_pck pck = { ref7.notified, ref7.host->make_trig_notifer(ref7.ath) };
						ref7->_popWait.push_front(pck);
						ref7.mit = ref7->_popWait.begin();
					}
					if (_buffer.size() <= _buffer.capacity() / 2 && !_pushWait.empty())
					{
						_pushWait.back()(false);
						_pushWait.pop_back();
					}
				}
			});
			if (!closed)
			{
				if (!isEmpty)
				{
					return true;
				}
				long long bt = get_tick_us();
				if (!host->timed_wait_trig(utm / 1000, ath, closed))
				{
					host->send(_strand, [&ref7]
					{
						if (!ref7.notified)
						{
							ref7->_popWait.erase(ref7.mit);
						}
					});
					if (notified)
					{
						closed = host->wait_trig(ath);
					}
				}
				utm -= get_tick_us() - bt;
				if (utm < 1000)
				{
					return false;
				}
			}
			if (closed)
			{
				qg.unlock();
				throw close_exception();
			}
		}
		return false;
	}
private:
	bool _closed;
	shared_strand _strand;
	boost::circular_buffer<T> _buffer;
	msg_list<push_pck> _pushWait;
	msg_list<pop_pck> _popWait;
};

#endif