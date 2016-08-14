#ifndef __SYNC_MSG_H
#define __SYNC_MSG_H

#include "my_actor.h"
#include "msg_queue.h"
#include "try_move.h"
#include "lambda_ref.h"

struct sync_csp_exception {};

struct sync_csp_close_exception : public sync_csp_exception {};

struct sync_msg_close_exception : public sync_csp_close_exception {};

struct csp_channel_close_exception : public sync_csp_close_exception {};

struct csp_try_invoke_exception : public sync_csp_exception {};

struct csp_timed_invoke_exception : public sync_csp_exception {};

struct csp_try_wait_exception : public sync_csp_exception {};

struct csp_timed_wait_exception : public sync_csp_exception {};


//////////////////////////////////////////////////////////////////////////
template <typename TP, typename TM>
struct SendMove_ : public try_move<TM>
{
};

template <typename TP, typename TM>
struct SendMove_<TP&, TM&&>
{
	static inline TM& move(TM& p0)
	{
		return p0;
	}
};

template <typename TP, typename TM>
struct SendMove_<TP&&, TM&&>
{
	static inline TM&& move(TM& p0)
	{
		return (TM&&)p0;
	}
};
//////////////////////////////////////////////////////////////////////////
template <typename TP>
struct TakeMove_
{
	static inline TP&& move(TP& p0)
	{
		return (TP&&)p0;
	}
};

template <typename TP>
struct TakeMove_<TP&>
{
	static inline TP& move(TP& p0)
	{
		return p0;
	}
};

template <typename TP>
struct TakeMove_<TP&&>
{
	static inline TP&& move(TP& p0)
	{
		return (TP&&)p0;
	}
};


template <typename T>
struct sync_msg_result_type
{
	typedef T type;
};

template <typename T>
struct sync_msg_result_type<T&>
{
	typedef std::reference_wrapper<T> type;
};

template <typename T>
struct sync_msg_result_type<T&&>
{
	typedef T type;
};

struct NtfHandle_
{
	NtfHandle_(shared_bool&& b, std::function<void()>&& n)
		:sign(std::move(b)), ntf(std::move(n)) {}

	NtfHandle_(const shared_bool& b, std::function<void()>&& n)
		:sign(b), ntf(std::move(n)) {}

	void operator()()
	{
		if (!sign.empty())
		{
			sign = true;
		}
		ntf();
	}

	void set()
	{
		if (!sign.empty())
		{
			sign = true;
		}
	}

	shared_bool sign;
	std::function<void()> ntf;
	RVALUE_COPY_CONSTRUCTION(NtfHandle_, sign, ntf)
};

/*!
@brief 同步发送消息（多读多写，角色可转换），发送方等到消息取出才返回
*/
template <typename T>
class sync_msg
{
	typedef RM_REF(T) ST;
	typedef typename sync_msg_result_type<T>::type RT;

	//////////////////////////////////////////////////////////////////////////
	struct send_wait
	{
		bool can_move;
		bool& notified;
		ST& src_msg;
		trig_notifer<bool> ntf;
	};

	struct take_wait
	{
		bool* can_move;
		bool& notified;
		char* dst;
		trig_notifer<bool>& takeOk;
		trig_notifer<bool> ntf;
	};
public:
	struct close_exception : public sync_msg_close_exception {};

	class ntf_handle
	{
		friend sync_msg;
	public:
		ntf_handle() {}
	private:
		bool ntfed()
		{
			if (!sign.empty())
			{
				return sign;
			}
			return true;
		}

		void set()
		{
			assert(!sign.empty());
			sign = true;
		}

