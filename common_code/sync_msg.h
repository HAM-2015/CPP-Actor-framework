#ifndef __SYNC_MSG_H
#define __SYNC_MSG_H

#include "actor_framework.h"
#include "msg_queue.h"
#include "check_move.h"

/*!
@brief 同步发送消息（多读多写，角色可转换），发送方等到消息取出才返回
*/
template <typename T>
class sync_msg
{
	struct send_wait 
	{
		bool is_rvalue;
		bool& notified;
		T& src_msg;
		actor_trig_notifer<bool> ntf;
	};

	struct take_wait 
	{
		bool& notified;
		unsigned char* dst;
		actor_trig_notifer<bool> ntf;
	};
public:
	struct close_exception
	{
	};
public:
	sync_msg(shared_strand strand)
		:_closed(false), _strand(strand), _sendWait(4), _takeWait(4)
	{

	}

	~sync_msg()
	{

	}
public:
	template <typename TM>
	void send(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		bool wait = false;
		bool closed = false;
		bool notified = false;
		host->send(_strand, [&]
		{
			if (_closed)
			{
				closed = true;
			} 
			else
			{
				if (_takeWait.empty())
				{
					wait = true;
					send_wait pw = { check_move<TM&&>::is_rvalue, notified, msg, host->make_trig_notifer(ath) };
					_sendWait.push_front(pw);
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)T(check_move<TM&&>::move(msg));
					wt.notified = true;
					wt.ntf(false);
					_takeWait.pop_back();
				}
			}
		});
		if (wait)
		{
			closed = host->wait_trig(ath);
		}
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
	}

	template <typename TM>
	bool try_send(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		bool ok = false;
		bool closed = false;
		host->send(_strand, [&]
		{
			if (_closed)
			{
				closed = true;
			} 
			else
			{
				if (!_takeWait.empty())
				{
					ok = true;
					take_wait& wt = _takeWait.back();
					new(wt.dst)T(check_move<TM&&>::move(msg));
					wt.ntf(false);
					_takeWait.pop_back();
				}
			}
		});
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return ok;
	}

	template <typename TM>
	bool timed_send(int tm, my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		msg_list<send_wait>::iterator mit;
		bool wait = false;
		bool closed = false;
		bool notified = false;
		host->send(_strand, [&]
		{
			if (_closed)
			{
				closed = true;
			} 
			else
			{
				if (_takeWait.empty())
				{
					wait = true;
					send_wait pw = { check_move<TM&&>::is_rvalue, notified, msg, host->make_trig_notifer(ath) };
					_sendWait.push_front(pw);
					mit = _sendWait.begin();
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)T(check_move<TM&&>::move(msg));
					wt.notified = true;
					wt.ntf(false);
					_takeWait.pop_back();
				}
			}
		});
		if (wait)
		{
			if (!host->timed_wait_trig(tm, ath, closed))
			{
				host->send(_strand, [&]
				{
					if (!notified)
					{
						wait = false;
						_sendWait.erase(mit);
					}
				});
				if (wait)
				{
					closed = host->wait_trig(ath);
				}
			}
		}
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return !wait || notified;
	}

	T take(my_actor* host)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		unsigned char msgBuf[sizeof(T)];
		bool wait = false;
		bool closed = false;
		bool notified = false;
		host->send(_strand, [&]
		{
			if (_closed)
			{
				closed = true;
			} 
			else
			{
				if (_sendWait.empty())
				{
					wait = true;
					take_wait pw = { notified, msgBuf, host->make_trig_notifer(ath) };
					_takeWait.push_front(pw);
				}
				else
				{
					send_wait& wt = _sendWait.back();
					new(msgBuf)T(wt.is_rvalue ? std::move(wt.src_msg) : wt.src_msg);
					wt.notified = true;
					wt.ntf(false);
					_sendWait.pop_back();
				}
			}
		});
		if (wait)
		{
			closed = host->wait_trig(ath);
		}
		if (!closed)
		{
			AUTO_CALL({ ((T*)msgBuf)->~T(); });
			return std::move(*(T*)msgBuf);
		}
		qg.unlock();
		throw close_exception();
	}

	bool try_take(my_actor* host, T& out)
	{
		return try_take_ct(host, [&](bool rval, T& msg)
		{
			out = rval ? std::move(msg) : msg;
		});
	}

	bool try_take(my_actor* host, stack_obj<T>& out)
	{
		return try_take_ct(host, [&](bool rval, T& msg)
		{
			out.create(rval ? std::move(msg) : msg);
		});
	}

	bool timed_take(int tm, my_actor* host, T& out)
	{
		return timed_take_ct(tm, host, [&](bool rval, T& msg)
		{
			out = rval ? std::move(msg) : msg;
		});
	}

	bool timed_take(int tm, my_actor* host, stack_obj<T>& out)
	{
		return timed_take_ct(tm, host, [&](bool rval, T& msg)
		{
			out.create(rval ? std::move(msg) : msg);
		});
	}

	void close(my_actor* host)
	{
		my_actor::quit_guard qg(host);
		host->send(_strand, [&]
		{
			_closed = true;
			while (!_takeWait.empty())
			{
				auto& wt = _takeWait.front();
				wt.notified = true;
				wt.ntf(true);
				_takeWait.pop_front();
			}
			while (!_sendWait.empty())
			{
				auto& st = _sendWait.front();
				st.notified = true;
				st.ntf(true);
				_sendWait.pop_front();
			}
		});
	}

	void reset()
	{
		assert(_closed);
		_closed = false;
	}
