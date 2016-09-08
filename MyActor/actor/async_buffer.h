#ifndef __ASYNC_BUFFER_H
#define __ASYNC_BUFFER_H

#include <memory>
#include "my_actor.h"

struct async_buffer_close_exception {};

template <typename T>
class fixed_buffer
{
	struct node
	{
		void destroy()
		{
			((T*)space)->~T();
#if (_DEBUG || DEBUG)
			memset(space, 0xcf, sizeof(space));
#endif
		}

		template <typename Arg>
		void set(Arg&& arg)
		{
			new(space)T(TRY_MOVE(arg));
		}

		T& get()
		{
			return *(T*)space;
		}

		__space_align char space[sizeof(T)];
	};
public:
	fixed_buffer(size_t maxSize)
	{
		assert(0 != maxSize);
		_size = 0;
		_index = 0;
		_maxSize = maxSize;
		_buffer = (node*)malloc(sizeof(node)*maxSize);
	}

	~fixed_buffer()
	{
		clear();
		free(_buffer);
	}
public:
	size_t size() const
	{
		return _size;
	}

	size_t max_size() const
	{
		return _maxSize;
	}

	bool empty() const
	{
		return 0 == _size;
	}

	bool full() const
	{
		return _maxSize == _size;
	}

	void clear()
	{
		for (size_t i = 0; i < _size; i++)
		{
			const size_t t = _index + i;
			_buffer[(t < _maxSize) ? t : (t - _maxSize)].destroy();
		}
		_index = 0;
		_size = 0;
	}

	T& front()
	{
		assert(!empty());
		const size_t i = _index + _size - 1;
		return _buffer[(i < _maxSize) ? i : (i - _maxSize)].get();
	}

	void pop_front()
	{
		assert(!empty());
		_size--;
		const size_t i = _index + _size;
		_buffer[(i < _maxSize) ? i : (i - _maxSize)].destroy();
	}

	template <typename Arg>
	void push_back(Arg&& arg)
	{
		assert(!full());
		const size_t i = (0 != _index) ? (_index - 1) : (_maxSize - 1);
		_buffer[i].set(TRY_MOVE(arg));
		_index = i;
		_size++;
	}
private:
	size_t _maxSize;
	size_t _size;
	size_t _index;
	node* _buffer;
};