		shared_bool sign;
		msg_list<NtfHandle_>::iterator it;
		RVALUE_COPY_CONSTRUCTION(ntf_handle, sign, it)
	};
public:
	sync_msg(const shared_strand& strand)
		:_closed(false), _strand(strand), _sendWait(4), _takeWait(4) {}
public:
	/*!
	@brief 同步发送一条消息，直到对方取出后返回
	*/
	template <typename TM>
	void send(my_actor* host, TM&& msg)
	{
		host->lock_quit();
		trig_handle<bool> ath;
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
					_sendWait.push_front({ try_move<TM&&>::can_move, notified, (ST&)msg });
					_sendWait.front().ntf = host->make_trig_notifer_to_self(ath);
					if (!_takeNtfQueue.empty())
					{
						_takeNtfQueue.front()();
						_takeNtfQueue.pop_front();
					}
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)RT(SendMove_<T, TM&&>::move(msg));
					if (wt.can_move)
					{
						*wt.can_move = try_move<TM&&>::can_move || !(std::is_reference<T>::value);
					}
					wt.notified = true;
					wt.takeOk = host->make_trig_notifer_to_self(ath);
					wt.ntf(false);
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
			host->unlock_quit();
			throw close_exception();
		}
		host->unlock_quit();
	}

	/*!
	@brief 尝试同步发送一条消息，如果对方在等待消息则成功
	@return 成功返回true
	*/
	template <typename TM>
	bool try_send(my_actor* host, TM&& msg)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		bool ok = false;
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
				if (!_takeWait.empty())
				{
					ok = true;
					take_wait& wt = _takeWait.back();
					new(wt.dst)RT(SendMove_<T, TM&&>::move(msg));
					if (wt.can_move)
					{
						*wt.can_move = try_move<TM&&>::can_move || !(std::is_reference<T>::value);
					}
					wt.takeOk = host->make_trig_notifer_to_self(ath);
					wt.ntf(false);
					_takeWait.pop_back();
				}
				else if (!_takeNtfQueue.empty())
				{
					ok = true;
					_sendWait.push_front({ try_move<TM&&>::can_move, notified, (T&)msg });
					_sendWait.front().ntf = host->make_trig_notifer_to_self(ath);
					_takeNtfQueue.front()();
					_takeNtfQueue.pop_front();
				}
			}
		});
		if (!closed && ok)
		{
			closed = host->wait_trig(ath);
		}
		if (closed)
		{
			host->unlock_quit();
			throw close_exception();
		}
		host->unlock_quit();
		return ok;
	}

	/*!
	@brief 尝试同步发送一条消息，对方在一定时间内取出则成功
	@return 成功返回true
	*/
	template <typename TM>
	bool timed_send(int tm, my_actor* host, TM&& msg)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		typename msg_list<send_wait>::iterator mit;
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
					_sendWait.push_front({ try_move<TM&&>::can_move, notified, (T&)msg });
					_sendWait.front().ntf = host->make_trig_notifer_to_self(ath);
					mit = _sendWait.begin();
					if (!_takeNtfQueue.empty())
					{
						_takeNtfQueue.front()();
						_takeNtfQueue.pop_front();
						tm = -1;
					}
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)RT(SendMove_<T, TM&&>::move(msg));
					if (wt.can_move)
					{
						*wt.can_move = try_move<TM&&>::can_move || !(std::is_reference<T>::value);
					}
					wt.notified = true;
					wt.takeOk = host->make_trig_notifer_to_self(ath);
					wt.ntf(false);
					_takeWait.pop_back();
				}
			}
		});
		if (!closed)
		{
			if (!host->timed_wait_trig(tm, ath, closed))
			{
				host->send(_strand, [&]
				{
					if (!notified)
					{
						_sendWait.erase(mit);
					}
				});
				if (notified)
				{
					closed = host->wait_trig(ath);
				}
			}
		}
		if (closed)
		{
			host->unlock_quit();
			throw close_exception();
		}
		host->unlock_quit();
		return notified;
	}

	/*!
	@brief 取出一条消息，直到有消息过来才返回
	*/
	RT take(my_actor* host)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		trig_notifer<bool> ntf;
		__space_align char msgBuf[sizeof(RT)];
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
					_takeWait.push_front({ NULL, notified, msgBuf, ntf });
					_takeWait.front().ntf = host->make_trig_notifer_to_self(ath);
				}
				else
				{
					send_wait& wt = _sendWait.back();
					new(msgBuf)RT(wt.can_move ? TakeMove_<T>::move(wt.src_msg) : wt.src_msg);
					wt.notified = true;
					ntf = std::move(wt.ntf);
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
			BREAK_OF_SCOPE(
			{
				typedef RT TP_;
				((TP_*)msgBuf)->~TP_();
				ntf(false);
			});
			host->unlock_quit();
			return std::move((RT&)*(RT*)msgBuf);
		}
		host->unlock_quit();
		throw close_exception();
	}

	/*!
	@brief 尝试取出一条消息，如果有就成功
	@return 成功返回true
	*/
	template <typename TM>
	bool try_take(my_actor* host, TM& out)
	{
		return try_take_ct(host, [&](bool rval, ST& msg)
		{
			out = rval ? std::move(msg) : msg;
		});
	}

	bool try_take(my_actor* host, stack_obj<T>& out)
	{
		return try_take_ct(host, [&](bool rval, ST& msg)
		{
			out.create(rval ? std::move(msg) : msg);
		});
	}

	/*!
	@brief 尝试取出一条消息，在一定时间内取到就成功
	@return 成功返回true
	*/
	template <typename TM>
	bool timed_take(int tm, my_actor* host, TM& out)
	{
		return timed_take_ct(tm, host, [&](bool rval, ST& msg)
		{
			out = rval ? std::move(msg) : msg;
		});
	}

	bool timed_take(int tm, my_actor* host, stack_obj<T>& out)
	{
		return timed_take_ct(tm, host, [&](bool rval, ST& msg)
		{
			out.create(rval ? std::move(msg) : msg);
		});
	}

	/*!
	@brief 注册一个take消息通知，只触发一次
	*/
	template <typename Ntf>
	void regist_take_ntf(my_actor* host, Ntf&& ntf)
	{
		host->lock_quit();
		host->send(_strand, [&]
		{
			if (!_sendWait.empty())
			{
				ntf();
			}
			else
			{
				_takeNtfQueue.push_back(NtfHandle_(shared_bool(), (Ntf&&)ntf));
			}
		});
		host->unlock_quit();
	}

	/*!
	@brief 注册一个take消息通知，只触发一次，返回注册句柄
	*/
	template <typename Ntf>
	ntf_handle regist_take_ntf2(my_actor* host, Ntf&& ntf)
	{
		host->lock_quit();
		ntf_handle h;
		host->send(_strand, [&]
		{
			if (!_sendWait.empty())
			{
				ntf();
			}
			else
			{
				h.sign = shared_bool::new_(false);
				_takeNtfQueue.push_back(NtfHandle_(h.sign, (Ntf&&)ntf));
				h.it = --_takeNtfQueue.end();
			}
		});
		host->unlock_quit();
		return h;
	}

	/*!
	@brief 移除通知句柄，成功返回true
	*/
	bool remove_ntf(my_actor* host, ntf_handle& h)
	{
		host->lock_quit();
		bool r = false;
		host->send(_strand, [&]
		{
			if (!h.ntfed())
			{
				h.set();
				r = true;
				_takeNtfQueue.erase(h.it);
			}
		});
		host->unlock_quit();
		return r;
	}

	/*!
	@brief 关闭消息通道，所有执行者将抛出 close_exception 异常
	*/
	void close(my_actor* host)
	{
		host->lock_quit();
		host->send(_strand, [this]
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
			while (!_takeNtfQueue.empty())
			{
				_takeNtfQueue.front().set();
				_takeNtfQueue.pop_front();
			}
		});
		host->unlock_quit();
	}

	/*!
	@brief close 后重置
	*/
	void reset()
	{
		assert(_closed);
		_closed = false;
	}