private:
	template <typename CT>
	bool try_take_ct(my_actor* host, const CT& ct)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		bool ok = false;
		bool closed = false;
		host->send(_strand, [&]
		{
			if (_closed)
			{
				closed = true;
			}
			else
			{
				if (!_sendWait.empty())
				{
					ok = true;
					send_wait& wt = _sendWait.back();
					ct(wt.is_rvalue, wt.src_msg);
					wt.notified = true;
					wt.ntf(false);
					_sendWait.pop_back();
				}
			}
		});
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return ok;
	}

	template <typename CT>
	bool timed_take_ct(int tm, my_actor* host, const CT& ct)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		msg_list<take_wait>::iterator mit;
		unsigned char msgBuf[sizeof(T)];
		bool wait = false;
		bool closed = false;
		bool notified = false;
		host->send(_strand, [&]
		{
			if (_closed)
			{
				closed = true;
			}
			else
			{
				if (_sendWait.empty())
				{
					wait = true;
					take_wait pw = { notified, msgBuf, host->make_trig_notifer(ath) };
					_takeWait.push_front(pw);
					mit = _takeWait.begin();
				}
				else
				{
					send_wait& wt = _sendWait.back();
					ct(wt.is_rvalue, wt.src_msg);
					wt.notified = true;
					wt.ntf(false);
					_sendWait.pop_back();
				}
			}
		});
		bool ok = true;
		if (wait)
		{
			if (!host->timed_wait_trig(tm, ath, closed))
			{
				host->send(_strand, [&]
				{
					if (!notified)
					{
						wait = false;
						_takeWait.erase(mit);
					}
				});
				ok = wait;
				if (wait)
				{
					closed = host->wait_trig(ath);
				}
			}
			if (wait && !closed)
			{
				ct(true, *(T*)msgBuf);
				((T*)msgBuf)->~T();
			}
		}
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return ok;
	}

	sync_msg(const sync_msg&){};
	void operator=(const sync_msg&){};
private:
	bool _closed;
	shared_strand _strand;
	msg_list<take_wait> _takeWait;
	msg_list<send_wait> _sendWait;
};

/*!
@brief CSP模型消息（多读多写，角色可转换，可递归），发送方等消息取出并处理完成后才返回
*/
template <typename T, typename R = void>
class csp_channel
{
	struct send_wait
	{
		bool is_rvalue;
		bool& notified;
		T& src_msg;
		unsigned char* res;
		actor_trig_notifer<bool> ntf;
	};

	struct take_wait
	{
		bool& notified;
		unsigned char* dst;
		unsigned char*& res;
		actor_trig_notifer<bool> ntf;
		actor_trig_notifer<bool>& ntfSend;
	};
public:
	struct close_exception
	{
	};
public:
	csp_channel(shared_strand strand)
		:_closed(false), _strand(strand), _takeWait(4), _sendWait(4)
	{

	}

