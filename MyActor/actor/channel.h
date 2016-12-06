#ifndef __CHANNEL_H
#define __CHANNEL_H

#include "try_move.h"
#include "lambda_ref.h"
#include "generator.h"
#include "my_actor.h"

struct channel_io_exception
{
	channel_io_exception(co_async_state st)
	:state(st) {}
	co_async_state state;
};

template <typename Buffer_>
class ActorMsgBuffer_ : public Buffer_
{
	typedef Buffer_ parent;
public:
	ActorMsgBuffer_(const shared_strand& strand, size_t poolSize)
		:parent(strand, poolSize) {}

	ActorMsgBuffer_(const shared_strand& strand)
		:parent(strand) {}
public:
	template <typename... Args>
	void send(my_actor* host, Args&&... msg)
	{
		co_async_state state = co_async_state::co_async_undefined;
		parent::push(host->make_context(state), std::forward<Args>(msg)...);
		if (co_async_state::co_async_ok != state)
		{
			throw channel_io_exception(state);
		}
	}

	template <typename... Args>
	void take(my_actor* host, Args&... msg)
	{
		co_async_state state = co_async_state::co_async_undefined;
		parent::pop(host->make_same_context(state, msg...));
		if (co_async_state::co_async_ok != state)
		{
			throw channel_io_exception(state);
		}
	}

	template <typename... Args>
	bool try_take(my_actor* host, Args&... msg)
	{
		my_actor::quit_guard qg(host);
		co_async_state state = co_async_state::co_async_undefined;
		parent::try_pop(host->make_same_context(state, msg...));
		if (co_async_state::co_async_ok == state)
		{
			return true;
		}
		else if (co_async_state::co_async_fail == state)
		{
			return false;
		}
		throw channel_io_exception(state);
	}

	template <typename... Args>
	bool timed_take(int ms, my_actor* host, Args&... msg)
	{
		co_async_state state = co_async_state::co_async_undefined;
		overlap_timer::timer_handle timer;
		parent::timed_pop(timer, ms, host->make_same_context(state, msg...));
		if (co_async_state::co_async_ok == state)
		{
			return true;
		}
		else if (co_async_state::co_async_overtime == state)
		{
			return false;
		}
		throw channel_io_exception(state);
	}
};

template <typename Channel_>
class ActorChannel_ : public ActorMsgBuffer_<Channel_>
{
	typedef ActorMsgBuffer_<Channel_> parent;
public:
	ActorChannel_(const shared_strand& strand, size_t buffLength)
		:parent(strand, buffLength) {}

	ActorChannel_(const shared_strand& strand)
		:parent(strand) {}
public:
	template <typename... Args>
	bool try_send(my_actor* host, Args&&... msg)
	{
		co_async_state state = co_async_state::co_async_undefined;
		parent::try_push(host->make_context(state), std::forward<Args>(msg)...);
		if (co_async_state::co_async_ok == state)
		{
			return true;
		}
		else if (co_async_state::co_async_fail == state)
		{
			return false;
		}
		throw channel_io_exception(state);
	}

	template <typename... Args>
	bool timed_send(int ms, my_actor* host, Args&&... msg)
	{
		co_async_state state = co_async_state::co_async_undefined;
		overlap_timer::timer_handle timer;
		parent::timed_push(timer, ms, host->make_context(state), std::forward<Args>(msg)...);
		if (co_async_state::co_async_ok == state)
		{
			return true;
		}
		else if (co_async_state::co_async_overtime == state)
		{
			return false;
		}
		throw channel_io_exception(state);
	}
};

template <typename... Types>
class msg_buffer : public ActorMsgBuffer_<co_msg_buffer<Types...>>
{
public:
	msg_buffer(const shared_strand& strand, size_t poolSize = sizeof(void*))
		:ActorMsgBuffer_<co_msg_buffer<Types...>>(strand, poolSize) {}

	static std::shared_ptr<msg_buffer> make(const shared_strand& strand, size_t poolSize = sizeof(void*))
	{
		return std::shared_ptr<msg_buffer>(new msg_buffer(strand, poolSize));
	}
};

template <typename... Types>
class channel : public ActorChannel_<co_channel<Types...>>
{
public:
	channel(const shared_strand& strand, size_t buffLength = 1)
		:ActorChannel_<co_channel<Types...>>(strand, buffLength) {}

	static std::shared_ptr<channel> make(const shared_strand& strand, size_t buffLength = 1)
	{
		return std::shared_ptr<channel>(new channel(strand, buffLength));
	}
};

template <typename... Types>
class nil_channel : public ActorChannel_<co_nil_channel<Types...>>
{
public:
	nil_channel(const shared_strand& strand)
		:ActorChannel_<co_nil_channel<Types...>>(strand) {}