private:
	template <typename CT>
	bool try_take_ct(my_actor* host, const CT& ct)
	{
		host->lock_quit();
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
					ct(wt.can_move, wt.src_msg);
					wt.notified = true;
					wt.ntf(false);
					_sendWait.pop_back();
				}
			}
		});
		if (closed)
		{
			host->unlock_quit();
			throw close_exception();
		}
		host->unlock_quit();
		return ok;
	}

	template <typename CT>
	bool timed_take_ct(int tm, my_actor* host, const CT& ct)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		trig_notifer<bool> ntf;
		typename msg_list<take_wait>::iterator mit;
		__space_align char msgBuf[sizeof(RT)];
		bool wait = false;
		bool closed = false;
		bool notified = false;
		bool can_move = false;
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
					_takeWait.push_front({ &can_move, notified, msgBuf, ntf });
					_takeWait.front().ntf = host->make_trig_notifer_to_self(ath);
					mit = _takeWait.begin();
				}
				else
				{
					send_wait& wt = _sendWait.back();
					ct(wt.can_move, wt.src_msg);
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
						_takeWait.erase(mit);
					}
				});
				ok = notified;
				if (notified)
				{
					closed = host->wait_trig(ath);
				}
			}
			if (notified && !closed)
			{
				ct(can_move, *(RT*)msgBuf);
				((RT*)msgBuf)->~RT();
				ntf(false);
			}
		}
		if (closed)
		{
			host->unlock_quit();
			throw close_exception();
		}
		host->unlock_quit();
		return ok;
	}