/*!
@brief 异步缓冲队列，多写/多读，角色可转换
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
		trig_notifer<bool> ntf;
	};

	struct pop_pck
	{
		void operator ()(bool closed)
		{
			notified = true;
			ntf(closed);
		}

		bool& notified;
		trig_notifer<bool> ntf;
	};
	
	struct buff_push
	{
		template <typename Fst, typename... Args>
		inline void operator ()(Fst&& fst, Args&&... args)
		{
			if (0 == _i)
			{
				_this->_buffer.push_back(TRY_MOVE(fst));
			} 
			else
			{
				buff_push bp = { _this, _i - 1 };
				bp(TRY_MOVE(args)...);
			}
		}

		inline void operator ()()
		{
			assert(false);
		}

		async_buffer* const _this;
		const size_t _i;
	};

	struct buff_pop
	{
		template <typename Fst, typename... Outs>
		inline void operator ()(Fst& fst, Outs&... outs)
		{
			if (0 == _i)
			{
				fst = std::move(_this->_buffer.front());
			} 
			else
			{
				buff_pop bp = { _this, _i - 1 };
				bp(outs...);
			}
		}

		inline void operator ()()
		{
			assert(false);
		}

		async_buffer* const _this;
		const size_t _i;
	};
public:
	struct close_exception : public async_buffer_close_exception {};
public:
	async_buffer(const shared_strand& strand, size_t buffLength, size_t halfLength = 0)
		:_strand(strand), _closed(false), _buffer(buffLength)//, _pushWait(4), _popWait(4)
	{
		_halfLength = halfLength ? halfLength : buffLength / 2;
	}
public:
	/*!
	@brief 从左到右添加多条数据，如果队列已满就等待直到都成功
	*/
	template <typename...  TMS>
	void push(my_actor* host, TMS&&... msgs)
	{
		static_assert(sizeof...(TMS) > 0, "");
		size_t pushCount = 0;
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
		std::tuple<TMS&&...> msgsTup(TRY_MOVE(msgs)...);
#endif
		_push(host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
			tuple_invoke(bp, msgsTup);
#else
			bp((TMS&&)(msgs)...);
#endif
			return sizeof...(TMS) == ++pushCount;
		});
	}

	/*!
	@brief 尝试添加数据，如果队列已满就放弃
	@return true 添加成功
	*/
	template <typename TM>
	bool try_push(my_actor* host, TM&& msg)
	{
		return !!_try_push(host, [&]()->bool
		{
			_buffer.push_back((TM&&)msg);
			return true;
		});
	}

	/*!
	@brief 从左到右尝试添加数据，如果队列放不下就放弃多余的
	@return 从左到右成功添加了几条
	*/
	template <typename...  TMS>
	size_t try_push(my_actor* host, TMS&&... msgs)
	{
		static_assert(sizeof...(TMS) > 1, "");
		size_t pushCount = 0;
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
		std::tuple<TMS&&...> msgsTup(TRY_MOVE(msgs)...);
#endif
		return _try_push(host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
			tuple_invoke(bp, msgsTup);
#else
			bp((TMS&&)(msgs)...);
#endif
			return sizeof...(TMS) == ++pushCount;
		});
	}

	/*!
	@brief 在一定时间内尝试添加数据，如果队列到时还无法添加就放弃
	@return true 添加成功
	*/
	template <typename TM>
	bool timed_push(int tm, my_actor* host, TM&& msg)
	{
		return !!_timed_push(tm, host, [&]()->bool
		{
			_buffer.push_back((TM&&)msg);
			return true;
		});
	}

	/*!
	@brief 从左到右在一定时间内尝试添加多条数据，如果队列到时还无法添加就放弃
	@return 从左到右成功添加了几条
	*/
	template <typename...  TMS>
	size_t timed_push(int tm, my_actor* host, TMS&&... msgs)
	{
		static_assert(sizeof...(TMS) > 1, "");
		size_t pushCount = 0;
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
		std::tuple<TMS&&...> msgsTup(TRY_MOVE(msgs)...);
#endif
		return _timed_push(tm, host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
			tuple_invoke(bp, msgsTup);
#else
			bp((TMS&&)(msgs)...);
#endif
			return sizeof...(TMS) == ++pushCount;
		});
	}

	/*!
	@brief 从左到右强行添加多条数据，超出队列就丢弃最前面的数据
	@return 丢弃了几条数据
	*/
	template <typename...  TMS>
	size_t force_push(my_actor* host, TMS&&... msgs)
	{
		static_assert(sizeof...(TMS) > 0, "");
		size_t pushCount = 0;
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
		std::tuple<TMS&&...> msgsTup(TRY_MOVE(msgs)...);
#endif
		return _force_push(host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
			tuple_invoke(bp, msgsTup);
#else
			bp((TMS&&)(msgs)...);
#endif
			return sizeof...(TMS) == ++pushCount;
		}, [](const T&){});
	}

	template <typename Recovery, typename... TMS>
	size_t force_push_recovery(my_actor* host, Recovery&& recovery, TMS&&... msgs)
	{
		static_assert(sizeof...(TMS) > 0, "");
		size_t pushCount = 0;
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
		std::tuple<TMS&&...> msgsTup(TRY_MOVE(msgs)...);
#endif
		return _force_push(host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
			tuple_invoke(bp, msgsTup);
#else
			bp((TMS&&)(msgs)...);
#endif
			return sizeof...(TMS) == ++pushCount;
		}, TRY_MOVE(recovery));
	}

	/*!
	@brief 弹出一条数据，如果队列空就等待直到有数据
	*/
	T pop(my_actor* host)
	{
		__space_align char resBuf[sizeof(T)];
		_pop(host, [&]()->bool
		{
			new(resBuf)T(std::move(_buffer.front()));
			_buffer.pop_front();
			return true;
		});
		BREAK_OF_SCOPE(
		{
			typedef T TP_;
			((TP_*)resBuf)->~TP_();
		});
		return std::move(*(T*)resBuf);
	}

	/*!
	@brief 从左到右弹出多条数据，如果队列不够就等待直到成功
	*/
	template <typename... OTMS>
	void pop(my_actor* host, OTMS&... outs)
	{
		static_assert(sizeof...(OTMS) > 0, "");
		size_t popCount = 0;
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
		std::tuple<OTMS&...> outsTup(TRY_MOVE(outs)...);
#endif
		_pop(host, [&]()->bool
		{
			buff_pop bp = { this, popCount };
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
			tuple_invoke(bp, outsTup);
#else
			bp(outs...);
#endif
			_buffer.pop_front();
			return sizeof...(OTMS) == ++popCount;
		});
	}

	/*!
	@brief 尝试弹出一条数据，如果队列为空就放弃
	@return 成功得到消息
	*/
	template <typename OTM>
	bool try_pop(my_actor* host, OTM& out)
	{
		size_t popCount = 0;
		return !!_try_pop(host, [&]()->bool
		{
			out = std::move(_buffer.front());
			_buffer.pop_front();
			return true;
		});
	}

	/*!
	@brief 从左到右尝试弹出多条数据，如果队列不够放弃剩下的
	@return 从左到右得到几条数据
	*/
	template <typename... OTMS>
	size_t try_pop(my_actor* host, OTMS&... outs)
	{
		static_assert(sizeof...(OTMS) > 1, "");
		size_t popCount = 0;
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
		std::tuple<OTMS&...> outsTup(TRY_MOVE(outs)...);
#endif
		return _try_pop(host, [&]()->bool
		{
			buff_pop bp = { this, popCount };
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
			tuple_invoke(bp, outsTup);
#else
			bp(outs...);
#endif
			_buffer.pop_front();
			return sizeof...(OTMS) == ++popCount;
		});
	}

	/*!
	@brief 在一定时间内尝试弹出一条数据，如果队列到时还无数据就放弃
	@return true 成功得到消息
	*/
	template <typename OTM>
	bool timed_pop(int tm, my_actor* host, OTM& out)
	{
		return !!_timed_pop(tm, host, [&]()->bool
		{
			out = std::move(_buffer.front());
			_buffer.pop_front();
			return true;
		});
	}

	/*!
	@brief 从左到右在一定时间内尝试弹出多条数据，如果队列到时还不够就放弃剩下的
	@return 成功得到几条消息
	*/
	template <typename... OTMS>
	size_t timed_pop(int tm, my_actor* host, OTMS&... outs)
	{
		static_assert(sizeof...(OTMS) > 1, "");
		size_t popCount = 0;
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
		std::tuple<OTMS&...> outsTup(TRY_MOVE(outs)...);
#endif
		return _timed_pop(tm, host, [&]()->bool
		{
			buff_pop bp = { this, popCount };
#if (_MSC_VER >= 1900 || (__GNUG__*10 + __GNUC_MINOR__) <= 48)
			tuple_invoke(bp, outsTup);
#else
			bp(outs...);
#endif
			_buffer.pop_front();
			return sizeof...(OTMS) == ++popCount;
		});
	}

	/*!
	@brief 注册一个pop消息通知，只触发一次
	*/
	template <typename Ntf>
	void regist_pop_ntf(my_actor* host, Ntf&& ntf)
	{
		host->lock_quit();
		host->send(_strand, [&]
		{
			if (!_buffer.empty())
			{
				ntf();
			}
			else
			{
				_popNtfQueue.push_back((Ntf&&)ntf);
			}
		});
		host->unlock_quit();
	}

	/*!
	@brief 注册一个push消息通知，只触发一次
	*/
	template <typename Ntf>
	void regist_push_ntf(my_actor* host, Ntf&& ntf)
	{
		host->lock_quit();
		host->send(_strand, [&]
		{
			if (_buffer.size() <= _halfLength)
			{
				ntf();
			}
			else
			{
				_pushNtfQueue.push_back((Ntf&&)ntf);
			}
		});
		host->unlock_quit();
	}

	void close(my_actor* host, bool clearBuff = true)
	{
		host->lock_quit();
		host->send(_strand, [&]
		{
			_closed = true;
			if (clearBuff)
			{
				_buffer.clear();
			}
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
			_pushNtfQueue.clear();
			_popNtfQueue.clear();
		});
		host->unlock_quit();
	}

	size_t length(my_actor* host)
	{
		host->lock_quit();
		size_t l = 0;
		host->send(_strand, [&]
		{
			l = _buffer.size();
		});
		host->unlock_quit();
		return l;
	}

	size_t snap_length()
	{
		return _buffer.size();
	}

	void reset()
	{
		assert(_closed);
		_closed = false;
	}
