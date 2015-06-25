#ifndef __SYNC_MSG_H
#define __SYNC_MSG_H

#include "actor_framework.h"
#include "msg_queue.h"
#include "check_move.h"

struct sync_csp_close_exception {};

struct sync_msg_close_exception : public sync_csp_close_exception {};

struct csp_channel_close_exception : public sync_csp_close_exception {};

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
	struct close_exception : public sync_msg_close_exception {};
public:
	sync_msg(shared_strand strand)
		:_closed(false), _strand(strand), _sendWait(4), _takeWait(4) {}
public:
	template <typename TM>
	void send(my_actor* host, TM&& msg)
	{
		my_actor::quit_guard qg(host);
		actor_trig_handle<bool> ath;
		bool wait = false;
		bool closed = false;
		bool notified = false;
		LAMBDA_THIS_REF6(ref6, host, msg, ath, wait, closed, notified);
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
					ref6.wait = true;
					send_wait pw = { check_move<TM&&>::is_rvalue, ref6.notified, ref6.msg, ref6.host->make_trig_notifer(ref6.ath) };
					ref6->_sendWait.push_front(pw);
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)T(check_move<TM&&>::move(ref6.msg));
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
		LAMBDA_THIS_REF4(ref4, host, msg, ok, closed);
		host->send(_strand, [&ref4]
		{
			if (ref4->_closed)
			{
				ref4.closed = true;
			} 
			else
			{
				auto& _takeWait = ref4->_takeWait;
				if (!_takeWait.empty())
				{
					ref4.ok = true;
					take_wait& wt = _takeWait.back();
					new(wt.dst)T(check_move<TM&&>::move(ref4.msg));
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
		LAMBDA_REF3(ref3, wait, closed, notified);
		LAMBDA_THIS_REF5(ref5, ref3, host, msg, ath, mit);
		host->send(_strand, [&ref5]
		{
			auto& ref3 = ref5.ref3;
			if (ref5->_closed)
			{
				ref3.closed = true;
			} 
			else
			{
				auto& _takeWait = ref5->_takeWait;
				if (_takeWait.empty())
				{
					ref3.wait = true;
					send_wait pw = { check_move<TM&&>::is_rvalue, ref3.notified, ref5.msg, ref5.host->make_trig_notifer(ref5.ath) };
					ref5->_sendWait.push_front(pw);
					ref5.mit = ref5->_sendWait.begin();
				}
				else
				{
					take_wait& wt = _takeWait.back();
					new(wt.dst)T(check_move<TM&&>::move(ref5.msg));
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
				host->send(_strand, [&ref5]
				{
					if (!ref5.ref3.notified)
					{
						ref5.ref3.wait = false;
						ref5->_sendWait.erase(ref5.mit);
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
		LAMBDA_THIS_REF6(ref6, host, ath, msgBuf, wait, closed, notified);
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
					take_wait pw = { ref6.notified, ref6.msgBuf, ref6.host->make_trig_notifer(ref6.ath) };
					ref6->_takeWait.push_front(pw);
				}
				else
				{
					send_wait& wt = _sendWait.back();
					new(ref6.msgBuf)T(wt.is_rvalue ? std::move(wt.src_msg) : wt.src_msg);
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
			AUTO_CALL(
			{
				typedef T TP_;
				((TP_*)msgBuf)->~TP_();
			});
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
					ref5.ct(wt.is_rvalue, wt.src_msg);
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
		LAMBDA_REF3(ref3, wait, closed, notified);
		LAMBDA_THIS_REF6(ref6, ref3, host, ct, ath, mit, msgBuf);
		host->send(_strand, [&ref6]
		{
			auto& ref3 = ref6.ref3;
			if (ref6->_closed)
			{
				ref3.closed = true;
			}
			else
			{
				auto& _sendWait = ref6->_sendWait;
				if (_sendWait.empty())
				{
					ref3.wait = true;
					take_wait pw = { ref3.notified, ref6.msgBuf, ref6.host->make_trig_notifer(ref6.ath) };
					ref6->_takeWait.push_front(pw);
					ref6.mit = ref6->_takeWait.begin();
				}
				else
				{
					send_wait& wt = _sendWait.back();
					ref6.ct(wt.is_rvalue, wt.src_msg);
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
					if (!ref6.ref3.notified)
					{
						ref6.ref3.wait = false;
						ref6->_takeWait.erase(ref6.mit);
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

template <typename T, typename R>
class csp_channel
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
	csp_channel(shared_strand strand)
		:_closed(false), _strand(strand), _takeWait(4), _sendWait(4) {}
	~csp_channel() {}
public:
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
					wt.ntf(false);
					wt.res = ref6.resBuf;
					wt.ntfSend = ref6.host->make_trig_notifer(ref6.ath);
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
		LAMBDA_REF3(ref3, wait, closed, notified);
		LAMBDA_THIS_REF6(ref6, ref3, host, ath, ntfSend, srcMsg, res);
		host->send(_strand, [&ref6]
		{
			auto& ref3 = ref6.ref3;
			if (ref6->_closed)
			{
				ref3.closed = true;
			}
			else
			{
				auto& _sendWait = ref6->_sendWait;
				if (_sendWait.empty())
				{
					ref3.wait = true;
					take_wait pw = { ref3.notified, ref6.srcMsg, ref6.res, ref6.host->make_trig_notifer(ref6.ath), ref6.ntfSend };
					ref6->_takeWait.push_front(pw);
				}
				else
				{
					send_wait& wt = _sendWait.back();
					ref6.srcMsg = &wt.srcMsg;
					wt.notified = true;
					ref6.ntfSend = wt.ntf;
					ref6.res = wt.res;
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
// 				if (!wt.ntfSend.empty())
// 				{
// 					wt.ntfSend(true);
// 				}
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

	virtual void throw_close_exception() = 0;
private:
	bool _closed;
	shared_strand _strand;
	msg_list<take_wait> _takeWait;
	msg_list<send_wait> _sendWait;
};

//////////////////////////////////////////////////////////////////////////

template <typename T0, typename T1, typename T2, typename T3, typename R = void>
class csp_invoke_base4 : public csp_channel<ref_ex<T0, T1, T2, T3>, R>
{
	typedef ref_ex<T0, T1, T2, T3> msg_type;
	typedef csp_channel<msg_type, R> base_csp_channel;
protected:
	csp_invoke_base4(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base4() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->R
		{
			return h(msg._p0, msg._p1, msg._p2, msg._p3);
		});
	}

	template <typename TM0, typename TM1, typename TM2, typename TM3>
	R invoke(my_actor* host, TM0&& p0, TM1&& p1, TM2&& p2, TM3&& p3)
	{
		return base_csp_channel::send(host, bind_ref(p0, p1, p2, p3));
	}
};

template <typename T0, typename T1, typename T2, typename R = void>
class csp_invoke_base3 : public csp_channel<ref_ex<T0, T1, T2>, R>
{
	typedef ref_ex<T0, T1, T2> msg_type;
	typedef csp_channel<msg_type, R> base_csp_channel;
protected:
	csp_invoke_base3(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base3() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->R
		{
			return h(msg._p0, msg._p1, msg._p2);
		});
	}

	template <typename TM0, typename TM1, typename TM2>
	R invoke(my_actor* host, TM0&& p0, TM1&& p1, TM2&& p2)
	{
		return base_csp_channel::send(host, bind_ref(p0, p1, p2));
	}
};

template <typename T0, typename T1, typename R = void>
class csp_invoke_base2 : public csp_channel<ref_ex<T0, T1>, R>
{
	typedef ref_ex<T0, T1> msg_type;
	typedef csp_channel<msg_type, R> base_csp_channel;
protected:
	csp_invoke_base2(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base2() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->R
		{
			return h(msg._p0, msg._p1);
		});
	}

	template <typename TM0, typename TM1>
	R invoke(my_actor* host, TM0&& p0, TM1&& p1)
	{
		return base_csp_channel::send(host, bind_ref(p0, p1));
	}
};

template <typename T0, typename R = void>
class csp_invoke_base1 : public csp_channel<ref_ex<T0>, R>
{
	typedef ref_ex<T0> msg_type;
	typedef csp_channel<msg_type, R> base_csp_channel;
protected:
	csp_invoke_base1(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base1() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->R
		{
			return h(msg._p0);
		});
	}

	template <typename TM0>
	R invoke(my_actor* host, TM0&& p0)
	{
		return base_csp_channel::send(host, bind_ref(p0));
	}
};

template <typename R = void>
class csp_invoke_base0 : public csp_channel<ref_ex<>, R>
{
	typedef ref_ex<> msg_type;
	typedef csp_channel<msg_type, R> base_csp_channel;
protected:
	csp_invoke_base0(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base0() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->R
		{
			return h();
		});
	}

	R invoke(my_actor* host)
	{
		return base_csp_channel::send(host, msg_type());
	}
};

struct void_return
{
};

template <typename T0, typename T1, typename T2, typename T3>
class csp_invoke_base4<T0, T1, T2, T3, void> : public csp_channel<ref_ex<T0, T1, T2, T3>, void_return>
{
	typedef ref_ex<T0, T1, T2, T3> msg_type;
	typedef csp_channel<msg_type, void_return> base_csp_channel;
protected:
	csp_invoke_base4(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base4() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->void_return
		{
			h(msg._p0, msg._p1, msg._p2, msg._p3);
			return void_return();
		});
	}

	template <typename TM0, typename TM1, typename TM2, typename TM3>
	void invoke(my_actor* host, TM0&& p0, TM1&& p1, TM2&& p2, TM3&& p3)
	{
		base_csp_channel::send(host, bind_ref(p0, p1, p2, p3));
	}
};

template <typename T0, typename T1, typename T2>
class csp_invoke_base3<T0, T1, T2, void> : public csp_channel<ref_ex<T0, T1, T2>, void_return>
{
	typedef ref_ex<T0, T1, T2> msg_type;
	typedef csp_channel<msg_type, void_return> base_csp_channel;
protected:
	csp_invoke_base3(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base3() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->void_return
		{
			h(msg._p0, msg._p1, msg._p2);
			return void_return();
		});
	}

	template <typename TM0, typename TM1, typename TM2>
	void invoke(my_actor* host, TM0&& p0, TM1&& p1, TM2&& p2)
	{
		base_csp_channel::send(host, bind_ref(p0, p1, p2));
	}
};

template <typename T0, typename T1>
class csp_invoke_base2<T0, T1, void> : public csp_channel<ref_ex<T0, T1>, void_return>
{
	typedef ref_ex<T0, T1> msg_type;
	typedef csp_channel<msg_type, void_return> base_csp_channel;
protected:
	csp_invoke_base2(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base2() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->void_return
		{
			h(msg._p0, msg._p1);
			return void_return();
		});
	}

	template <typename TM0, typename TM1>
	void invoke(my_actor* host, TM0&& p0, TM1&& p1)
	{
		base_csp_channel::send(host, bind_ref(p0, p1));
	}
};

template <typename T0>
class csp_invoke_base1<T0, void> : public csp_channel<ref_ex<T0>, void_return>
{
	typedef ref_ex<T0> msg_type;
	typedef csp_channel<msg_type, void_return> base_csp_channel;
protected:
	csp_invoke_base1(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base1() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->void_return
		{
			h(msg._p0);
			return void_return();
		});
	}

	template <typename TM0>
	void invoke(my_actor* host, TM0&& p0)
	{
		base_csp_channel::send(host, bind_ref(p0));
	}
};

template <>
class csp_invoke_base0<void> : public csp_channel<ref_ex<>, void_return>
{
	typedef ref_ex<> msg_type;
	typedef csp_channel<msg_type, void_return> base_csp_channel;
protected:
	csp_invoke_base0(shared_strand strand)
		:base_csp_channel(strand) {}
	~csp_invoke_base0() {}
public:
	template <typename H>
	void wait(my_actor* host, const H& h)
	{
		base_csp_channel::take(host, [&](msg_type& msg)->void_return
		{
			h();
			return void_return();
		});
	}

	void invoke(my_actor* host)
	{
		base_csp_channel::send(host, msg_type());
	}
};

template <typename R, typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class csp_invoke;

/*!
@brief CSP模型消息（多读多写，角色可转换，可递归），发送方等消息取出并处理完成后才返回
*/
template <typename R, typename T0, typename T1, typename T2, typename T3>
class csp_invoke
	<R(T0, T1, T2, T3)> : public csp_invoke_base4<T0, T1, T2, T3, R>
{
public:
	struct close_exception : public csp_channel_close_exception {};
public:
	csp_invoke(shared_strand strand)
		:csp_invoke_base4(strand) {}
private:
	void throw_close_exception()
	{
		throw close_exception();
	}
};

template <typename R, typename T0, typename T1, typename T2>
class csp_invoke
	<R(T0, T1, T2)> : public csp_invoke_base3<T0, T1, T2, R>
{
public:
	struct close_exception : public csp_channel_close_exception {};
public:
	csp_invoke(shared_strand strand)
		:csp_invoke_base3(strand) {}
private:
	void throw_close_exception()
	{
		throw close_exception();
	}
};

template <typename R, typename T0, typename T1>
class csp_invoke
	<R(T0, T1)> : public csp_invoke_base2<T0, T1, R>
{
public:
	struct close_exception : public csp_channel_close_exception {};
public:
	csp_invoke(shared_strand strand)
		:csp_invoke_base2(strand) {}
private:
	void throw_close_exception()
	{
		throw close_exception();
	}
};

template <typename R, typename T0>
class csp_invoke
	<R(T0)> : public csp_invoke_base1<T0, R>
{
public:
	struct close_exception : public csp_channel_close_exception {};
public:
	csp_invoke(shared_strand strand)
		:csp_invoke_base1(strand) {}
private:
	void throw_close_exception()
	{
		throw close_exception();
	}
};

template <typename R>
class csp_invoke
	<R()> : public csp_invoke_base0<R>
{
public:
	struct close_exception : public csp_channel_close_exception {};
public:
	csp_invoke(shared_strand strand)
		:csp_invoke_base0(strand) {}
private:
	void throw_close_exception()
	{
		throw close_exception();
	}
};

#endif