private:
	shared_strand _strand;
	msg_list<take_wait> _takeWait;
	msg_list<send_wait> _sendWait;
	msg_list<NtfHandle_> _takeNtfQueue;
	bool _closed;
	NONE_COPY(sync_msg);
};

/*!
@brief csp函数调用，不直接使用
*/
template <typename T, typename R>
class CspChannel_
{
	struct send_wait
	{
		bool& notified;
		T& srcMsg;
		char* res;
		trig_notifer<bool> ntf;
	};

	struct take_wait
	{
		bool& notified;
		T*& srcMsg;
		char*& res;
		trig_notifer<bool>& ntfSend;
		trig_notifer<bool> ntf;
	};
protected:
	CspChannel_(const shared_strand& strand)
		:_closed(false), _strand(strand), _takeWait(4), _sendWait(4)
	{
		DEBUG_OPERATION(_thrownCloseExp = false);
	}
	~CspChannel_() {}
public:
	class ntf_handle
	{
		friend CspChannel_;
	public:
		ntf_handle() {}
	private:
		shared_bool sign;
		msg_list<NtfHandle_>::iterator it;
		RVALUE_COPY_CONSTRUCTION(ntf_handle, sign, it)
	};
protected:
	/*!
	@brief 开始准备执行对方的一个函数，执行完毕后返回结果
	@return 返回结果
	*/
	template <typename TM>
	R send(my_actor* host, TM&& msg)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		__space_align char resBuf[sizeof(R)];
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
					_sendWait.push_front({ notified, msg, resBuf });
					_sendWait.front().ntf = host->make_trig_notifer_to_self(ath);
					if (!_takeNtfQueue.empty())
					{
						_takeNtfQueue.front()();
						_takeNtfQueue.pop_front();
					}
				}
				else
				{
					take_wait& wt = _takeWait.back();
					wt.srcMsg = &msg;
					wt.notified = true;
					wt.res = resBuf;
					wt.ntfSend = host->make_trig_notifer_to_self(ath);
					wt.ntf(false);
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
			host->unlock_quit();
			throw_close_exception();
		}
		BREAK_OF_SCOPE(
		{
			typedef R TP_;
			((TP_*)resBuf)->~TP_();
		});
		host->unlock_quit();
		return std::move(*(R*)resBuf);
	}

	/*!
	@brief 尝试执行对方的一个函数，如果对方在等待执行就成功，失败抛出 try_invoke_exception 异常
	@return 成功返回结果
	*/
	template <typename TM>
	R try_send(my_actor* host, TM&& msg)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		__space_align char resBuf[sizeof(R)];
		bool closed = false;
		bool notified = false;
		bool has = false;
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
					has = true;
					take_wait& wt = _takeWait.back();
					wt.srcMsg = &msg;
					wt.notified = true;
					wt.res = resBuf;
					wt.ntfSend = host->make_trig_notifer_to_self(ath);
					wt.ntf(false);
					_takeWait.pop_back();
				}
				else if (!_takeNtfQueue.empty())
				{
					has = true;
					_sendWait.push_front({ notified, msg, resBuf });
					_sendWait.front().ntf = host->make_trig_notifer_to_self(ath);
					_takeNtfQueue.front()();
					_takeNtfQueue.pop_front();
				}
			}
		});
		if (!closed)
		{
			if (!has)
			{
				host->unlock_quit();
				throw_try_send_exception();
			}
			closed = host->wait_trig(ath);
		}
		if (closed)
		{
			host->unlock_quit();
			throw_close_exception();
		}
		BREAK_OF_SCOPE(
		{
			typedef R TP_;
			((TP_*)resBuf)->~TP_();
		});
		host->unlock_quit();
		return std::move(*(R*)resBuf);
	}

	/*!
	@brief 尝试执行对方的一个函数，如果对方在一定时间内提供函数就成功，失败抛出 timed_invoke_exception 异常
	@return 成功返回结果
	*/
	template <typename TM>
	R timed_send(int tm, my_actor* host, TM&& msg)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		__space_align char resBuf[sizeof(R)];
		typename msg_list<send_wait>::iterator nit;
		bool closed = false;
		bool notified = false;
		bool has = false;
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
					_sendWait.push_front({ notified, msg, resBuf });
					_sendWait.front().ntf = host->make_trig_notifer_to_self(ath);
					nit = _sendWait.begin();
					if (!_takeNtfQueue.empty())
					{
						_takeNtfQueue.front()();
						_takeNtfQueue.pop_front();
						has = true;
					}
				}
				else
				{
					has = true;
					take_wait& wt = _takeWait.back();
					wt.srcMsg = &msg;
					wt.notified = true;
					wt.res = resBuf;
					wt.ntfSend = host->make_trig_notifer_to_self(ath);
					wt.ntf(false);
					_takeWait.pop_back();
				}
			}
		});
		if (!closed)
		{
			if (!has)
			{
				if (!host->timed_wait_trig(tm, ath, closed))
				{
					host->send(_strand, [&]
					{
						if (notified)
						{
							has = true;
						}
						else
						{
							_sendWait.erase(nit);
						}
					});
					if (!has)
					{
						host->unlock_quit();
						throw_timed_send_exception();
					}
				}
			}
			if (has)
			{
				closed = host->wait_trig(ath);
			}
		}
		if (closed)
		{
			host->unlock_quit();
			throw_close_exception();
		}
		BREAK_OF_SCOPE(
		{
			typedef R TP_;
			((TP_*)resBuf)->~TP_();
		});
		host->unlock_quit();
		return std::move(*(R*)resBuf);
	}

	/*!
	@brief 等待对方执行一个函数体，函数体内返回对方需要的执行结果
	*/
	template <typename Func>
	void take(my_actor* host, Func&& h)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		trig_notifer<bool> ntfSend;
		T* srcMsg = NULL;
		char* res = NULL;
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
					_takeWait.push_front({ notified, srcMsg, res, ntfSend });
					_takeWait.front().ntf = host->make_trig_notifer_to_self(ath);
				}
				else
				{
					send_wait& wt = _sendWait.back();
					srcMsg = &wt.srcMsg;
					wt.notified = true;
					ntfSend = std::move(wt.ntf);
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
			BREAK_OF_SCOPE({ ntfSend(!ok); });
			BEGIN_TRY_
			{
				new(res)R(h(*srcMsg));
			}
			CATCH_FOR(sync_csp_close_exception)
			{
				assert(_thrownCloseExp);
				DEBUG_OPERATION(_thrownCloseExp = false);
				host->unlock_quit();
				throw_close_exception();
			}
			END_TRY_;
			ok = true;
			host->unlock_quit();
			return;
		}
		host->unlock_quit();
		throw_close_exception();
	}

	/*!
	@brief 尝试执行一个函数体，如果对方在准备执行则成功，否则抛出 try_wait_exception 异常
	*/
	template <typename Func>
	void try_take(my_actor* host, Func&& h)
	{
		host->lock_quit();
		trig_notifer<bool> ntfSend;
		T* srcMsg = NULL;
		char* res = NULL;
		bool closed = false;
		bool has = false;
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
					has = true;
					send_wait& wt = _sendWait.back();
					srcMsg = &wt.srcMsg;
					wt.notified = true;
					ntfSend = std::move(wt.ntf);
					res = wt.res;
					_sendWait.pop_back();
				}
			}
		});
		if (closed)
		{
			host->unlock_quit();
			throw_close_exception();
		}
		if (!has)
		{
			host->unlock_quit();
			throw_try_take_exception();
		}
		bool ok = false;
		BREAK_OF_SCOPE({ ntfSend(!ok); });
		BEGIN_TRY_
		{
			new(res)R(h(*srcMsg));
		}
		CATCH_FOR(sync_csp_close_exception)
		{
			assert(_thrownCloseExp);
			DEBUG_OPERATION(_thrownCloseExp = false);
			host->unlock_quit();
			throw_close_exception();
		}
		END_TRY_;
		ok = true;
		host->unlock_quit();
	}

	/*!
	@brief 尝试执行一个函数体，如果对方在一定时间内执行则成功，否则抛出 timed_wait_exception 异常
	*/
	template <typename Func>
	void timed_take(int tm, my_actor* host, Func&& h)
	{
		host->lock_quit();
		trig_handle<bool> ath;
		trig_notifer<bool> ntfSend;
		typename msg_list<take_wait>::iterator wit;
		T* srcMsg = NULL;
		char* res = NULL;
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
					_takeWait.push_front({ notified, srcMsg, res, ntfSend });
					_takeWait.front().ntf = host->make_trig_notifer_to_self(ath);
					wit = _takeWait.begin();
				}
				else
				{
					send_wait& wt = _sendWait.back();
					srcMsg = &wt.srcMsg;
					wt.notified = true;
					ntfSend = std::move(wt.ntf);
					res = wt.res;
					_sendWait.pop_back();
				}
			}
		});
		if (!closed)
		{
			if (wait)
			{
				if (!host->timed_wait_trig(tm, ath, closed))
				{
					host->send(_strand, [&]
					{
						wait = notified;
						if (!notified)
						{
							_takeWait.erase(wit);
						}
					});
					if (wait)
					{
						closed = host->wait_trig(ath);
					}
					if (!wait && !closed)
					{
						host->unlock_quit();
						throw_timed_take_exception();
					}
				}
			}
		}
		if (closed)
		{
			host->unlock_quit();
			throw_close_exception();
		}
		bool ok = false;
		BREAK_OF_SCOPE({ ntfSend(!ok); });
		BEGIN_TRY_
		{
			new(res)R(h(*srcMsg));
		}
		CATCH_FOR(sync_csp_close_exception)
		{
			assert(_thrownCloseExp);
			DEBUG_OPERATION(_thrownCloseExp = false);
			host->unlock_quit();
			throw_close_exception();
		}
		END_TRY_;
		ok = true;
		host->unlock_quit();
	}