private:
	async_buffer(const async_buffer&){};
	void operator=(const async_buffer&){};

	template <typename Func>
	void _push(my_actor* host, Func&& h)
	{
		host->lock_quit();
		while (true)
		{
			trig_handle<bool> ath;
			bool notified = false;
			bool isFull = false;
			bool closed = false;
			host->send(_strand, [&]
			{
				closed = _closed;
				if (!_closed)
				{
					while (true)
					{
						bool break_ = true;
						isFull = _buffer.full();
						if (!isFull)
						{
							break_ = h();
							if (!_popWait.empty())
							{
								_popWait.back()(false);
								_popWait.pop_back();
							}
							else if (!_popNtfQueue.empty())
							{
								_popNtfQueue.front()();
								_popNtfQueue.pop_front();
							}
						}
						else
						{
							_pushWait.push_front(push_pck{ notified });
							_pushWait.front().ntf = host->make_trig_notifer_to_self(ath);
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
				host->unlock_quit();
				throw close_exception();
			}
			if (!isFull)
			{
				host->unlock_quit();
				return;
			}
			if (host->wait_trig(ath))
			{
				host->unlock_quit();
				throw close_exception();
			}
		}
		host->unlock_quit();
	}

	template <typename Func>
	size_t _try_push(my_actor* host, Func&& h)
	{
		host->lock_quit();
		bool closed = false;
		size_t pushCount = 0;
		host->send(_strand, [&]
		{
			closed = _closed;
			if (!_closed)
			{
				while (true)
				{
					bool break_ = true;
					if (!_buffer.full())
					{
						pushCount++;
						break_ = h();
						if (!_popWait.empty())
						{
							_popWait.back()(false);
							_popWait.pop_back();
						}
						else if (!_popNtfQueue.empty())
						{
							_popNtfQueue.front()();
							_popNtfQueue.pop_front();
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
			host->unlock_quit();
			throw close_exception();
		}
		host->unlock_quit();
		return pushCount;
	}

	template <typename Func>
	size_t _timed_push(int tm, my_actor* host, Func&& h)
	{
		host->lock_quit();
		long long utm = (long long)tm * 1000;
		size_t pushCount = 0;
		while (true)
		{
			trig_handle<bool> ath;
			typename msg_list<push_pck>::iterator mit;
			bool notified = false;
			bool isFull = false;
			bool closed = false;
			host->send(_strand, [&]
			{
				closed = _closed;
				if (!_closed)
				{
					while (true)
					{
						bool break_ = true;
						isFull = _buffer.full();
						if (!isFull)
						{
							pushCount++;
							break_ = h();
							if (!_popWait.empty())
							{
								_popWait.back()(false);
								_popWait.pop_back();
							}
							else if (!_popNtfQueue.empty())
							{
								_popNtfQueue.front()();
								_popNtfQueue.pop_front();
							}
						}
						else
						{
							_pushWait.push_front(push_pck{ notified });
							_pushWait.front().ntf = host->make_trig_notifer_to_self(ath);
							mit = _pushWait.begin();
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
					host->unlock_quit();
					return pushCount;
				}
				long long bt = get_tick_us();
				if (!host->timed_wait_trig(utm / 1000, ath, closed))
				{
					host->send(_strand, [&]
					{
						if (!notified)
						{
							_pushWait.erase(mit);
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
					host->unlock_quit();
					return pushCount;
				}
			}
			if (closed)
			{
				host->unlock_quit();
				throw close_exception();
			}
		}
		host->unlock_quit();
		return pushCount;
	}

	template <typename Func, typename Recovery>
	size_t _force_push(my_actor* host, Func&& h, Recovery&& rf)
	{
		host->lock_quit();
		bool closed = false;
		size_t lostCount = 0;
		host->send(_strand, [&]
		{
			closed = _closed;
			if (!_closed)
			{
				while (true)
				{
					if (_buffer.full())
					{
						lostCount++;
						rf(_buffer.front());
						_buffer.pop_front();
					}
					bool break_ = h();
					if (!_popWait.empty())
					{
						_popWait.back()(false);
						_popWait.pop_back();
					}
					else if (!_popNtfQueue.empty())
					{
						_popNtfQueue.front()();
						_popNtfQueue.pop_front();
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
			host->unlock_quit();
			throw close_exception();
		}
		host->unlock_quit();
		return lostCount;
	}

	template <typename Func>
	void _pop(my_actor* host, Func&& out)
	{
		host->lock_quit();
		while (true)
		{
			trig_handle<bool> ath;
			bool notified = false;
			bool isEmpty = false;
			bool closed = false;
			host->send(_strand, [&]
			{
				closed = _closed;
				if (!_closed)
				{
					while (true)
					{
						bool break_ = true;
						isEmpty = _buffer.empty();
						if (!isEmpty)
						{
							break_ = out();
						}
						else
						{
							_popWait.push_front(pop_pck{ notified });
							_popWait.front().ntf = host->make_trig_notifer_to_self(ath);
						}
						if (_buffer.size() <= _halfLength)
						{
							if (!_pushWait.empty())
							{
								_pushWait.back()(false);
								_pushWait.pop_back();
							}
							else if (!_pushNtfQueue.empty())
							{
								_pushNtfQueue.front()();
								_pushNtfQueue.pop_front();
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
				host->unlock_quit();
				throw close_exception();
			}
			if (!isEmpty)
			{
				host->unlock_quit();
				return;
			}
			if (host->wait_trig(ath))
			{
				host->unlock_quit();
				throw close_exception();
			}
		}
		host->unlock_quit();
	}

	template <typename Func>
	size_t _try_pop(my_actor* host, Func&& out)
	{
		host->lock_quit();
		bool closed = false;
		bool popCount = 0;
		host->send(_strand, [&]
		{
			closed = _closed;
			if (!_closed)
			{
				while (true)
				{
					bool break_ = true;
					if (!_buffer.empty())
					{
						popCount++;
						break_ = out();
					}
					if (_buffer.size() <= _halfLength)
					{
						if (!_pushWait.empty())
						{
							_pushWait.back()(false);
							_pushWait.pop_back();
						}
						else if (!_pushNtfQueue.empty())
						{
							_pushNtfQueue.front()();
							_pushNtfQueue.pop_front();
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
			host->unlock_quit();
			throw close_exception();
		}
		host->unlock_quit();
		return popCount;
	}

	template <typename Func>
	size_t _timed_pop(int tm, my_actor* host, Func&& out)
	{
		host->lock_quit();
		long long utm = (long long)tm * 1000;
		size_t popCount = 0;
		while (true)
		{
			trig_handle<bool> ath;
			typename msg_list<pop_pck>::iterator mit;
			bool notified = false;
			bool isEmpty = false;
			bool closed = false;
			host->send(_strand, [&]
			{
				closed = _closed;
				if (!_closed)
				{
					while (true)
					{
						bool break_ = true;
						isEmpty = _buffer.empty();
						if (!isEmpty)
						{
							popCount++;
							break_ = out();
						}
						else
						{
							_popWait.push_front(pop_pck{ notified });
							_popWait.front().ntf = host->make_trig_notifer_to_self(ath);
							mit = _popWait.begin();
						}
						if (_buffer.size() <= _halfLength)
						{
							if (!_pushWait.empty())
							{
								_pushWait.back()(false);
								_pushWait.pop_back();
							}
							else if (!_pushNtfQueue.empty())
							{
								_pushNtfQueue.front()();
								_pushNtfQueue.pop_front();
							}
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
					host->unlock_quit();
					return popCount;
				}
				long long bt = get_tick_us();
				if (!host->timed_wait_trig(utm / 1000, ath, closed))
				{
					host->send(_strand, [&]
					{
						if (!notified)
						{
							_popWait.erase(mit);
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
					host->unlock_quit();
					return popCount;
				}
			}
			if (closed)
			{
				host->unlock_quit();
				throw close_exception();
			}
		}
		host->unlock_quit();
		return popCount;
	}
private:
	shared_strand _strand;
	fixed_buffer<T> _buffer;
	msg_list<push_pck> _pushWait;
	msg_list<pop_pck> _popWait;
	msg_queue<std::function<void()>> _popNtfQueue;
	msg_queue<std::function<void()>> _pushNtfQueue;
	size_t _halfLength;
	bool _closed;
};

#endif