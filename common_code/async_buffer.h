#ifndef __ASYNC_BUFFER_H
#define __ASYNC_BUFFER_H

#include <boost/circular_buffer.hpp>
#include <functional>
#include <memory>
#include "actor_framework.h"

struct async_buffer_close_exception {};

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
	
	struct buff_push1
	{
		template <typename... Args>
		inline void operator ()(Args&&... args)
		{
			if (_i < sizeof...(Args) / 2)
			{
				left_invoke<sizeof...(Args) / 2>::invoke(*this, TRY_MOVE(args)...);
			} 
			else
			{
				buff_push1 bp = { _this, _i - sizeof...(Args) / 2 };
				right_invoke<sizeof...(Args)-sizeof...(Args) / 2>::invoke(bp, TRY_MOVE(args)...);
			}
		}

		template <typename Arg>
		inline void operator ()(Arg&& arg)
		{
			assert(0 == _i);
			_this->_buffer.push_back(TRY_MOVE(arg));
		}

		async_buffer* const _this;
		const size_t _i;
	};
	
	struct buff_pop1
	{
		template <typename... Outs>
		inline void operator ()(Outs&... outs)
		{
			if (_i < sizeof...(Outs) / 2)
			{
				left_invoke<sizeof...(Outs) / 2>::invoke(*this, outs...);
			}
			else
			{
				buff_pop1 bp = { _this, _i - sizeof...(Outs) / 2 };
				right_invoke<sizeof...(Outs)-sizeof...(Outs) / 2>::invoke(bp, outs...);
			}
		}

		template <typename Out>
		inline void operator ()(Out& out)
		{
			assert(0 == _i);
			out = std::move(_this->_buffer.front());
		}

		async_buffer* const _this;
		const size_t _i;
	};
	
	struct buff_push2
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
				buff_push2 bp = { _this, _i - 1 };
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

	struct buff_pop2
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
				buff_pop2 bp = { _this, _i - 1 };
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

	typedef buff_push2 buff_push;
	typedef buff_pop2 buff_pop;
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
		_push(host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
			bp((TMS&&)(msgs)...);
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
		return _try_push(host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
			bp((TMS&&)(msgs)...);
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
		return _timed_push(tm, host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
			bp((TMS&&)(msgs)...);
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
		return _force_push(host, [&]()->bool
		{
			buff_push bp = { this, pushCount };
			bp((TMS&&)(msgs)...);
			return sizeof...(TMS) == ++pushCount;
		});
	}

	/*!
	@brief 弹出一条数据，如果队列空就等待直到有数据
	*/
	T pop(my_actor* host)
	{
		char resBuf[sizeof(T)];
		_pop(host, [&]()->bool
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

	/*!
	@brief 从左到右弹出多条数据，如果队列不够就等待直到成功
	*/
	template <typename... OTMS>
	void pop(my_actor* host, OTMS&... outs)
	{
		static_assert(sizeof...(OTMS) > 0, "");
		size_t popCount = 0;
		_pop(host, [&]()->bool
		{
			buff_pop bp = { this, popCount };
			bp(outs...);
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
		return _try_pop(host, [&]()->bool
		{
			buff_pop bp = { this, popCount };
			bp(outs...);
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
		return _timed_pop(tm, host, [&]()->bool
		{
			buff_pop bp = { this, popCount };
			bp(outs...);
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
		bool closed = false;
		size_t pushCount = 0;
		LAMBDA_THIS_REF4(ref4, host, h, closed, pushCount);
		host->send(_strand, [&ref4]
		{
			ref4.closed = ref4->_closed;
			if (!ref4->_closed)
			{
				while (true)
				{
					bool break_ = true;
					if (!ref4->_buffer.full())
					{
						ref4.pushCount++;
						break_ = ref4.h();
						auto& _popWait = ref4->_popWait;
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
	size_t _force_push(my_actor* host, H& h)
	{
		my_actor::quit_guard qg(host);
		bool closed = false;
		size_t lostCount = 0;
		LAMBDA_THIS_REF4(ref4, host, h, closed, lostCount);
		host->send(_strand, [&ref4]
		{
			ref4.closed = ref4->_closed;
			if (!ref4->_closed)
			{
				while (true)
				{
					if (ref4->_buffer.full())
					{
						ref4.lostCount++;
						ref4->_buffer.pop_front();
					}
					bool break_ = ref4.h();
					auto& _popWait = ref4->_popWait;
					if (!_popWait.empty())
					{
						_popWait.back()(false);
						_popWait.pop_back();
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
		return lostCount;
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
						if (!_pushWait.empty() && _buffer.size() <= ref6->_halfLength)
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
		bool closed = false;
		bool popCount = 0;
		LAMBDA_THIS_REF4(ref4, host, out, closed, popCount);
		host->send(_strand, [&ref4]
		{
			ref4.closed = ref4->_closed;
			if (!ref4->_closed)
			{
				while (true)
				{
					bool break_ = true;
					auto& _buffer = ref4->_buffer;
					auto& _pushWait = ref4->_pushWait;
					if (!_buffer.empty())
					{
						ref4.popCount++;
						break_ = ref4.out();
					}
					if (!_pushWait.empty() && _buffer.size() <= ref4->_halfLength)
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
						if (!_pushWait.empty() && _buffer.size() <= ref8->_halfLength)
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
	shared_strand _strand;
	boost::circular_buffer<T> _buffer;
	msg_list<push_pck> _pushWait;
	msg_list<pop_pck> _popWait;
	size_t _halfLength;
	bool _closed;
};

#endif