public:
	/*!
	@brief 注册一个take消息通知，只触发一次
	*/
	template <typename Ntf>
	void regist_take_ntf(my_actor* host, Ntf&& ntf)
	{
		host->lock_quit();
		host->send(_strand, [&]
		{
			if (!_sendWait.empty())
			{
				ntf();
			}
			else
			{
				_takeNtfQueue.push_back(NtfHandle_(shared_bool(), (Ntf&&)ntf));
			}
		});
		host->unlock_quit();
	}

	/*!
	@brief 注册一个take消息通知，只触发一次，返回注册句柄
	*/
	template <typename Ntf>
	ntf_handle regist_take_ntf2(my_actor* host, Ntf&& ntf)
	{
		host->lock_quit();
		ntf_handle h;
		host->send(_strand, [&]
		{
			if (!_sendWait.empty())
			{
				ntf();
			}
			else
			{
				h.sign = shared_bool::new_(false);
				_takeNtfQueue.push_back(NtfHandle_(h.sign, (Ntf&&)ntf));
				h.it = --_takeNtfQueue.end();
			}
		});
		host->unlock_quit();
		return h;
	}

	/*!
	@brief 移除通知句柄，成功返回true
	*/
	bool remove_ntf(my_actor* host, ntf_handle& h)
	{
		host->lock_quit();
		bool r = false;
		host->send(_strand, [&]
		{
			if (!h.ntfed())
			{
				h.set();
				r = true;
				_takeNtfQueue.erase(h.it);
			}
		});
		host->unlock_quit();
		return r;
	}

	/*!
	@brief 关闭执行通道，所有执行抛出 close_exception 异常
	*/
	void close(my_actor* host)
	{
		host->lock_quit();
		host->send(_strand, [this]
		{
			_closed = true;
			while (!_takeWait.empty())
			{
				auto& wt = _takeWait.front();
				wt.notified = true;
				wt.ntf(true);
				assert(wt.ntfSend.empty());
				_takeWait.pop_front();
			}
			while (!_sendWait.empty())
			{
				auto& st = _sendWait.front();
				st.notified = true;
				st.ntf(true);
				_sendWait.pop_front();
			}
			while (!_takeNtfQueue.empty())
			{
				_takeNtfQueue.front().set();
				_takeNtfQueue.pop_front();
			}
		});
		host->unlock_quit();
	}

	/*!
	@brief close 后重置
	*/
	void reset()
	{
		assert(_closed);
		_closed = false;
		DEBUG_OPERATION(_thrownCloseExp = false);
	}
