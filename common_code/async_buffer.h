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

	template <size_t N, size_t DEPTH = 0, bool TOP = false>
	struct buff_push
	{
		template <typename Fst, typename... Args>
		static void push(async_buffer* const this_, const size_t& i, Fst&& fst, Args&&... args)
		{
			if (DEPTH == i)
			{
				this_->_buffer.push_back(try_move<Fst&&>::move(fst));
			}
			else
			{
				buff_push<N, DEPTH + 1, N == DEPTH + 1>::push(this_, i, TRY_MOVE(args)...);
			}
		}
	};

	template <size_t N, size_t DEPTH>
	struct buff_push<N, DEPTH, true>
	{
		static void push(async_buffer* const, const size_t&){}
	};

	template <size_t N, size_t DEPTH = 0, bool TOP = false>
	struct buff_pop
	{
		template <typename Fst, typename... Outs>
		static void pop(async_buffer* const this_, const size_t& i, Fst& fst, Outs&... outs)
		{
			if (DEPTH == i)
			{
				fst = std::move(this_->_buffer.front());
			}
			else
			{
				buff_pop<N, DEPTH + 1, N == DEPTH + 1>::pop(this_, i, outs...);
			}
		}
	};

	template <size_t N, size_t DEPTH>
	struct buff_pop<N, DEPTH, true>
	{
		static void pop(async_buffer* const, const size_t&){}
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
	template <typename...  TMS>
	void push(my_actor* host, TMS&&... msgs)
	{
		static_assert(sizeof...(TMS) > 0, "");
		size_t pushCount = 0;
		_push(host, [&]
		{
			buff_push<sizeof...(TMS)>::push(this, pushCount, try_move<TMS&&>::move(msgs)...);
			return sizeof...(TMS) == ++pushCount;
		});
	}

	template <typename TM>
	bool try_push(my_actor* host, TM&& msg)
	{
		return !!_try_push(host, [&]
		{
			_buffer.push_back(try_move<TM&&>::move(msg));
			return true;
		});
	}

	template <typename...  TMS>
	size_t try_push(my_actor* host, TMS&&... msgs)
	{
		static_assert(sizeof...(TMS) > 1, "");
		size_t pushCount = 0;
		return _try_push(host, [&]
		{
			buff_push<sizeof...(TMS)>::push(this, pushCount, try_move<TMS&&>::move(msgs)...);
			return sizeof...(TMS) == ++pushCount;
		});
	}

	template <typename TM>
	bool timed_push(int tm, my_actor* host, TM&& msg)
	{
		return !!_timed_push(tm, host, [&]
		{
			_buffer.push_back(try_move<TM&&>::move(msg));
			return true;
		});
	}

	template <typename...  TMS>
	size_t timed_push(int tm, my_actor* host, TMS&&... msgs)
	{
		static_assert(sizeof...(TMS) > 1, "");
		size_t pushCount = 0;
		return _timed_push(tm, host, [&]
		{
			buff_push<sizeof...(TMS)>::push(this, pushCount, try_move<TMS&&>::move(msgs)...);
			return sizeof...(TMS) == ++pushCount;
		});
	}

	T pop(my_actor* host)
	{
		char resBuf[sizeof(T)];
		_pop(host, [&]
		{
			new(resBuf)T(std::move(_buffer.front()));
			_buffer.pop_front();
			return true;
		});
		AUTO_CALL(
		{
			typedef T TP_;
			((TP_*)resBuf)->~TP_();
		});
		return std::move(*(T*)resBuf);
	}

	template <typename... OTMS>
	void pop(my_actor* host, OTMS&... outs)
	{
		static_assert(sizeof...(OTMS) > 0, "");
		size_t popCount = 0;
		_pop(host, [&]
		{
			buff_pop<sizeof...(OTMS)>::pop(this, popCount, outs...);
			_buffer.pop_front();
			return sizeof...(OTMS) == ++popCount;
		});
	}

	template <typename OTM>
	bool try_pop(my_actor* host, OTM& out)
	{
		size_t popCount = 0;
		return !!_try_pop(host, [&]
		{
			out = std::move(_buffer.front());
			_buffer.pop_front();
			return true;
		});
	}

	template <typename... OTMS>
	size_t try_pop(my_actor* host, OTMS&... outs)
	{
		static_assert(sizeof...(OTMS) > 1, "");
		size_t popCount = 0;
		return _try_pop(host, [&]
		{
			buff_pop<sizeof...(OTMS)>::pop(this, popCount, outs...);
			_buffer.pop_front();
			return sizeof...(OTMS) == ++popCount;
		});
	}

	template <typename OTM>
	bool timed_pop(int tm, my_actor* host, OTM& out)
	{
		return !!_timed_pop(tm, host, [&]
		{
			out = std::move(_buffer.front());
			_buffer.pop_front();
			return true;
		});
	}

	template <typename... OTMS>
	size_t timed_pop(int tm, my_actor* host, OTMS&... outs)
	{
		static_assert(sizeof...(OTMS) > 1, "");
		size_t popCount = 0;
		return _timed_pop(tm, host, [&]
		{
			buff_pop<sizeof...(OTMS)>::pop(this, popCount, outs...);
			_buffer.pop_front();
			return sizeof...(OTMS) == ++popCount;
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
	void _push(my_actor* host, H& h)
	{
		my_actor::quit_guard qg(host);
		while (true)
		{
			actor_trig_handle<bool> ath;
			bool notified = false;
			bool isFull = false;
			bool closed = false;
			LAMBDA_THIS_REF6(ref6, host, h, ath, notified, isFull, closed);
			host->send(_strand, [&ref6]
			{
				ref6.closed = ref6->_closed;
				if (!ref6->_closed)
				{
					while (true)
					{
						bool break_ = true;
						ref6.isFull = ref6->_buffer.full();
						if (!ref6.isFull)
						{
							break_ = ref6.h();
							auto& _popWait = ref6->_popWait;
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
						if (break_)
						{
							break;
						}
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

	template <typename H>
	size_t _try_push(my_actor* host, H& h)
	{
		my_actor::quit_guard qg(host);
		bool isFull = false;
		bool closed = false;
		size_t pushCount = 0;
		LAMBDA_THIS_REF5(ref5, host, h, isFull, closed, pushCount);
		host->send(_strand, [&ref5]
		{
			ref5.closed = ref5->_closed;
			if (!ref5->_closed)
			{
				while (true)
				{
					bool break_ = true;
					ref5.isFull = ref5->_buffer.full();
					if (!ref5.isFull)
					{
						ref5.pushCount++;
						break_ = ref5.h();
						auto& _popWait = ref5->_popWait;
						if (!_popWait.empty())
						{
							_popWait.back()(false);
							_popWait.pop_back();
						}
					}
					if (break_)
					{
						break;
					}
				}
			}
		});
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return pushCount;
	}

	template <typename H>
	size_t _timed_push(int tm, my_actor* host, H& h)
	{
		my_actor::quit_guard qg(host);
		long long utm = (long long)tm * 1000;
		size_t pushCount = 0;
		while (true)
		{
			actor_trig_handle<bool> ath;
			msg_list<push_pck>::iterator mit;
			bool notified = false;
			bool isFull = false;
			bool closed = false;
			LAMBDA_THIS_REF8(ref8, notified, isFull, closed, host, h, ath, mit, pushCount);
			host->send(_strand, [&ref8]
			{
				ref8.closed = ref8->_closed;
				if (!ref8->_closed)
				{
					while (true)
					{
						bool break_ = true;
						ref8.isFull = ref8->_buffer.full();
						if (!ref8.isFull)
						{
							ref8.pushCount++;
							break_ = ref8.h();
							auto& _popWait = ref8->_popWait;
							if (!_popWait.empty())
							{
								_popWait.back()(false);
								_popWait.pop_back();
							}
						}
						else
						{
							push_pck pck = { ref8.notified, ref8.host->make_trig_notifer(ref8.ath) };
							ref8->_pushWait.push_front(pck);
							ref8.mit = ref8->_pushWait.begin();
						}
						if (break_)
						{
							break;
						}
					}
				}
			});
			if (!closed)
			{
				if (!isFull)
				{
					return pushCount;
				}
				long long bt = get_tick_us();
				if (!host->timed_wait_trig(utm / 1000, ath, closed))
				{
					host->send(_strand, [&ref8]
					{
						if (!ref8.notified)
						{
							ref8->_pushWait.erase(ref8.mit);
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
					return pushCount;
				}
			}
			if (closed)
			{
				qg.unlock();
				throw close_exception();
			}
		}
		return pushCount;
	}

	template <typename H>
	void _pop(my_actor* host, H& out)
	{
		my_actor::quit_guard qg(host);
		while (true)
		{
			actor_trig_handle<bool> ath;
			bool notified = false;
			bool isEmpty = false;
			bool closed = false;
			LAMBDA_THIS_REF6(ref6, host, out, ath, notified, isEmpty, closed);
			host->send(_strand, [&ref6]
			{
				ref6.closed = ref6->_closed;
				if (!ref6->_closed)
				{
					while (true)
					{
						bool break_ = true;
						auto& _buffer = ref6->_buffer;
						auto& _pushWait = ref6->_pushWait;
						ref6.isEmpty = _buffer.empty();
						if (!ref6.isEmpty)
						{
							break_ = ref6.out();
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
						if (break_)
						{
							break;
						}
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
				return;
			}
			if (host->wait_trig(ath))
			{
				qg.unlock();
				throw close_exception();
			}
		}
	}

	template <typename H>
	size_t _try_pop(my_actor* host, H& out)
	{
		my_actor::quit_guard qg(host);
		bool isEmpty = false;
		bool closed = false;
		bool popCount = 0;
		LAMBDA_THIS_REF5(ref5, host, out, isEmpty, closed, popCount);
		host->send(_strand, [&ref5]
		{
			ref5.closed = ref5->_closed;
			if (!ref5->_closed)
			{
				while (true)
				{
					bool break_ = true;
					auto& _buffer = ref5->_buffer;
					auto& _pushWait = ref5->_pushWait;
					ref5.isEmpty = _buffer.empty();
					if (!ref5.isEmpty)
					{
						ref5.popCount++;
						break_ = ref5.out();
					}
					if (_buffer.size() <= _buffer.capacity() / 2 && !_pushWait.empty())
					{
						_pushWait.back()(false);
						_pushWait.pop_back();
					}
					if (break_)
					{
						break;
					}
				}
			}
		});
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return popCount;
	}

	template <typename H>
	size_t _timed_pop(int tm, my_actor* host, H& out)
	{
		my_actor::quit_guard qg(host);
		long long utm = (long long)tm * 1000;
		size_t popCount = 0;
		while (true)
		{
			actor_trig_handle<bool> ath;
			msg_list<pop_pck>::iterator mit;
			bool notified = false;
			bool isEmpty = false;
			bool closed = false;
			LAMBDA_THIS_REF8(ref8, notified, isEmpty, closed, host, out, ath, mit, popCount);
			host->send(_strand, [&ref8]
			{
				ref8.closed = ref8->_closed;
				if (!ref8->_closed)
				{
					while (true)
					{
						bool break_ = true;
						auto& _buffer = ref8->_buffer;
						auto& _pushWait = ref8->_pushWait;
						ref8.isEmpty = _buffer.empty();
						if (!ref8.isEmpty)
						{
							ref8.popCount++;
							break_ = ref8.out();
						}
						else
						{
							pop_pck pck = { ref8.notified, ref8.host->make_trig_notifer(ref8.ath) };
							ref8->_popWait.push_front(pck);
							ref8.mit = ref8->_popWait.begin();
						}
						if (_buffer.size() <= _buffer.capacity() / 2 && !_pushWait.empty())
						{
							_pushWait.back()(false);
							_pushWait.pop_back();
						}
						if (break_)
						{
							break;
						}
					}
				}
			});
			if (!closed)
			{
				if (!isEmpty)
				{
					return popCount;
				}
				long long bt = get_tick_us();
				if (!host->timed_wait_trig(utm / 1000, ath, closed))
				{
					host->send(_strand, [&ref8]
					{
						if (!ref8.notified)
						{
							ref8->_popWait.erase(ref8.mit);
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
					return popCount;
				}
			}
			if (closed)
			{
				qg.unlock();
				throw close_exception();
			}
		}
		return popCount;
	}
private:
	bool _closed;
	shared_strand _strand;
	boost::circular_buffer<T> _buffer;
	msg_list<push_pck> _pushWait;
	msg_list<pop_pck> _popWait;
};

#endif