#ifndef __SYNC_MSG_H
#define __SYNC_MSG_H

#include "actor_framework.h"
#include "msg_queue.h"
#include "try_move.h"

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

/*!
@brief 同步发送消息（多读多写，角色可转换），发送方等到消息取出才返回
*/
template <typename T>
class sync_msg
{
	typedef RM_REF(T) ST;

	template <typename T>
	struct result_type
	{
		typedef T type;
	};

	template <typename T>
	struct result_type<T&>
	{
		typedef std::reference_wrapper<T> type;
	};

	template <typename T>
	struct result_type<T&&>
	{
		typedef T type;
	};

	typedef typename result_type<T>::type RT;

	//////////////////////////////////////////////////////////////////////////
	struct send_wait
	{
		bool can_move;
		bool& notified;
		ST& src_msg;
		actor_trig_notifer<bool> ntf;
	};

	struct take_wait
	{
		bool* can_move;
		bool& notified;
		unsigned char* dst;
		actor_trig_notifer<bool> ntf;
		actor_trig_notifer<bool>& takeOk;
	};
public:
	struct close_exception : public sync_msg_close_exception {};
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
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		bool closed = false;
		bool notified = false;
		LAMBDA_THIS_REF5(ref5, host, msg, ath, closed, notified);
		host->send(_strand, [&ref5]
		{
			if (ref5->_closed)
			{
				ref5.closed = true;
			} 
			else
			{
				auto& _takeWait = ref5->_takeWait;
				if (_takeWait.empty())
				{
					send_wait pw = { try_move<TM&&>::can_move, ref5.notified, (ST&)ref5.msg, ref5.host->make_trig_notifer(ref5.ath) };
					ref5->_sendWait.push_front(pw);
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)RT(SendMove_<T, TM&&>::move(ref5.msg));
					if (wt.can_move)
					{
						*wt.can_move = try_move<TM&&>::can_move || !(std::is_reference<T>::value);
					}
					wt.notified = true;
					wt.takeOk = ref5.host->make_trig_notifer(ref5.ath);
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
			qg.unlock();
			throw close_exception();
		}
	}

	/*!
	@brief 尝试同步发送一条消息，如果对方在等待消息则成功
	@return 成功返回true
	*/
	template <typename TM>
	bool try_send(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		bool ok = false;
		bool closed = false;
		LAMBDA_THIS_REF5(ref5, host, msg, ath, ok, closed);
		host->send(_strand, [&ref5]
		{
			if (ref5->_closed)
			{
				ref5.closed = true;
			} 
			else
			{
				auto& _takeWait = ref5->_takeWait;
				if (!_takeWait.empty())
				{
					ref5.ok = true;
					take_wait& wt = _takeWait.back();
					new(wt.dst)RT(SendMove_<T, TM&&>::move(ref5.msg));
					if (wt.can_move)
					{
						*wt.can_move = try_move<TM&&>::can_move || !(std::is_reference<T>::value);
					}
					wt.takeOk = ref5.host->make_trig_notifer(ref5.ath);
					wt.ntf(false);
					_takeWait.pop_back();
				}
			}
		});
		if (!closed && ok)
		{
			closed = host->wait_trig(ath);
		}
		if (closed)
		{
			qg.unlock();
			throw close_exception();
		}
		return ok;
	}

	/*!
	@brief 尝试同步发送一条消息，对方在一定时间内取出则成功
	@return 成功返回true
	*/
	template <typename TM>
	bool timed_send(int tm, my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		msg_list<send_wait>::iterator mit;
		bool closed = false;
		bool notified = false;
		LAMBDA_THIS_REF6(ref6, host, msg, ath, mit, closed, notified);
		host->send(_strand, [&ref6]
		{
			if (ref6->_closed)
			{
				ref6.closed = true;
			} 
			else
			{
				auto& _takeWait = ref6->_takeWait;
				if (_takeWait.empty())
				{
					send_wait pw = { try_move<TM&&>::can_move, ref6.notified, (T&)ref6.msg, ref6.host->make_trig_notifer(ref6.ath) };
					ref6->_sendWait.push_front(pw);
					ref6.mit = ref6->_sendWait.begin();
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)RT(SendMove_<T, TM&&>::move(ref6.msg));
					if (wt.can_move)
					{
						*wt.can_move = try_move<TM&&>::can_move || !(std::is_reference<T>::value);
					}
					wt.notified = true;
					wt.takeOk = ref6.host->make_trig_notifer(ref6.ath);
					wt.ntf(false);
					_takeWait.pop_back();
				}
			}
		});
		if (!closed)
		{
			if (!host->timed_wait_trig(tm, ath, closed))
			{
				host->send(_strand, [&ref6]
				{
					if (!ref6.notified)
					{
						ref6->_sendWait.erase(ref6.mit);
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
			qg.unlock();
			throw close_exception();
		}
		return notified;
	}

	/*!
	@brief 取出一条消息，直到有消息过来才返回
	*/
	RT take(my_actor* host)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		actor_trig_notifer<bool> ntf;
		unsigned char msgBuf[sizeof(RT)];
		bool wait = false;
		bool closed = false;
		bool notified = false;
		LAMBDA_THIS_REF7(ref6, host, ath, ntf, msgBuf, wait, closed, notified);
		host->send(_strand, [&ref6]
		{
			if (ref6->_closed)
			{
				ref6.closed = true;
			} 
			else
			{
				auto& _sendWait = ref6->_sendWait;
				if (_sendWait.empty())
				{
					ref6.wait = true;
					take_wait pw = { NULL, ref6.notified, ref6.msgBuf, ref6.host->make_trig_notifer(ref6.ath), ref6.ntf };
					ref6->_takeWait.push_front(pw);
				}
				else
				{
					send_wait& wt = _sendWait.back();
					new(ref6.msgBuf)RT(wt.can_move ? TakeMove_<T>::move(wt.src_msg) : wt.src_msg);
					wt.notified = true;
					ref6.ntf = wt.ntf;
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
			AUTO_CALL(
			{
				typedef RT TP_;
				((TP_*)msgBuf)->~TP_();
				ntf(false);
			});
			return std::move((RM_REF(RT)&)*(RT*)msgBuf);
		}
		qg.unlock();
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
	@brief 关闭消息通道，所有执行者将抛出 close_exception 异常
	*/
	void close(my_actor* host)
	{
		my_actor::quit_guard qg(host);
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
		});
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
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		bool ok = false;
		bool closed = false;
		LAMBDA_THIS_REF5(ref5, host, ct, ath, ok, closed);
		host->send(_strand, [&ref5]
		{
			if (ref5->_closed)
			{
				ref5.closed = true;
			}
			else
			{
				auto& _sendWait = ref5->_sendWait;
				if (!_sendWait.empty())
				{
					ref5.ok = true;
					send_wait& wt = _sendWait.back();
					ref5.ct(wt.can_move, wt.src_msg);
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
		actor_trig_notifer<bool> ntf;
		msg_list<take_wait>::iterator mit;
		unsigned char msgBuf[sizeof(RT)];
		bool wait = false;
		bool closed = false;
		bool notified = false;
		bool can_move = false;
		LAMBDA_REF5(ref4, wait, closed, notified, can_move, ntf);
		LAMBDA_THIS_REF6(ref6, ref4, host, ct, ath, mit, msgBuf);
		host->send(_strand, [&ref6]
		{
			auto& ref4 = ref6.ref9;
			if (ref6->_closed)
			{
				ref4.closed = true;
			}
			else
			{
				auto& _sendWait = ref6->_sendWait;
				if (_sendWait.empty())
				{
					ref4.wait = true;
					take_wait pw = { &ref4.can_move, ref4.notified, ref6.msgBuf, ref6.host->make_trig_notifer(ref6.ath), ref4.ntf };
					ref6->_takeWait.push_front(pw);
					ref6.mit = ref6->_takeWait.begin();
				}
				else
				{
					send_wait& wt = _sendWait.back();
					ref6.ct(wt.can_move, wt.src_msg);
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
				host->send(_strand, [&ref6]
				{
					if (!ref6.ref9.notified)
					{
						ref6->_takeWait.erase(ref6.mit);
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
@brief csp函数调用，不直接使用
*/
template <typename T, typename R>
class CspChannel_
{
	struct send_wait
	{
		bool& notified;
		T& srcMsg;
		unsigned char* res;
		actor_trig_notifer<bool> ntf;
	};

	struct take_wait
	{
		bool& notified;
		T*& srcMsg;
		unsigned char*& res;
		actor_trig_notifer<bool> ntf;
		actor_trig_notifer<bool>& ntfSend;
	};
protected:
	CspChannel_(const shared_strand& strand)
		:_closed(false), _strand(strand), _takeWait(4), _sendWait(4)
	{
		DEBUG_OPERATION(_thrownCloseExp = false);
	}
	~CspChannel_() {}
public:
	/*!
	@brief 开始准备执行对方的一个函数，执行完毕后返回结果
	@return 返回结果
	*/
	template <typename TM>
	R send(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		unsigned char resBuf[sizeof(R)];
		bool closed = false;
		bool notified = false;
		LAMBDA_THIS_REF6(ref6, host, msg, ath, resBuf, closed, notified);
		host->send(_strand, [&ref6]
		{
			if (ref6->_closed)
			{
				ref6.closed = true;
			}
			else
			{
				auto& _takeWait = ref6->_takeWait;
				if (_takeWait.empty())
				{
					send_wait pw = { ref6.notified, ref6.msg, ref6.resBuf, ref6.host->make_trig_notifer(ref6.ath) };
					ref6->_sendWait.push_front(pw);
				}
				else
				{
					take_wait& wt = _takeWait.back();
					wt.srcMsg = &ref6.msg;
					wt.notified = true;
					wt.res = ref6.resBuf;
					wt.ntfSend = ref6.host->make_trig_notifer(ref6.ath);
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
			qg.unlock();
			throw_close_exception();
		}
		AUTO_CALL(
		{
			typedef R TP_;
			((TP_*)resBuf)->~TP_();
		});
		return std::move(*(R*)resBuf);
	}

	/*!
	@brief 尝试执行对方的一个函数，如果对方在等待执行就成功，失败抛出 try_invoke_exception 异常
	@return 成功返回结果
	*/
	template <typename TM>
	R try_send(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		unsigned char resBuf[sizeof(R)];
		bool closed = false;
		bool has = false;
		LAMBDA_THIS_REF6(ref6, host, msg, ath, resBuf, closed, has);
		host->send(_strand, [&ref6]
		{
			if (ref6->_closed)
			{
				ref6.closed = true;
			}
			else
			{
				auto& _takeWait = ref6->_takeWait;
				if (!_takeWait.empty())
				{
					ref6.has = true;
					take_wait& wt = _takeWait.back();
					wt.srcMsg = &ref6.msg;
					wt.notified = true;
					wt.res = ref6.resBuf;
					wt.ntfSend = ref6.host->make_trig_notifer(ref6.ath);
					wt.ntf(false);
					_takeWait.pop_back();
				}
			}
		});
		if (!closed)
		{
			if (!has)
			{
				qg.unlock();
				throw_try_send_exception();
			}
			closed = host->wait_trig(ath);
		}
		if (closed)
		{
			qg.unlock();
			throw_close_exception();
		}
		AUTO_CALL(
		{
			typedef R TP_;
			((TP_*)resBuf)->~TP_();
		});
		return std::move(*(R*)resBuf);
	}

	/*!
	@brief 尝试执行对方的一个函数，如果对方在一定时间内提供函数就成功，失败抛出 timed_invoke_exception 异常
	@return 成功返回结果
	*/
	template <typename TM>
	R timed_send(int tm, my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		unsigned char resBuf[sizeof(R)];
		msg_list<send_wait>::iterator nit;
		bool closed = false;
		bool notified = false;
		bool has = false;
		LAMBDA_THIS_REF8(ref8, host, msg, ath, resBuf, nit, closed, notified, has);
		host->send(_strand, [&ref8]
		{
			if (ref8->_closed)
			{
				ref8.closed = true;
			}
			else
			{
				auto& _takeWait = ref8->_takeWait;
				if (_takeWait.empty())
				{
					send_wait pw = { ref8.notified, ref8.msg, ref8.resBuf, ref8.host->make_trig_notifer(ref8.ath) };
					ref8->_sendWait.push_front(pw);
					ref8.nit = ref8->_sendWait.begin();
				}
				else
				{
					ref8.has = true;
					take_wait& wt = _takeWait.back();
					wt.srcMsg = &ref8.msg;
					wt.notified = true;
					wt.res = ref8.resBuf;
					wt.ntfSend = ref8.host->make_trig_notifer(ref8.ath);
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
					host->send(_strand, [&ref8]
					{
						if (ref8.notified)
						{
							ref8.has = true;
						}
						else
						{
							ref8->_sendWait.erase(ref8.nit);
						}
					});
					if (!has)
					{
						qg.unlock();
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
			qg.unlock();
			throw_close_exception();
		}
		AUTO_CALL(
		{
			typedef R TP_;
			((TP_*)resBuf)->~TP_();
		});
		return std::move(*(R*)resBuf);
	}

	/*!
	@brief 等待对方执行一个函数体，函数体内返回对方需要的执行结果
	*/
	template <typename H>
	void take(my_actor* host, const H& h)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		actor_trig_notifer<bool> ntfSend;
		T* srcMsg = NULL;
		unsigned char* res = NULL;
		bool wait = false;
		bool closed = false;
		bool notified = false;
		LAMBDA_THIS_REF8(ref8, host, ath, ntfSend, srcMsg, res, wait, closed, notified);
		host->send(_strand, [&ref8]
		{
			if (ref8->_closed)
			{
				ref8.closed = true;
			}
			else
			{
				auto& _sendWait = ref8->_sendWait;
				if (_sendWait.empty())
				{
					ref8.wait = true;
					take_wait pw = { ref8.notified, ref8.srcMsg, ref8.res, ref8.host->make_trig_notifer(ref8.ath), ref8.ntfSend };
					ref8->_takeWait.push_front(pw);
				}
				else
				{
					send_wait& wt = _sendWait.back();
					ref8.srcMsg = &wt.srcMsg;
					wt.notified = true;
					ref8.ntfSend = wt.ntf;
					ref8.res = wt.res;
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
			AUTO_CALL({ ntfSend(!ok); });
			BEGIN_TRY_
			{
				new(res)R(h(*srcMsg));
			}
			CATCH_FOR(sync_csp_close_exception)
			{
				assert(_thrownCloseExp);
				DEBUG_OPERATION(_thrownCloseExp = false);
				qg.unlock();
				throw_close_exception();
			}
			END_TRY_;
			ok = true;
			return;
		}
		qg.unlock();
		throw_close_exception();
	}

	/*!
	@brief 尝试执行一个函数体，如果对方在准备执行则成功，否则抛出 try_wait_exception 异常
	*/
	template <typename H>
	void try_take(my_actor* host, const H& h)
	{
		my_actor::quit_guard qg(host);
		actor_trig_notifer<bool> ntfSend;
		T* srcMsg = NULL;
		unsigned char* res = NULL;
		bool closed = false;
		bool has = false;
		LAMBDA_REF2(ref2, closed, has);
		LAMBDA_THIS_REF5(ref5, ref2, host, ntfSend, srcMsg, res);
		host->send(_strand, [&ref5]
		{
			auto& ref2 = ref5.ref2;
			if (ref5->_closed)
			{
				ref2.closed = true;
			}
			else
			{
				auto& _sendWait = ref5->_sendWait;
				if (!_sendWait.empty())
				{
					ref2.has = true;
					send_wait& wt = _sendWait.back();
					ref5.srcMsg = &wt.srcMsg;
					wt.notified = true;
					ref5.ntfSend = wt.ntf;
					ref5.res = wt.res;
					_sendWait.pop_back();
				}
			}
		});
		if (closed)
		{
			qg.unlock();
			throw_close_exception();
		}
		if (!has)
		{
			qg.unlock();
			throw_try_take_exception();
		}
		bool ok = false;
		AUTO_CALL({ ntfSend(!ok); });
		BEGIN_TRY_
		{
			new(res)R(h(*srcMsg));
		}
		CATCH_FOR(sync_csp_close_exception)
		{
			assert(_thrownCloseExp);
			DEBUG_OPERATION(_thrownCloseExp = false);
			qg.unlock();
			throw_close_exception();
		}
		END_TRY_;
		ok = true;
	}

	/*!
	@brief 尝试执行一个函数体，如果对方在一定时间内执行则成功，否则抛出 timed_wait_exception 异常
	*/
	template <typename H>
	void timed_take(int tm, my_actor* host, const H& h)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		actor_trig_notifer<bool> ntfSend;
		msg_list<take_wait>::iterator wit;
		T* srcMsg = NULL;
		unsigned char* res = NULL;
		bool wait = false;
		bool closed = false;
		bool notified = false;
		LAMBDA_THIS_REF9(ref9, wait, closed, notified, wit, host, ath, ntfSend, srcMsg, res);
		host->send(_strand, [&ref9]
		{
			if (ref9->_closed)
			{
				ref9.closed = true;
			}
			else
			{
				auto& _sendWait = ref9->_sendWait;
				if (_sendWait.empty())
				{
					ref9.wait = true;
					take_wait pw = { ref9.notified, ref9.srcMsg, ref9.res, ref9.host->make_trig_notifer(ref9.ath), ref9.ntfSend };
					ref9->_takeWait.push_front(pw);
					ref9.wit = ref9->_takeWait.begin();
				}
				else
				{
					send_wait& wt = _sendWait.back();
					ref9.srcMsg = &wt.srcMsg;
					wt.notified = true;
					ref9.ntfSend = wt.ntf;
					ref9.res = wt.res;
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
					host->send(_strand, [&ref9]
					{
						ref9.wait = ref9.notified;
						if (!ref9.notified)
						{
							ref9->_takeWait.erase(ref9.wit);
						}
					});
					if (wait)
					{
						closed = host->wait_trig(ath);
					}
					if (!wait && !closed)
					{
						qg.unlock();
						throw_timed_take_exception();
					}
				}
			}
		}
		if (closed)
		{
			qg.unlock();
			throw_close_exception();
		}
		bool ok = false;
		AUTO_CALL({ ntfSend(!ok); });
		BEGIN_TRY_
		{
			new(res)R(h(*srcMsg));
		}
		CATCH_FOR(sync_csp_close_exception)
		{
			assert(_thrownCloseExp);
			DEBUG_OPERATION(_thrownCloseExp = false);
			qg.unlock();
			throw_close_exception();
		}
		END_TRY_;
		ok = true;
	}

	/*!
	@brief 关闭执行通道，所有执行抛出 close_exception 异常
	*/
	void close(my_actor* host)
	{
		my_actor::quit_guard qg(host);
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
		});
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
	CspChannel_(const CspChannel_&){};
	void operator=(const CspChannel_&){};

	virtual void throw_close_exception() = 0;
	virtual void throw_try_send_exception() = 0;
	virtual void throw_timed_send_exception() = 0;
	virtual void throw_try_take_exception() = 0;
	virtual void throw_timed_take_exception() = 0;
private:
	bool _closed;
	shared_strand _strand;
	msg_list<take_wait> _takeWait;
	msg_list<send_wait> _sendWait;
protected:
	DEBUG_OPERATION(bool _thrownCloseExp);
};

//////////////////////////////////////////////////////////////////////////

struct VoidReturn_
{
};

template <typename T = void>
struct ReturnType_
{
	typedef T type;
};

template <>
struct ReturnType_<void>
{
	typedef VoidReturn_ type;
};

template <typename R = void, typename... ARGS>
class CspInvokeBase_ : public CspChannel_<typename std::tuple<RM_REF(ARGS)&...>, typename ReturnType_<R>::type>
{
	typedef std::tuple<RM_REF(ARGS)&...> msg_type;
	typedef typename ReturnType_<R>::type return_type;
	typedef CspChannel_<msg_type, return_type> base_csp_channel;

	template <typename R = void>
	struct func 
	{
		typedef RM_REF(R)& type;

		template <typename H, typename TUPLE>
		static inline R invoke(H&& h, TUPLE&& t)
		{
			return tuple_invoke<R>(TRY_MOVE(h), TRY_MOVE(t));
		}
	};

	template <>
	struct func<void>
	{
		typedef void type;

		template <typename H, typename TUPLE>
		static inline VoidReturn_ invoke(H&& h, TUPLE&& t)
		{
			tuple_invoke<void>(TRY_MOVE(h), TRY_MOVE(t));
			return VoidReturn_();
		}
	};
protected:
	CspInvokeBase_(const shared_strand& strand)
		:base_csp_channel(strand) {}
	~CspInvokeBase_() {}
public:
	template <typename H>
	void wait_invoke(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->return_type
		{
			return func<R>::invoke(h, msg);
		});
	}

	template <typename H>
	void try_wait_invoke(my_actor* host, const H& h)
	{
		base_csp_channel::try_take(host, [&](msg_type& msg)->return_type
		{
			return func<R>::invoke<R>(h, msg);
		});
	}

	template <typename H>
	void timed_wait_invoke(int tm, my_actor* host, const H& h)
	{
		base_csp_channel::timed_take(tm, host, [&](msg_type& msg)->return_type
		{
			return func<R>::invoke<R>(h, msg);
		});
	}

	template <typename... Args>
	typename R invoke(my_actor* host, Args&&... args)
	{
		return (typename func<R>::type)base_csp_channel::send(host, std::tuple<RM_REF(Args)&...>(args...));
	}

	template <typename... Args>
	typename R try_invoke(my_actor* host, Args&&... args)
	{
		return (typename func<R>::type)base_csp_channel::try_send(host, std::tuple<RM_REF(Args)&...>(args...));
	}

	template <typename... Args>
	typename R timed_invoke(int tm, my_actor* host, Args&&... args)
	{
		return (typename func<R>::type)base_csp_channel::timed_send(tm, host, std::tuple<RM_REF(Args)&...>(args...));
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
public:
	struct close_exception : public csp_channel_close_exception {};
	struct try_invoke_exception : public csp_try_invoke_exception {};
	struct timed_invoke_exception : public csp_timed_invoke_exception {};
	struct try_wait_exception : public csp_try_wait_exception {};
	struct timed_wait_exception : public csp_timed_wait_exception {};
public:
	csp_invoke(const shared_strand& strand)
		:CspInvokeBase_(strand) {}
private:
	void throw_close_exception()
	{
		assert(!_thrownCloseExp);
		DEBUG_OPERATION(_thrownCloseExp = true);
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