private:
	virtual void throw_close_exception() = 0;
	virtual void throw_try_send_exception() = 0;
	virtual void throw_timed_send_exception() = 0;
	virtual void throw_try_take_exception() = 0;
	virtual void throw_timed_take_exception() = 0;
private:
	shared_strand _strand;
	msg_list<take_wait> _takeWait;
	msg_list<send_wait> _sendWait;
	msg_list<NtfHandle_> _takeNtfQueue;
	bool _closed;
protected:
	DEBUG_OPERATION(bool _thrownCloseExp);
	NONE_COPY(CspChannel_);
};

//////////////////////////////////////////////////////////////////////////

struct VoidReturn_
{
};

template <typename T = void>
struct ReturnType_
{
	typedef TYPE_PIPE(T) type;
};

template <>
struct ReturnType_<void>
{
	typedef VoidReturn_ type;
};


template <typename R = void>
struct CspInvokeBaseFunc_
{
	typedef R&& result_type;

	template <typename H, typename TUPLE>
	static inline R invoke(H& h, TUPLE&& t)
	{
		return tuple_invoke<R>(h, TRY_MOVE(t));
	}
};

template <>
struct CspInvokeBaseFunc_<void>
{
	typedef void result_type;

	template <typename H, typename TUPLE>
	static inline VoidReturn_ invoke(H& h, TUPLE&& t)
	{
		tuple_invoke(h, TRY_MOVE(t));
		return VoidReturn_();
	}
};