	virtual ~csp_channel()
	{

	}
public:
	template <typename TM>
	R send(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		unsigned char resBuf[sizeof(R)];
		bool closed = false;
		bool notified = false;
		host->send(_strand, [&]
		{
			if (_closed)
			{
				closed = true;
			}
			else
			{
				if (_takeWait.empty())
				{
					send_wait pw = { check_move<TM&&>::is_rvalue, notified, msg, resBuf, host->make_trig_notifer(ath) };
					_sendWait.push_front(pw);
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)T(check_move<TM&&>::move(msg));
					wt.notified = true;
					wt.ntf(false);
					wt.res = resBuf;
					wt.ntfSend = host->make_trig_notifer(ath);
					_takeWait.pop_back();
				}
			}
		});
		if (!closed)
		{
			closed = host->wait_trig(ath);
		}
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		AUTO_CALL({ ((R*)resBuf)->~R(); });
		return std::move(*(R*)resBuf);
	}

	template <typename H>
	void take(my_actor* host, const H& h)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		actor_trig_notifer<bool> ntfSend;
		unsigned char msgBuf[sizeof(T)];
		unsigned char* res = NULL;
		bool wait = false;
		bool closed = false;
		bool notified = false;
		host->send(_strand, [&]
		{
			if (_closed)
			{
				closed = true;
			}
			else
			{
				if (_sendWait.empty())
				{
					wait = true;
					take_wait pw = { notified, msgBuf, res, host->make_trig_notifer(ath), ntfSend };
					_takeWait.push_front(pw);
				}
				else
				{
					send_wait& wt = _sendWait.back();
					new(msgBuf)T(wt.is_rvalue ? std::move(wt.src_msg) : wt.src_msg);
					wt.notified = true;
					ntfSend = wt.ntf;
					res = wt.res;
					_sendWait.pop_back();
				}
			}
		});
		if (wait)
		{
			closed = host->wait_trig(ath);
		}
		if (!closed)
		{
			bool ok = false;
			AUTO_CALL({ ((T*)msgBuf)->~T(); });
			AUTO_CALL({ ntfSend(!ok); });
			BEGIN_TRY_
			{
				new(res)R(h(*(T*)msgBuf));
			}
			CATCH_FOR(close_exception)
			{
				qg.unlock();
				throw close_exception();
			}
			END_TRY_;
			ok = true;
			return;
		}
		qg.unlock();
		throw close_exception();
	}

	void close(my_actor* host)
	{
		my_actor::quit_guard qg(host);
		host->send(_strand, [&]
		{
			_closed = true;
			while (!_takeWait.empty())
			{
				auto& wt = _takeWait.front();
				wt.notified = true;
				wt.ntf(true);
				if (!wt.ntfSend.empty())
				{
					wt.ntfSend(true);
				}
				_takeWait.pop_front();
			}
			while (!_sendWait.empty())
			{
				auto& st = _sendWait.front();
				st.notified = true;
				st.ntf(true);
				_sendWait.pop_front();
			}
		});
	}

	void reset()
	{
		assert(_closed);
		_closed = false;
	}
private:
	csp_channel(const csp_channel&){};
	void operator=(const csp_channel&){};
private:
	bool _closed;
	shared_strand _strand;
	msg_list<take_wait> _takeWait;
	msg_list<send_wait> _sendWait;
};

struct void_return
{
};

template <typename T>
class csp_channel<T, void>: public csp_channel<T, void_return>
{
	typedef csp_channel<T, void_return> base_csp_channel;
public:
	struct close_exception
	{
	};
public:
	csp_channel(shared_strand strand)
		:base_csp_channel(strand)
	{

	}

	~csp_channel()
	{

	}
public:
	template <typename TM>
	void send(my_actor* host, TM&& msg)
	{
		base_csp_channel::send(host, CHECK_MOVE(msg));
	}

	template <typename H>
	void take(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](T& msg)->void_return
		{
			h(msg);
			return void_return();
		});
	}
};

#endif