	static std::shared_ptr<nil_channel> make(const shared_strand& strand)
	{
		return std::shared_ptr<nil_channel>(new nil_channel(strand));
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename... Types>
class csp_channel;

template <typename R, typename... Types>
class csp_channel<R(Types...)> : public co_csp_channel<R(Types...)>
{
	typedef co_csp_channel<R(Types...)> parent;

	struct wait_handler
	{
		wait_handler(trig_once_notifer<>&& ntf, co_async_state& st, csp_result<R>& result, std::tuple<stack_obj<Types>...>& params)
		:_ntf(std::move(ntf)), _state(st), _result(result), _params(params) {}

		template <typename... Args>
		void operator()(co_async_state st, csp_result<R> res, Args&&... msg)
		{
			_state = st;
			_result = res;
			_params = std::tuple<Args&&...>(std::forward<Args>(msg)...);
			_ntf();
		}

		void operator()(co_async_state st)
		{
			_state = st;
			_ntf();
		}

		trig_once_notifer<> _ntf;
		co_async_state& _state;
		csp_result<R>& _result;
		std::tuple<stack_obj<Types>...>& _params;
		NONE_COPY(wait_handler);
		RVALUE_CONSTRUCT4(wait_handler, _ntf, _state, _result, _params);
	};
public:
	csp_channel(const shared_strand& strand)
		:parent(strand) {}

	static std::shared_ptr<csp_channel> make(const shared_strand& strand)
	{
		return std::shared_ptr<csp_channel>(new csp_channel(strand));
	}
public:
	template <typename... Args>
	R send(my_actor* host, Args&&... msg)
	{
		my_actor::quit_guard qg(host);
		stack_obj<R> res;
		co_async_state state = co_async_state::co_async_undefined;
		parent::push(host->make_asio_same_context(state, res), std::forward<Args>(msg)...);
		if (co_async_state::co_async_ok != state)
		{
			throw channel_io_exception(state);
		}
		return std::forward<R>(res.get());
	}

	template <typename TR, typename... Args>
	bool try_send(my_actor* host, TR& res, Args&&... msg)
	{
		my_actor::quit_guard qg(host);
		co_async_state state = co_async_state::co_async_undefined;
		parent::push(host->make_asio_same_context(state, res), std::forward<Args>(msg)...);
		if (co_async_state::co_async_ok == state)
		{
			return true;
		}
		else if (co_async_state::co_async_fail == state)
		{
			return false;
		}
		throw channel_io_exception(state);
	}

	template <typename TR, typename... Args>
	bool timed_send(int ms, my_actor* host, TR& res, Args&&... msg)
	{
		my_actor::quit_guard qg(host);
		co_async_state state = co_async_state::co_async_undefined;
		overlap_timer::timer_handle timer;
		parent::timed_push(timer, ms, host->make_asio_same_context(state, res), std::forward<Args>(msg)...);
		if (co_async_state::co_async_ok == state)
		{
			return true;
		}
		else if (co_async_state::co_async_overtime == state)
		{
			return false;
		}
		throw channel_io_exception(state);
	}

	template <typename Handler>
	void wait(my_actor* host, Handler&& handler)
	{
		my_actor::quit_guard qg(host);
		co_async_state state = co_async_state::co_async_undefined;
		csp_result<R> result;
		std::tuple<stack_obj<Types>...> params;
		host->trig([&](trig_once_notifer<>&& ntf)
		{
			parent::pop(wait_handler(std::move(ntf), state, result, params));
		});
		if (co_async_state::co_async_ok != state)
		{
			throw channel_io_exception(state);
		}
		check_result_void(result, std::forward<Handler>(handler), std::move(params));
	}

	template <typename Handler>
	bool try_wait(my_actor* host, Handler&& handler)
	{
		my_actor::quit_guard qg(host);
		co_async_state state = co_async_state::co_async_undefined;
		csp_result<R> result;
		std::tuple<stack_obj<Types>...> params;
		host->trig([&](trig_once_notifer<>&& ntf)
		{
			parent::try_pop(wait_handler(std::move(ntf), state, result, params));
		});
		if (co_async_state::co_async_ok == state)
		{
			check_result_void(result, std::forward<Handler>(handler), std::move(params));
			return true;
		}
		else if (co_async_state::co_async_fail == state)
		{
			return false;
		}
		throw channel_io_exception(state);
	}

	template <typename Handler>
	bool timed_wait(int ms, my_actor* host, Handler&& handler)
	{
		my_actor::quit_guard qg(host);
		co_async_state state = co_async_state::co_async_undefined;
		overlap_timer::timer_handle timer;
		csp_result<R> result;
		std::tuple<stack_obj<Types>...> params;
		host->trig([&](trig_once_notifer<>&& ntf)
		{
			parent::timed_pop(timer, ms, wait_handler(std::move(ntf), state, result, params));
		});
		if (co_async_state::co_async_ok == state)
		{
			check_result_void(result, std::forward<Handler>(handler), std::move(params));
			return true;
		}
		else if (co_async_state::co_async_overtime == state)
		{
			return false;
		}
		throw channel_io_exception(state);
	}
private:
	template <typename TR, typename Handler, typename Parames>
	void check_result_void(csp_result<TR>& result, Handler&& handler, Parames&& params)
	{
		result.return_(tuple_invoke<R>(handler, std::forward<Parames>(params)));
	}

	template <typename Handler, typename Parames>
	void check_result_void(csp_result<void_type>& result, Handler&& handler, Parames&& params)
	{
		tuple_invoke(handler, std::forward<Parames>(params));
		result.return_();
	}
};

template <typename... Types>
class csp_channel<void(Types...)> : public csp_channel<void_type(Types...)>
{
	typedef csp_channel<void_type(Types...)> parent;
public:
	csp_channel(const shared_strand& strand)
		:parent(strand) {}

	static std::shared_ptr<csp_channel> make(const shared_strand& strand)
	{
		return std::shared_ptr<csp_channel>(new csp_channel(strand));
	}
public:
	template <typename... Args>
	void send(my_actor* host, Args&&... msg)
	{
		my_actor::quit_guard qg(host);
		co_async_state state = co_async_state::co_async_undefined;
		parent::push(host->make_asio_context(state), std::forward<Args>(msg)...);
		if (co_async_state::co_async_ok != state)
		{
			throw channel_io_exception(state);
		}
	}

	template <typename... Args>
	bool try_send(my_actor* host, Args&&... msg)
	{
		void_type1 temp;
		return parent::try_send(host, temp, std::forward<Args>(msg)...);
	}

	template <typename... Args>
	bool timed_send(int ms, my_actor* host, Args&&... msg)
	{
		void_type1 temp;
		return parent::timed_send(ms, host, temp, std::forward<Args>(msg)...);
	}
};

/*!
@brief channel连接器，disconnect时有可能造成消息丢失
*/
template <typename Chan1, typename Chan2>
class chan_connector
{
	struct connector_handler
	{
		template <typename... Args>
		void operator()(co_async_state state, Args&&... msg)
		{
			assert(co_async_state::co_async_ok == state);
			if (!_connector._connSign2._disable)
			{
				_connector._chan2.append_push_notify(wrap_bind_(std::bind([](co_async_state st, chan_connector<Chan1, Chan2>& _connector, RM_CREF(Args)&... msg)
				{
					if (co_async_state::co_async_ok == st)
					{
						_connector._chan2.push(wrap_bind_(std::bind([](co_async_state st, chan_connector<Chan1, Chan2>& _connector)
						{
							assert(co_async_state::co_async_ok == st);
							if (!_connector._connSign1._disable)
							{
								_connector.connector();
							}
						}, __1, std::ref(_connector))), std::move(msg)...);
					}
				}, __1, std::ref(_connector), std::forward<Args>(msg)...)), _connector._connSign2);
			}
		}

		void operator()(co_async_state state)
		{
			assert(co_async_state::co_async_ok != state);
		}

		chan_connector<Chan1, Chan2>& _connector;
	};

	friend connector_handler;
public:
	chan_connector(Chan1& chan1, Chan2& chan2)
		:_chan1(chan1), _chan2(chan2)
	{
		connector();
	}
public:
	template <typename Notify>
	void disconnect(Notify&& ntf)
	{
		typedef RM_CREF(Notify) Notify_;
		_chan1.remove_pop_notify(_chan2.self_strand()->wrap(std::bind([this](co_async_state, Notify_& ntf)
		{
			_connSign1._disable = true;
			_chan2.remove_push_notify(_chan1.self_strand()->wrap(std::bind([this](co_async_state, Notify_& ntf)
			{
				_connSign2._disable = true;
				CHECK_EXCEPTION(ntf);
			}, __1, std::move(ntf))), _connSign2);
		}, __1, std::forward<Notify>(ntf))), _connSign1);
	}
private:
	void connector()
	{
		_chan1.append_pop_notify([&](co_async_state st)
		{
			if (co_async_state::co_async_ok == st)
			{
				_chan1.pop(connector_handler{*this});
			}
		}, _connSign1);
	}
private:
	Chan1& _chan1;
	Chan2& _chan2;
	co_notify_sign _connSign1;
	co_notify_sign _connSign2;
	NONE_COPY(chan_connector);
};

#endif