template <typename R = void, typename... ARGS>
class CspInvokeBase_ : public CspChannel_<typename std::tuple<ARGS&...>, typename ReturnType_<R>::type>
{
	typedef std::tuple<ARGS&...> msg_type;
	typedef typename ReturnType_<R>::type return_type;
	typedef CspChannel_<msg_type, return_type> base_csp_channel;

protected:
	CspInvokeBase_(const shared_strand& strand)
		:base_csp_channel(strand) {}
	~CspInvokeBase_() {}
public:
	template <typename Func>
	void wait_invoke(my_actor* host, Func&& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->return_type
		{
			return CspInvokeBaseFunc_<R>::invoke(h, msg);
		});
	}

	template <typename Func>
	void try_wait_invoke(my_actor* host, Func&& h)
	{
		base_csp_channel::try_take(host, [&](msg_type& msg)->return_type
		{
			return CspInvokeBaseFunc_<R>::invoke(h, msg);
		});
	}

	template <typename Func>
	void timed_wait_invoke(int tm, my_actor* host, Func&& h)
	{
		base_csp_channel::timed_take(tm, host, [&](msg_type& msg)->return_type
		{
			return CspInvokeBaseFunc_<R>::invoke(h, msg);
		});
	}

	template <typename... Args>
	R invoke(my_actor* host, Args&&... args)
	{
		return (typename CspInvokeBaseFunc_<R>::result_type)base_csp_channel::send(host, std::tuple<Args&...>(args...));
	}

	template <typename... Args>
	R invoke_rval(my_actor* host, Args&&... args)
	{
		return try_rval_invoke<R>([&](ARGS&... nargs)
		{
			return invoke(host, nargs...);
		}, TRY_MOVE(args)...);
	}

	template <typename... Args>
	R try_invoke(my_actor* host, Args&&... args)
	{
		return (typename CspInvokeBaseFunc_<R>::result_type)base_csp_channel::try_send(host, std::tuple<Args&...>(args...));
	}

	template <typename... Args>
	R try_invoke_rval(my_actor* host, Args&&... args)
	{
		return try_rval_invoke<R>([&](ARGS&... nargs)
		{
			return try_invoke(host, nargs...);
		}, TRY_MOVE(args)...);
	}

	template <typename... Args>
	R timed_invoke(int tm, my_actor* host, Args&&... args)
	{
		return (typename CspInvokeBaseFunc_<R>::result_type)base_csp_channel::timed_send(tm, host, std::tuple<Args&...>(args...));
	}

	template <typename... Args>
	R timed_invoke_rval(int tm, my_actor* host, Args&&... args)
	{
		return try_rval_invoke<R>([&](ARGS&... nargs)
		{
			return timed_invoke(tm, host, nargs...);
		}, TRY_MOVE(args)...);
	}
};

