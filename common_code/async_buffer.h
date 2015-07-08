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
public:
	struct close_exception : public async_buffer_close_exception
	{
	};
public:
	async_buffer(size_t maxLength, shared_strand strand)
		:_buffer(maxLength), _pushWait(4), _popWait(4)
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
			bool isFull = false;
			bool closed = false;
			host->send(_strand, [&]
			{
				closed = _closed;
				if (!_closed)
				{
					isFull = _buffer.full();
					if (!isFull)
					{
						_buffer.push_back(try_move<TM&&>::move(msg));
						if (!_popWait.empty())
						{
							_popWait.front()(true);
							_popWait.pop_front();
						}
					}
					else
					{
						_pushWait.push_back(host->make_trig_notifer(ath));
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
			if (!host->wait_trig(ath))
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
		host->send(_strand, [&]
		{
			closed = _closed;
			if (!_closed)
			{
				isFull = _buffer.full();
				if (!isFull)
				{
					_buffer.push_back(try_move<TM&&>::move(msg));
					if (!_popWait.empty())
					{
						_popWait.front()(true);
						_popWait.pop_front();
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

	T pop(my_actor* host)
	{
		my_actor::quit_guard qg(host);
		char resBuf[sizeof(T)];
		while (true)
		{
			actor_trig_handle<bool> ath;
			bool isEmpty = false;
			bool closed = false;
			host->send(_strand, [&]
			{
				closed = _closed;
				if (!_closed)
				{
					isEmpty = _buffer.empty();
					if (!isEmpty)
					{
						new(resBuf)T(std::move(_buffer.front()));
						_buffer.pop_front();
					}
					else
					{
						_popWait.push_back(host->make_trig_notifer(ath));
					}
					if (_buffer.size() <= _buffer.capacity() / 2 && !_pushWait.empty())
					{
						_pushWait.front()(true);
						_pushWait.pop_front();
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
			if (!host->wait_trig(ath))
			{
				qg.unlock();
				throw close_exception();
			}
		}
	}

	bool try_pop(my_actor* host, T& out)
	{
		my_actor::quit_guard qg(host);
		bool isEmpty = false;
		bool closed = false;
		host->send(_strand, [&]
		{
			closed = _closed;
			if (!_closed)
			{
				isEmpty = _buffer.empty();
				if (!isEmpty)
				{
					out = std::move(_buffer.front());
					_buffer.pop_front();
				}
				if (_buffer.size() <= _buffer.capacity() / 2 && !_pushWait.empty())
				{
					_pushWait.front()(true);
					_pushWait.pop_front();
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

	void close(my_actor* host)
	{
		my_actor::quit_guard qg(host);
		host->send(_strand, [&]
		{
			_closed = true;
			_buffer.clear();
			while (!_pushWait.empty())
			{
				_pushWait.front()(false);
				_pushWait.pop_front();
			}
			while (!_popWait.empty())
			{
				_popWait.front()(false);
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
private:
	bool _closed;
	shared_strand _strand;
	boost::circular_buffer<T> _buffer;
	msg_queue<actor_trig_notifer<bool>> _pushWait;
	msg_queue<actor_trig_notifer<bool>> _popWait;
};

#endif