template <typename R, typename... ARGS>
class csp_invoke;

/*!
@brief CSP模型消息（多读多写，角色可转换，可递归），发送方等消息取出并处理完成后才返回
*/
template <typename R, typename... ARGS>
class csp_invoke
	<R(ARGS...)>: public CspInvokeBase_<R, ARGS...>
{
	typedef CspInvokeBase_<R, ARGS...> Parent;
public:
	struct close_exception : public csp_channel_close_exception {};
	struct try_invoke_exception : public csp_try_invoke_exception {};
	struct timed_invoke_exception : public csp_timed_invoke_exception {};
	struct try_wait_exception : public csp_try_wait_exception {};
	struct timed_wait_exception : public csp_timed_wait_exception {};
public:
	csp_invoke(const shared_strand& strand)
		:Parent(strand) {}
private:
	void throw_close_exception()
	{
		assert(!Parent::_thrownCloseExp);
		DEBUG_OPERATION(Parent::_thrownCloseExp = true);
		throw close_exception();
	}

	void throw_try_send_exception()
	{
		throw try_invoke_exception();
	}

	void throw_timed_send_exception()
	{
		throw timed_invoke_exception();
	}

	void throw_try_take_exception()
	{
		throw try_wait_exception();
	}

	void throw_timed_take_exception()
	{
		throw timed_wait_exception();
	}
};

#endif