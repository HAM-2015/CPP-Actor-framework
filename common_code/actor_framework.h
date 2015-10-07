/*!
 @header     actor_framework.h
 @abstract   并发逻辑控制框架(Actor Model)，使用"协程(coroutine)"技术，依赖boost_1.55或更新;
 @discussion 一个Actor对象(actor_handle)依赖一个shared_strand(二级调度器，本身依赖于io_service)，多个Actor可以共同依赖同一个shared_strand;
             支持强制结束、挂起/恢复、延时、多子任务(并发控制);
             在Actor中或所依赖的io_service中进行长时间阻塞的操作或重量级运算，会严重影响依赖同一个io_service的Actor响应速度;
             默认Actor栈空间64k字节，远比线程栈小，注意局部变量占用的空间以及调用层次(注意递归).
 @copyright  Copyright (c) 2015 HAM, E-Mail:591170887@qq.com
 */

#ifndef __ACTOR_FRAMEWORK_H
#define __ACTOR_FRAMEWORK_H

#include <list>
#include <xutility>
#include <functional>
#include "io_engine.h"
#include "shared_strand.h"
#include "msg_queue.h"
#include "actor_mutex.h"
#include "scattered.h"
#include "stack_object.h"
#include "check_actor_stack.h"
#include "actor_timer.h"
#include "tuple_option.h"
#include "trace.h"

class my_actor;
typedef std::shared_ptr<my_actor> actor_handle;//Actor句柄

class mutex_trig_notifer;
class mutex_trig_handle;
class ActorMutex_;

using namespace std;

//此函数会进入Actor中断标记，使用时注意逻辑的“连续性”可能会被打破
#define __yield_interrupt

/*!
@brief 用于检测在Actor内调用的函数是否触发了强制退出
*/
#ifdef _DEBUG
#define BEGIN_CHECK_FORCE_QUIT try {
#define END_CHECK_FORCE_QUIT } catch (my_actor::force_quit_exception&) {assert(false);}
#else
#define BEGIN_CHECK_FORCE_QUIT
#define END_CHECK_FORCE_QUIT
#endif

//检测 pump_msg 是否有 pump_disconnected_exception 异常抛出，因为在 catch 内部不能安全的进行coro切换
#define CATCH_PUMP_DISCONNECTED CATCH_FOR(my_actor::pump_disconnected_exception)


//////////////////////////////////////////////////////////////////////////

template <size_t N>
struct TupleTec_
{
	template <typename DTuple, typename SRC, typename... Args>
	static inline void receive(DTuple& dst, SRC&& src, Args&&... args)
	{
		std::get<std::tuple_size<DTuple>::value - N>(dst) = TRY_MOVE(src);
		TupleTec_<N - 1>::receive(dst, TRY_MOVE(args)...);
	}

	template <typename DTuple, typename STuple>
	static inline void receive_ref(DTuple& dst, STuple&& src)
	{
		std::get<N - 1>(dst) = tuple_move<N - 1, STuple&&>::get(TRY_MOVE(src));
		TupleTec_<N - 1>::receive_ref(dst, TRY_MOVE(src));
	}
};

template <>
struct TupleTec_<0>
{
	template <typename DTuple>
	static inline void receive(DTuple&) {}

	template <typename DTuple, typename STuple>
	static inline void receive_ref(DTuple&, STuple&&) {}
};

template <typename DTuple, typename... Args>
void TupleReceiver_(DTuple& dst, Args&&... args)
{
	static_assert(std::tuple_size<DTuple>::value == sizeof...(Args), "");
	TupleTec_<sizeof...(Args)>::receive(dst, TRY_MOVE(args)...);
}

template <typename DTuple, typename STuple>
void TupleReceiverRef_(DTuple& dst, STuple&& src)
{
	static_assert(std::tuple_size<DTuple>::value == std::tuple_size<STuple>::value, "");
	TupleTec_<std::tuple_size<DTuple>::value>::receive_ref(dst, TRY_MOVE(src));
}
//////////////////////////////////////////////////////////////////////////

template <typename... TYPES>
struct DstReceiverRef_ {};

template <typename... ArgsPipe>
struct DstReceiverBase_
{
	virtual void move_from(std::tuple<ArgsPipe...>& s) = 0;
	virtual void clear() = 0;
	virtual bool has() = 0;
};

template <typename... ARGS>
struct DstReceiverBuff_ : public DstReceiverBase_<TYPE_PIPE(ARGS)...>
{
	void move_from(std::tuple<TYPE_PIPE(ARGS)...>& s)
	{
		_dstBuff.create(std::move(s));
	}

	void clear()
	{
		_dstBuff.destroy();
	}

	bool has()
	{
		return _dstBuff.has();
	}

	stack_obj<std::tuple<TYPE_PIPE(ARGS)...>> _dstBuff;
};

template <typename... ARGS, typename... OUTS>
struct DstReceiverRef_<types_pck<ARGS...>, types_pck<OUTS...>> : public DstReceiverBase_<TYPE_PIPE(ARGS)...>
{
	template <typename... Outs>
	DstReceiverRef_(Outs&... outs)
		:_dstRef(outs...) {}

	void move_from(std::tuple<TYPE_PIPE(ARGS)...>& s)
	{
		_has = true;
		TupleReceiverRef_(_dstRef, std::move(s));
	}

	void clear()
	{
		_has = false;
	}

	bool has()
	{
		return _has;
	}

	std::tuple<OUTS&...> _dstRef;
	bool _has = false;
};

//////////////////////////////////////////////////////////////////////////

template <typename... ARGS>
class mutex_block_msg;

template <typename... ARGS>
class mutex_block_trig;

template <typename... ARGS>
class mutex_block_pump;

template <typename... ARGS>
class actor_msg_handle;

template <typename... ARGS>
class actor_trig_handle;

template <typename... ARGS>
class MsgNotiferBase_;

class actor_msg_handle_base
{
protected:
	actor_msg_handle_base();
	virtual ~actor_msg_handle_base(){}
public:
	virtual void close() = 0;
	virtual size_t size() = 0;
protected:
	void pull_yield();
	bool is_quited();
	void set_actor(const actor_handle& hostActor);
	static std::shared_ptr<bool> new_bool();
protected:
	my_actor* _hostActor;
	std::shared_ptr<bool> _closed;
	DEBUG_OPERATION(shared_strand _strand);
	bool _waiting;
};

template <typename... ARGS>
class ActorMsgHandlePush_: public actor_msg_handle_base
{
	friend MsgNotiferBase_<ARGS...>;
protected:
	virtual void push_msg(std::tuple<TYPE_PIPE(ARGS)...>&) = 0;
};

template <>
class ActorMsgHandlePush_<> : public actor_msg_handle_base
{
	friend MsgNotiferBase_<>;
protected:
	virtual void push_msg() = 0;
};

template <typename... ARGS>
class MsgNotiferBase_
{
	typedef ActorMsgHandlePush_<ARGS...> msg_handle;
protected:
	MsgNotiferBase_()
		:_msgHandle(NULL){}

	MsgNotiferBase_(msg_handle* msgHandle)
		:_msgHandle(msgHandle),
		_hostActor(_msgHandle->_hostActor->shared_from_this()),
		_closed(msgHandle->_closed)
	{
		assert(msgHandle->_strand == _hostActor->self_strand());
	}

	template <typename... ArgsPipe>
	struct msg_capture
	{
		template <typename... Args>
		msg_capture(msg_handle* msgHandle, const actor_handle& host, const std::shared_ptr<bool>& closed, Args&&... args)
			:_msgHandle(msgHandle), _hostActor(host), _closed(closed), _args(TRY_MOVE(args)...) {}

		msg_capture(const msg_capture& s)
			:_msgHandle(s._msgHandle), _hostActor(s._hostActor), _closed(s._closed), _args(std::move(s._args)) {}

		msg_capture(msg_capture&& s)
			:_msgHandle(s._msgHandle), _hostActor(s._hostActor), _closed(s._closed), _args(std::move(s._args)) {}

		void operator =(const msg_capture& s)
		{
			static_assert(false, "no copy");
			_msgHandle = s._msgHandle;
			_hostActor = s._hostActor;
			_closed = s._closed;
			_args = std::move(s._args);
		}

		void operator =(msg_capture&& s)
		{
			_msgHandle = s._msgHandle;
			_hostActor = s._hostActor;
			_closed = s._closed;
			_args = std::move(s._args);
		}

		void operator ()()
		{
			if (!(*_closed))
			{
				_msgHandle->push_msg(_args);
			}
		}

		mutable std::tuple<ArgsPipe...> _args;
		msg_handle* _msgHandle;
		actor_handle _hostActor;
		std::shared_ptr<bool> _closed;
	};

	template <>
	struct msg_capture<>
	{
		msg_capture(msg_handle* msgHandle, const actor_handle& host, const std::shared_ptr<bool>& closed)
		:_msgHandle(msgHandle), _hostActor(host), _closed(closed) {}

		void operator ()()
		{
			if (!(*_closed))
			{
				_msgHandle->push_msg();
			}
		}

		msg_handle* _msgHandle;
		actor_handle _hostActor;
		std::shared_ptr<bool> _closed;
	};
public:
	template <typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		_hostActor->self_strand()->try_tick(msg_capture<TYPE_PIPE(ARGS)...>(_msgHandle, _hostActor, _closed, TRY_MOVE(args)...));
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		_hostActor->self_strand()->try_tick(msg_capture<>(_msgHandle, _hostActor, _closed));
	}

	std::function<void(ARGS...)> case_func() const
	{
		return std::function<void(ARGS...)>(*this);
	}

	actor_handle host_actor() const
	{
		return _hostActor;
	}

	bool empty() const
	{
		return !_msgHandle;
	}

	void clear()
	{
		_msgHandle = NULL;
		_hostActor.reset();
		_closed.reset();
	}

	operator bool() const
	{
		return !empty();
	}
private:
	msg_handle* _msgHandle;
	actor_handle _hostActor;
	std::shared_ptr<bool> _closed;
};

template <typename... ARGS>
class actor_msg_notifer : public MsgNotiferBase_<ARGS...>
{
	friend actor_msg_handle<ARGS...>;
public:
	actor_msg_notifer()	{}
private:
	actor_msg_notifer(ActorMsgHandlePush_<ARGS...>* msgHandle)
		:MsgNotiferBase_(msgHandle) {}
};

template <typename... ARGS>
class actor_trig_notifer : public MsgNotiferBase_<ARGS...>
{
	friend actor_trig_handle<ARGS...>;
public:
	actor_trig_notifer() {}
private:
	actor_trig_notifer(ActorMsgHandlePush_<ARGS...>* msgHandle)
		:MsgNotiferBase_(msgHandle) {}
};

template <typename... ARGS>
class actor_msg_handle : public ActorMsgHandlePush_<ARGS...>
{
	typedef std::tuple<TYPE_PIPE(ARGS)...> msg_type;
	typedef DstReceiverBase_<TYPE_PIPE(ARGS)...> dst_receiver;
	typedef actor_msg_notifer<ARGS...> msg_notifer;

	friend mutex_block_msg<ARGS...>;
	friend my_actor;
public:
	actor_msg_handle(size_t fixedSize = 16)
		:_msgBuff(fixedSize), _dstRec(NULL) {}

	~actor_msg_handle()
	{
		close();
	}
private:
	msg_notifer make_notifer(const actor_handle& hostActor)
	{
		close();
		set_actor(hostActor);
		_closed = new_bool();
		_waiting = false;
		return msg_notifer(this);
	}

	void push_msg(msg_type& msg)
	{
		assert(_strand->running_in_this_thread());
		if (!is_quited())
		{
			if (_waiting)
			{
				_waiting = false;
				assert(_msgBuff.empty());
				assert(_dstRec);
				_dstRec->move_from(msg);
				_dstRec = NULL;
				pull_yield();
				return;
			}
			_msgBuff.push_back(std::move(msg));
		}
	}

	bool read_msg(dst_receiver& dst)
	{
		assert(_strand->running_in_this_thread());
		assert(_closed);
		assert(!*_closed);
		if (!_msgBuff.empty())
		{
			dst.move_from(_msgBuff.front());
			_msgBuff.pop_front();
			return true;
		}
		_dstRec = &dst;
		_waiting = true;
		return false;
	}

	void stop_waiting()
	{
		_waiting = false;
		_dstRec = NULL;
	}

	void close()
	{
		if (_closed)
		{
			assert(_strand->running_in_this_thread());
			*_closed = true;
			_closed.reset();
		}
		_dstRec = NULL;
		_waiting = false;
		_msgBuff.clear();
		_hostActor = NULL;
	}

	size_t size()
	{
		assert(_strand->running_in_this_thread());
		return _msgBuff.size();
	}
private:
	dst_receiver* _dstRec;
	msg_queue<msg_type> _msgBuff;
};

template <>
class actor_msg_handle<> : public ActorMsgHandlePush_<>
{
	typedef actor_msg_notifer<> msg_notifer;

	friend mutex_block_msg<>;
	friend my_actor;
public:
	~actor_msg_handle()
	{
		close();
	}
private:
	msg_notifer make_notifer(const actor_handle& hostActor)
	{
		close();
		set_actor(hostActor);
		_closed = new_bool();
		_waiting = false;
		_dstRec = NULL;
		return msg_notifer(this);
	}

	void push_msg()
	{
		assert(_strand->running_in_this_thread());
		if (!is_quited())
		{
			if (_waiting)
			{
				_waiting = false;
				if (_dstRec)
				{
					*_dstRec = true;
					_dstRec = NULL;
				}
				pull_yield();
				return;
			}
			_msgCount++;
		}
	}

	bool read_msg()
	{
		assert(_strand->running_in_this_thread());
		assert(_closed);
		assert(!*_closed);
		assert(!_dstRec);
		if (_msgCount)
		{
			_msgCount--;
			return true;
		}
		_waiting = true;
		return false;
	}

	bool read_msg(bool& dst)
	{
		assert(_strand->running_in_this_thread());
		assert(_closed);
		assert(!*_closed);
		if (_msgCount)
		{
			_msgCount--;
			dst = true;
			return true;
		}
		_dstRec = &dst;
		_waiting = true;
		return false;
	}

	void stop_waiting()
	{
		_waiting = false;
		_dstRec = NULL;
	}

	void close()
	{
		if (_closed)
		{
			assert(_strand->running_in_this_thread());
			*_closed = true;
			_closed.reset();
		}
		_dstRec = NULL;
		_msgCount = 0;
		_waiting = false;
		_hostActor = NULL;
	}

	size_t size()
	{
		assert(_strand->running_in_this_thread());
		return _msgCount;
	}
private:
	bool* _dstRec;
	size_t _msgCount;
};
//////////////////////////////////////////////////////////////////////////

template <typename... ARGS>
class actor_trig_handle : public ActorMsgHandlePush_<ARGS...>
{
	typedef std::tuple<TYPE_PIPE(ARGS)...> msg_type;
	typedef DstReceiverBase_<TYPE_PIPE(ARGS)...> dst_receiver;
	typedef actor_trig_notifer<ARGS...> msg_notifer;

	friend mutex_block_trig<ARGS...>;
	friend my_actor;
public:
	actor_trig_handle()
		:_hasMsg(false), _dstRec(NULL) {}

	~actor_trig_handle()
	{
		close();
	}
private:
	msg_notifer make_notifer(const actor_handle& hostActor)
	{
		close();
		set_actor(hostActor);
		_closed = new_bool();
		_waiting = false;
		_hasMsg = false;
		return msg_notifer(this);
	}

	void push_msg(msg_type& msg)
	{
		assert(_strand->running_in_this_thread());
		*_closed = true;
		if (!is_quited())
		{
			if (_waiting)
			{
				_waiting = false;
				assert(_dstRec);
				_dstRec->move_from(msg);
				_dstRec = NULL;
				pull_yield();
				return;
			}
			_hasMsg = true;
			new(_msgBuff)msg_type(std::move(msg));
		}
	}

	bool read_msg(dst_receiver& dst)
	{
		assert(_strand->running_in_this_thread());
		assert(_closed);
		if (_hasMsg)
		{
			_hasMsg = false;
			dst.move_from(*(msg_type*)_msgBuff);
			((msg_type*)_msgBuff)->~msg_type();
			return true;
		}
		assert(!*_closed);
		_dstRec = &dst;
		_waiting = true;
		return false;
	}

	void stop_waiting()
	{
		_waiting = false;
		_dstRec = NULL;
	}

	void close()
	{
		if (_closed)
		{
			assert(_strand->running_in_this_thread());
			*_closed = true;
			_closed.reset();
		}
		if (_hasMsg)
		{
			_hasMsg = false;
			((msg_type*)_msgBuff)->~msg_type();
		}
		_dstRec = NULL;
		_waiting = false;
		_hostActor = NULL;
	}
public:
	bool has()
	{
		assert(_strand->running_in_this_thread());
		return _hasMsg;
	}

	size_t size()
	{
		assert(_strand->running_in_this_thread());
		return _hasMsg ? 1 : 0;
	}
private:
	dst_receiver* _dstRec;
	bool _hasMsg;
	unsigned char _msgBuff[sizeof(msg_type)];
};

template <>
class actor_trig_handle<> : public ActorMsgHandlePush_<>
{
	typedef actor_trig_notifer<> msg_notifer;

	friend mutex_block_trig<>;
	friend my_actor;
public:
	actor_trig_handle()
		:_hasMsg(false){}

	~actor_trig_handle()
	{
		close();
	}
private:
	msg_notifer make_notifer(const actor_handle& hostActor)
	{
		close();
		set_actor(hostActor);
		_closed = new_bool();
		_waiting = false;
		_hasMsg = false;
		_dstRec = NULL;
		return msg_notifer(this);
	}

	void push_msg()
	{
		assert(_strand->running_in_this_thread());
		*_closed = true;
		if (!is_quited())
		{
			if (_waiting)
			{
				_waiting = false;
				if (_dstRec)
				{
					*_dstRec = true;
					_dstRec = NULL;
				}
				pull_yield();
				return;
			}
			_hasMsg = true;
		}
	}

	bool read_msg()
	{
		assert(_strand->running_in_this_thread());
		assert(_closed);
		assert(!_dstRec);
		if (_hasMsg)
		{
			_hasMsg = false;
			return true;
		}
		assert(!*_closed);
		_waiting = true;
		return false;
	}

	bool read_msg(bool& dst)
	{
		assert(_strand->running_in_this_thread());
		assert(_closed);
		if (_hasMsg)
		{
			_hasMsg = false;
			dst = true;
			return true;
		}
		assert(!*_closed);
		_dstRec = &dst;
		_waiting = true;
		return false;
	}

	void stop_waiting()
	{
		_waiting = false;
		_dstRec = NULL;
	}

	void close()
	{
		if (_closed)
		{
			assert(_strand->running_in_this_thread());
			*_closed = true;
			_closed.reset();
		}
		_dstRec = NULL;
		_hasMsg = false;
		_waiting = false;
		_hostActor = NULL;
	}
public:
	bool has()
	{
		assert(_strand->running_in_this_thread());
		return _hasMsg;
	}

	size_t size()
	{
		assert(_strand->running_in_this_thread());
		return _hasMsg ? 1 : 0;
	}
private:
	bool* _dstRec;
	bool _hasMsg;
};

//////////////////////////////////////////////////////////////////////////
template <typename... ARGS>
class MsgPump_;

template <typename... ARGS>
class MsgPool_;

template <typename... ARGS>
class post_actor_msg;

class MsgPumpBase_
{
	friend my_actor;
public:
	virtual ~MsgPumpBase_() {}
protected:
	bool is_quited();
	void pull_yield();
	void push_yield();
protected:
	my_actor* _hostActor;
};

template <typename... ARGS>
class MsgPump_ : public MsgPumpBase_
{
	typedef std::tuple<TYPE_PIPE(ARGS)...> msg_type;
	typedef DstReceiverBase_<TYPE_PIPE(ARGS)...> dst_receiver;
	typedef MsgPool_<ARGS...> msg_pool_type;
	typedef typename msg_pool_type::pump_handler pump_handler;

	struct msg_capture
	{
		msg_capture(const actor_handle& hostActor, const shared_ptr<MsgPump_>& sharedThis, msg_type&& msg)
		:_hostActor(hostActor), _sharedThis(sharedThis), _msg(std::move(msg)) {}

		msg_capture(const msg_capture& s)
			:_hostActor(s._hostActor), _sharedThis(s._sharedThis), _msg(std::move(s._msg)) {}

		msg_capture(msg_capture&& s)
			:_hostActor(s._hostActor), _sharedThis(s._sharedThis), _msg(std::move(s._msg)) {}

		void operator =(const msg_capture& s)
		{
			static_assert(false, "no copy");
			_hostActor = s._hostActor;
			_sharedThis = s._sharedThis;
			_msg = std::move(s._msg);
		}

		void operator =(msg_capture&& s)
		{
			_hostActor = s._hostActor;
			_sharedThis = s._sharedThis;
			_msg = std::move(s._msg);
		}

		void operator()()
		{
			_sharedThis->receiver(std::move(_msg));
		}

		mutable msg_type _msg;
		actor_handle _hostActor;
		shared_ptr<MsgPump_> _sharedThis;
	};

	friend my_actor;
	friend MsgPool_<ARGS...>;
	friend mutex_block_pump<ARGS...>;
	friend pump_handler;
private:
	MsgPump_(){}
	~MsgPump_(){}
private:
	static std::shared_ptr<MsgPump_> make(const actor_handle& hostActor)
	{
		std::shared_ptr<MsgPump_> res(new MsgPump_(), [](MsgPump_* p){delete p; });
		res->_weakThis = res;
		res->_hasMsg = false;
		res->_waiting = false;
		res->_checkDis = false;
		res->_pumpCount = 0;
		res->_dstRec = NULL;
		res->_hostActor = hostActor.get();
		res->_strand = hostActor->self_strand();
		return res;
	}

	void receiver(msg_type&& msg)
	{
		if (!is_quited())
		{
			assert(!_hasMsg);
			_pumpCount++;
			if (_dstRec)
			{
				_dstRec->move_from(msg);
				_dstRec = NULL;
				if (_waiting)
				{
					_waiting = false;
					_checkDis = false;
					pull_yield();
				}
				//read_msg时
			}
			else
			{//pump_msg超时结束后才接受到消息
				assert(!_waiting);
				_hasMsg = true;
				new (_msgSpace)msg_type(std::move(msg));
			}
		}
	}

	void receive_msg_tick(msg_type&& msg, const actor_handle& hostActor)
	{
		_strand->try_tick(msg_capture(hostActor, _weakThis.lock(), std::move(msg)));
	}

	void receive_msg(msg_type&& msg, const actor_handle& hostActor)
	{
		if (_strand->running_in_this_thread())
		{
			receiver(std::move(msg));
		} 
		else
		{
			_strand->post(msg_capture(hostActor, _weakThis.lock(), std::move(msg)));
		}
	}

	bool read_msg(dst_receiver& dst)
	{
		assert(_strand->running_in_this_thread());
		assert(!_dstRec);
		assert(!_waiting);
		if (_hasMsg)
		{
			_hasMsg = false;
			dst.move_from(*(msg_type*)_msgSpace);
			((msg_type*)_msgSpace)->~msg_type();
			return true;
		}
		_dstRec = &dst;
		if (!_pumpHandler.empty())
		{
			_pumpHandler(_pumpCount);
			_waiting = !!_dstRec;
			return !_dstRec;
		}
		_waiting = true;
		return false;
	}

	bool try_read(dst_receiver& dst)
	{
		assert(_strand->running_in_this_thread());
		assert(!_dstRec);
		assert(!_waiting);
		if (_hasMsg)
		{
			_hasMsg = false;
			dst.move_from(*(msg_type*)_msgSpace);
			((msg_type*)_msgSpace)->~msg_type();
			return true;
		}
		if (!_pumpHandler.empty())
		{
			bool wait = false;
			if (_pumpHandler.try_pump(_hostActor, dst, _pumpCount, wait))
			{
				if (wait)
				{
					if (!_hasMsg)
					{
						_dstRec = &dst;
						push_yield();
						assert(_hasMsg);
					}
					_hasMsg = false;
					dst.move_from(*(msg_type*)_msgSpace);
					((msg_type*)_msgSpace)->~msg_type();
				}
				return true;
			}
		}
		return false;
	}

	size_t size()
	{
		assert(_strand->running_in_this_thread());
		if (!_pumpHandler.empty())
		{
			return (_hasMsg ? 1 : 0) + _pumpHandler.size(_hostActor, _pumpCount);
		}
		return 0;
	}

	size_t snap_size()
	{
		assert(_strand->running_in_this_thread());
		if (!_pumpHandler.empty())
		{
			return (_hasMsg ? 1 : 0) + _pumpHandler.snap_size(_pumpCount);
		}
		return 0;
	}

	void stop_waiting()
	{
		_waiting = false;
		_dstRec = NULL;
	}

	void connect(const pump_handler& pumpHandler)
	{
		assert(_strand->running_in_this_thread());
		assert(_hostActor);
		_pumpHandler = pumpHandler;
		_pumpCount = 0;
		if (_waiting)
		{
			_pumpHandler.post_pump(_pumpCount);
		}
	}

	void clear()
	{
		assert(_strand->running_in_this_thread());
		assert(_hostActor);
		_pumpHandler.clear();
		if (!is_quited() && _checkDis)
		{
			assert(_waiting);
			_waiting = false;
			_dstRec = NULL;
			pull_yield();
		}
	}

	void close()
	{
		if (_hasMsg)
		{
			((msg_type*)_msgSpace)->~msg_type();
		}
		_hasMsg = false;
		_dstRec = NULL;
		_pumpCount = 0;
		_waiting = false;
		_checkDis = false;
		_pumpHandler.clear();
		_hostActor = NULL;
	}

	bool isDisconnected()
	{
		return _pumpHandler.empty();
	}
private:
	std::weak_ptr<MsgPump_> _weakThis;
	unsigned char _msgSpace[sizeof(msg_type)];
	pump_handler _pumpHandler;
	shared_strand _strand;
	dst_receiver* _dstRec;
	unsigned char _pumpCount;
	bool _hasMsg : 1;
	bool _waiting : 1;
	bool _checkDis : 1;
};

template <typename... ARGS>
class MsgPool_
{
	typedef std::tuple<TYPE_PIPE(ARGS)...> msg_type;
	typedef DstReceiverBase_<TYPE_PIPE(ARGS)...> dst_receiver;
	typedef MsgPump_<ARGS...> msg_pump_type;
	typedef post_actor_msg<ARGS...> post_type;

	struct pump_handler
	{
		void pump_msg(unsigned char pumpID, const actor_handle& hostActor)
		{
			assert(_msgPump == _thisPool->_msgPump);
			auto& msgBuff = _thisPool->_msgBuff;
			if (!_thisPool->_waiting)//上次取消息超时后取消了等待，此时取还没消息
			{
				if (pumpID == _thisPool->_sendCount)
				{
					if (!msgBuff.empty())
					{
						msg_type mt_ = std::move(msgBuff.front());
						msgBuff.pop_front();
						_thisPool->_sendCount++;
						_msgPump->receive_msg(std::move(mt_), hostActor);
					}
					else
					{
						_thisPool->_waiting = true;
					}
				}
				else
				{//上次消息没取到，重新取，但实际中间已经post出去了
					assert(!_thisPool->_waiting);
					assert(pumpID + 1 == _thisPool->_sendCount);
				}
			}
			else
			{
				assert(msgBuff.empty());
				assert(pumpID == _thisPool->_sendCount);
			}
		}

		void operator()(unsigned char pumpID)
		{
			assert(_thisPool);
			if (_thisPool->_strand->running_in_this_thread())
			{
				if (_msgPump == _thisPool->_msgPump)
				{
					pump_msg(pumpID, _msgPump->_hostActor->shared_from_this());
				}
			}
			else
			{
				post_pump(pumpID);
			}
		}

		bool try_pump(my_actor* host, dst_receiver&dst, unsigned char pumpID, bool& wait)
		{
			assert(_thisPool);
			auto& refThis_ = *this;
			my_actor::quit_guard qg(host);
			return host->send<bool>(_thisPool->_strand, [&, refThis_]()->bool
			{
				bool ok = false;
				if (_msgPump == _thisPool->_msgPump)
				{
					auto& msgBuff = _thisPool->_msgBuff;
					if (pumpID == _thisPool->_sendCount)
					{
						if (!msgBuff.empty())
						{
							dst.move_from(msgBuff.front());
							msgBuff.pop_front();
							ok = true;
						}
					}
					else
					{//上次消息没取到，重新取，但实际中间已经post出去了
						assert(!_thisPool->_waiting);
						assert(pumpID + 1 == _thisPool->_sendCount);
						wait = true;
						ok = true;
					}
				}
				return ok;
			});
		}

		size_t size(my_actor* host, unsigned char pumpID)
		{
			assert(_thisPool);
			auto& refThis_ = *this;
			my_actor::quit_guard qg(host);
			return host->send<size_t>(_thisPool->_strand, [&, refThis_]()->size_t
			{
				if (_msgPump == _thisPool->_msgPump)
				{
					auto& msgBuff = _thisPool->_msgBuff;
					if (pumpID == _thisPool->_sendCount)
					{
						return msgBuff.size();
					}
					else
					{
						return msgBuff.size() + 1;
					}
				}
				return 0;
			});
		}

		size_t snap_size(unsigned char pumpID)
		{
			assert(_thisPool);
			if (_thisPool->_strand->running_in_this_thread())
			{
				if (_msgPump == _thisPool->_msgPump)
				{
					auto& msgBuff = _thisPool->_msgBuff;
					if (pumpID == _thisPool->_sendCount)
					{
						return msgBuff.size();
					}
					else
					{
						return msgBuff.size() + 1;
					}
				}
				return 0;
			} 
			return _thisPool->_msgBuff.size();
		}

		void post_pump(unsigned char pumpID)
		{
			assert(!empty());
			auto& refThis_ = *this;
			auto hostActor = _msgPump->_hostActor->shared_from_this();
			_thisPool->_strand->post([=]
			{
				if (refThis_._msgPump == refThis_._thisPool->_msgPump)
				{
					((pump_handler&)refThis_).pump_msg(pumpID, hostActor);
				}
			});
		}

		bool empty()
		{
			return !_thisPool;
		}

		void clear()
		{
			_thisPool.reset();
			_msgPump.reset();
		}

		std::shared_ptr<MsgPool_> _thisPool;
		std::shared_ptr<msg_pump_type> _msgPump;
	};

	struct msg_capture
	{
		msg_capture(const actor_handle& hostActor, const shared_ptr<MsgPool_>& sharedThis, msg_type&& msg)
		:_hostActor(hostActor), _sharedThis(sharedThis), _msg(std::move(msg)) {}

		msg_capture(const msg_capture& s)
			:_hostActor(s._hostActor), _sharedThis(s._sharedThis), _msg(std::move(s._msg)) {}

		msg_capture(msg_capture&& s)
			:_hostActor(s._hostActor), _sharedThis(s._sharedThis), _msg(std::move(s._msg)) {}

		void operator =(const msg_capture& s)
		{
			static_assert(false, "no copy");
			_hostActor = s._hostActor;
			_sharedThis = s._sharedThis;
			_msg = std::move(s._msg);
		}

		void operator =(msg_capture&& s)
		{
			_hostActor = s._hostActor;
			_sharedThis = s._sharedThis;
			_msg = std::move(s._msg);
		}

		void operator()()
		{
			_sharedThis->send_msg(std::move(_msg), _hostActor);
		}

		mutable msg_type _msg;
		actor_handle _hostActor;
		shared_ptr<MsgPool_> _sharedThis;
	};

	friend my_actor;
	friend msg_pump_type;
	friend post_type;
private:
	MsgPool_(size_t fixedSize)
		:_msgBuff(fixedSize)
	{

	}
	~MsgPool_()
	{

	}
private:
	static std::shared_ptr<MsgPool_> make(const shared_strand& strand, size_t fixedSize)
	{
		std::shared_ptr<MsgPool_> res(new MsgPool_(fixedSize), [](MsgPool_* p){delete p; });
		res->_weakThis = res;
		res->_strand = strand;
		res->_waiting = false;
		res->_closed = false;
		res->_sendCount = 0;
		return res;
	}

	void send_msg(msg_type&& mt, const actor_handle& hostActor)
	{
		if (_closed) return;

		if (_waiting)
		{
			_waiting = false;
			assert(_msgPump);
			assert(_msgBuff.empty());
			_sendCount++;
			_msgPump->receive_msg(std::move(mt), hostActor);
// 			if (_msgBuff.empty())
// 			{
// 				_msgPump->receive_msg(std::move(mt), hostActor);
// 			}
// 			else
// 			{
// 				msg_type mt_ = std::move(_msgBuff.front());
// 				_msgBuff.pop_front();
// 				_msgBuff.push_back(std::move(mt));
// 				_msgPump->receive_msg(std::move(mt_), hostActor);
// 			}
		}
		else
		{
			_msgBuff.push_back(std::move(mt));
		}
	}

	void post_msg(msg_type&& mt, const actor_handle& hostActor)
	{
		if (_waiting)
		{
			_waiting = false;
			assert(_msgPump);
			assert(_msgBuff.empty());
			_sendCount++;
			_msgPump->receive_msg_tick(std::move(mt), hostActor);
// 			if (_msgBuff.empty())
// 			{
// 				_msgPump->receive_msg_tick(std::move(mt), hostActor);
// 			}
// 			else
// 			{
// 				_msgPump->receive_msg_tick(std::move(_msgBuff.front()), hostActor);
// 				_msgBuff.pop_front();
// 				_msgBuff.push_back(std::move(mt));
// 			}
		}
		else
		{
			_msgBuff.push_back(std::move(mt));
		}
	}

	void push_msg(msg_type&& mt, const actor_handle& hostActor)
	{
		if (_closed) return;

		if (_strand->running_in_this_thread())
		{
			post_msg(std::move(mt), hostActor);
		}
		else
		{
			_strand->post(msg_capture(hostActor, _weakThis.lock(), std::move(mt)));
		}
	}

	pump_handler connect_pump(const std::shared_ptr<msg_pump_type>& msgPump)
	{
		assert(msgPump);
		assert(_strand->running_in_this_thread());
		_msgPump = msgPump;
		pump_handler compHandler;
		compHandler._thisPool = _weakThis.lock();
		compHandler._msgPump = msgPump;
		_sendCount = 0;
		_waiting = false;
		return compHandler;
	}

	void disconnect()
	{
		assert(_strand->running_in_this_thread());
		_msgPump.reset();
		_waiting = false;
	}

	void expand_fixed(size_t fixedSize)
	{
		assert(_strand->running_in_this_thread());
		_msgBuff.expand_fixed(fixedSize);
	}
private:
	std::weak_ptr<MsgPool_> _weakThis;
	std::shared_ptr<msg_pump_type> _msgPump;
	msg_queue<msg_type> _msgBuff;
	shared_strand _strand;
	unsigned char _sendCount;
	bool _waiting : 1;
	bool _closed : 1;
};

class MsgPumpVoid_;

class MsgPoolVoid_
{
	typedef post_actor_msg<> post_type;
	typedef MsgPumpVoid_ msg_pump_type;

	struct pump_handler
	{
		void operator()(unsigned char pumpID);
		void pump_msg(unsigned char pumpID, const actor_handle& hostActor);
		bool try_pump(my_actor* host, unsigned char pumpID, bool& wait);
		size_t size(my_actor* host, unsigned char pumpID);
		size_t snap_size(unsigned char pumpID);
		void post_pump(unsigned char pumpID);
		bool empty();
		bool same_strand();
		void clear();

		std::shared_ptr<MsgPoolVoid_> _thisPool;
		std::shared_ptr<msg_pump_type> _msgPump;
	};

	friend my_actor;
	friend MsgPumpVoid_;
	friend post_type;
protected:
	MsgPoolVoid_(const shared_strand& strand);
	virtual ~MsgPoolVoid_();
protected:
	void send_msg(const actor_handle& hostActor);
	void post_msg(const actor_handle& hostActor);
	void push_msg(const actor_handle& hostActor);
	pump_handler connect_pump(const std::shared_ptr<msg_pump_type>& msgPump);
	void disconnect();
	void expand_fixed(size_t fixedSize){};
protected:
	std::weak_ptr<MsgPoolVoid_> _weakThis;
	std::shared_ptr<msg_pump_type> _msgPump;
	size_t _msgBuff;
	shared_strand _strand;
	unsigned char _sendCount;
	bool _waiting : 1;
	bool _closed : 1;
};

class MsgPumpVoid_ : public MsgPumpBase_
{
	typedef MsgPoolVoid_ msg_pool_type;
	typedef MsgPoolVoid_::pump_handler pump_handler;

	friend my_actor;
	friend MsgPoolVoid_;
	friend mutex_block_pump<>;
	friend pump_handler;
protected:
	MsgPumpVoid_(const actor_handle& hostActor);
	virtual ~MsgPumpVoid_();
protected:
	void receiver();
	void receive_msg_tick(const actor_handle& hostActor);
	void receive_msg(const actor_handle& hostActor);
	bool read_msg();
	bool read_msg(bool& dst);
	bool try_read();
	size_t size();
	size_t snap_size();
	void stop_waiting();
	void connect(const pump_handler& pumpHandler);
	void clear();
	void close();
	bool isDisconnected();
protected:
	std::weak_ptr<MsgPumpVoid_> _weakThis;
	pump_handler _pumpHandler;
	shared_strand _strand;
	unsigned char _pumpCount;
	bool* _dstRec;
	bool _waiting : 1;
	bool _hasMsg : 1;
	bool _checkDis : 1;
};

template <>
class MsgPool_<> : public MsgPoolVoid_
{
	friend my_actor;
private:
	typedef std::shared_ptr<MsgPool_> handle;

	MsgPool_(const shared_strand& strand)
		:MsgPoolVoid_(strand){}

	~MsgPool_(){}

	static handle make(const shared_strand& strand, size_t fixedSize)
	{
		handle res(new MsgPool_(strand), [](MsgPool_* p){delete p; });
		res->_weakThis = res;
		return res;
	}
};

template <>
class MsgPump_<> : public MsgPumpVoid_
{
	friend my_actor;
	friend mutex_block_pump<>;
public:
	typedef MsgPump_* handle;
private:
	MsgPump_(const actor_handle& hostActor)
		:MsgPumpVoid_(hostActor){}

	~MsgPump_(){}

	static std::shared_ptr<MsgPump_> make(const actor_handle& hostActor)
	{
		std::shared_ptr<MsgPump_> res(new MsgPump_(hostActor), [](MsgPump_* p){delete p; });
		res->_weakThis = res;
		return res;
	}
};

template <typename... ARGS>
class msg_pump_handle
{
	friend my_actor;
	friend mutex_block_pump<ARGS...>;

	typedef MsgPump_<ARGS...> pump;

	pump* operator ->() const
	{
		return _handle;
	}

	pump* _handle;
};

template <typename... ARGS>
class post_actor_msg
{
	typedef MsgPool_<ARGS...> msg_pool_type;
public:
	post_actor_msg(){}
	post_actor_msg(const std::shared_ptr<msg_pool_type>& msgPool, const actor_handle& hostActor)
		:_msgPool(msgPool), _hostActor(hostActor){}
public:
	template <typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		assert(!empty());
		_msgPool->push_msg(std::tuple<TYPE_PIPE(ARGS)...>(TRY_MOVE(args)...), _hostActor);
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		assert(!empty());
		_msgPool->push_msg(_hostActor);
	}

	std::function<void(ARGS...)> case_func() const
	{
		return std::function<void(ARGS...)>(*this);
	}

	bool empty() const
	{
		return !_msgPool;
	}

	void clear()
	{
		_hostActor.reset();
		_msgPool.reset();
	}

	operator bool() const
	{
		return !empty();
	}
private:
	actor_handle _hostActor;
	std::shared_ptr<msg_pool_type> _msgPool;
};
//////////////////////////////////////////////////////////////////////////
class MutexBlock_
{
	friend my_actor;
private:
	virtual bool ready() = 0;
	virtual void cancel() = 0;
	virtual bool go() = 0;
	virtual size_t snap_id() = 0;

	MutexBlock_(const MutexBlock_&) {}
	void operator =(const MutexBlock_&) {}
protected:
	MutexBlock_() {}

	template <typename... Args>
	msg_pump_handle<Args...> connect_msg_pump(const int id, my_actor* host)
	{
		return my_actor::_connect_msg_pump<Args...>(id, host);
	}
};

/*!
@brief actor_msg_handle消息互斥执行块
*/
template <typename... ARGS>
class mutex_block_msg : public MutexBlock_
{
	typedef actor_msg_handle<ARGS...> msg_handle;
	typedef DstReceiverBuff_<ARGS...> dst_receiver;

	friend my_actor;
public:
	template <typename Handler>
	mutex_block_msg(msg_handle& msgHandle, Handler&& handler)
		:_msgHandle(msgHandle), _handler(TRY_MOVE(handler)) {}
private:
	bool ready()
	{
		assert(!_msgBuff.has());
		return _msgHandle.read_msg(_msgBuff);
	}

	void cancel()
	{
		_msgHandle.stop_waiting();
	}

	bool go()
	{
		if (_msgBuff.has())
		{
			bool r = tuple_invoke<bool>(_handler, std::move(_msgBuff._dstBuff.get()));
			_msgBuff.clear();
			return r;
		}
		return false;
	}

	size_t snap_id()
	{
		return (size_t)&_msgHandle;
	}
private:
	msg_handle& _msgHandle;
	std::function<bool(ARGS...)> _handler;
	dst_receiver _msgBuff;
};

/*!
@brief actor_trig_handle消息互斥执行块
*/
template <typename... ARGS>
class mutex_block_trig : public MutexBlock_
{
	typedef actor_trig_handle<ARGS...> msg_handle;
	typedef DstReceiverBuff_<ARGS...> dst_receiver;

	friend my_actor;
public:
	template <typename Handler>
	mutex_block_trig(msg_handle& msgHandle, Handler&& handler)
		:_msgHandle(msgHandle), _handler(TRY_MOVE(handler)), _triged(false) {}
private:
	bool ready()
	{
		if (!_triged)
		{
			assert(!_msgBuff.has());
			return _msgHandle.read_msg(_msgBuff);
		}
		return false;
	}

	void cancel()
	{
		_msgHandle.stop_waiting();
	}

	bool go()
	{
		if (_msgBuff.has())
		{
			_triged = true;
			bool r = tuple_invoke<bool>(_handler, std::move(_msgBuff._dstBuff.get()));
			_msgBuff.clear();
			return r;
		}
		return false;
	}

	size_t snap_id()
	{
		return (size_t)&_msgHandle;
	}
private:
	msg_handle& _msgHandle;
	std::function<bool(ARGS...)> _handler;
	dst_receiver _msgBuff;
	bool _triged;
};

/*!
@brief msg_pump消息互斥执行块
*/
template <typename... ARGS>
class mutex_block_pump : public MutexBlock_
{
	typedef msg_pump_handle<ARGS...> pump_handle;
	typedef DstReceiverBuff_<ARGS...> dst_receiver;

	friend my_actor;
public:
	template <typename Handler>
	mutex_block_pump(my_actor* host, Handler&& handler)
		:_handler(TRY_MOVE(handler))
	{
		_msgHandle = connect_msg_pump<ARGS...>(0, host);
	}

	template <typename Handler>
	mutex_block_pump(const int id, my_actor* host, Handler&& handler)
		: _handler(TRY_MOVE(handler))
	{
		_msgHandle = connect_msg_pump<ARGS...>(id, host);
	}

	template <typename Handler>
	mutex_block_pump(const pump_handle& pump, Handler&& handler)
		: _msgHandle(pump), _handler(TRY_MOVE(handler)) {}
private:
	bool ready()
	{
		assert(!_msgBuff.has());
		return _msgHandle._handle->read_msg(_msgBuff);
	}

	void cancel()
	{
		_msgHandle._handle->stop_waiting();
	}

	bool go()
	{
		if (_msgBuff.has())
		{
			bool r = tuple_invoke<bool>(_handler, std::move(_msgBuff._dstBuff.get()));
			_msgBuff.clear();
			return r;
		}
		return false;
	}

	size_t snap_id()
	{
		return (size_t)_msgHandle._handle;
	}
private:
	pump_handle _msgHandle;
	std::function<bool(ARGS...)> _handler;
	dst_receiver _msgBuff;
};

template <>
class mutex_block_msg<> : public MutexBlock_
{
	typedef actor_msg_handle<> msg_handle;

	friend my_actor;
public:
	template <typename Handler>
	mutex_block_msg(msg_handle& msgHandle, Handler&& handler)
		:_msgHandle(msgHandle), _handler(TRY_MOVE(handler)), _has(false) {}
private:
	bool ready()
	{
		assert(!_has);
		return _msgHandle.read_msg(_has);
	}

	void cancel()
	{
		_msgHandle.stop_waiting();
	}

	bool go()
	{
		if (_has)
		{
			_has = false;
			return _handler();
		}
		return false;
	}

	size_t snap_id()
	{
		return (size_t)&_msgHandle;
	}
private:
	msg_handle& _msgHandle;
	std::function<bool()> _handler;
	bool _has;
};

template <>
class mutex_block_trig<> : public MutexBlock_
{
	typedef actor_trig_handle<> msg_handle;

	friend my_actor;
public:
	template <typename Handler>
	mutex_block_trig(msg_handle& msgHandle, Handler&& handler)
		:_msgHandle(msgHandle), _handler(TRY_MOVE(handler)), _has(false), _triged(false) {}
private:
	bool ready()
	{
		if (!_triged)
		{
			assert(!_has);
			return _msgHandle.read_msg(_has);
		}
		return false;
	}

	void cancel()
	{
		_msgHandle.stop_waiting();
	}

	bool go()
	{
		if (_has)
		{
			_triged = true;
			_has = false;
			return _handler();
		}
		return false;
	}

	size_t snap_id()
	{
		return (size_t)&_msgHandle;
	}
private:
	msg_handle& _msgHandle;
	std::function<bool()> _handler;
	bool _has;
	bool _triged;
};

template <>
class mutex_block_pump<> : public MutexBlock_
{
	typedef msg_pump_handle<> pump_handle;

	friend my_actor;
public:
	template <typename Handler>
	mutex_block_pump(my_actor* host, Handler&& handler)
		:_handler(TRY_MOVE(handler)), _has(false)
	{
		_msgHandle = host->connect_msg_pump<>(0);
	}
	
	template <typename Handler>
	mutex_block_pump(const int id, my_actor* host, Handler&& handler)
		:_handler(TRY_MOVE(handler)), _has(false)
	{
		_msgHandle = host->connect_msg_pump<>(id);
	}

	template <typename Handler>
	mutex_block_pump(const pump_handle& pump, Handler&& handler)
		: _msgHandle(pump), _handler(TRY_MOVE(handler)), _has(false) {}
private:
	bool ready()
	{
		assert(!_has);
		return _msgHandle._handle->read_msg(_has);
	}

	void cancel()
	{
		_msgHandle._handle->stop_waiting();
	}

	bool go()
	{
		if (_has)
		{
			_has = false;
			return _handler();
		}
		return false;
	}

	size_t snap_id()
	{
		return (size_t)_msgHandle._handle;
	}
private:
	pump_handle _msgHandle;
	std::function<bool()> _handler;
	bool _has;
};

//////////////////////////////////////////////////////////////////////////

class TrigOnceBase_
{
protected:
	TrigOnceBase_()
		DEBUG_OPERATION(:_pIsTrig(new boost::atomic<bool>(false)))
	{
	}

	TrigOnceBase_(const TrigOnceBase_& s)
		:_hostActor(s._hostActor)
	{
		DEBUG_OPERATION(_pIsTrig = s._pIsTrig);
	}

	virtual ~TrigOnceBase_() {}
protected:
	template <typename DST, typename... Args>
	void _trig_handler(DST& dstRec, Args&&... args) const
	{
		assert(!_pIsTrig->exchange(true));
		assert(_hostActor);
		_hostActor->_trig_handler(dstRec, TRY_MOVE(args)...);
		reset();
	}

	template <typename DST, typename... Args>
	void _trig_handler2(DST& dstRec, Args&&... args) const
	{
		assert(!_pIsTrig->exchange(true));
		assert(_hostActor);
		_hostActor->_trig_handler2(dstRec, TRY_MOVE(args)...);
		reset();
	}

	template <typename DST, typename... Args>
	void _dispatch_handler(DST& dstRec, Args&&... args) const
	{
		assert(!_pIsTrig->exchange(true));
		assert(_hostActor);
		_hostActor->_dispatch_handler(dstRec, TRY_MOVE(args)...);
		reset();
	}

	template <typename DST, typename... Args>
	void _dispatch_handler2(DST& dstRec, Args&&... args) const
	{
		assert(!_pIsTrig->exchange(true));
		assert(_hostActor);
		_hostActor->_dispatch_handler2(dstRec, TRY_MOVE(args)...);
		reset();
	}

	void tick_handler() const;
	void dispatch_handler() const;
	void push_yield() const;
	virtual void reset() const = 0;
private:
	void operator =(const TrigOnceBase_&);
protected:
	mutable actor_handle _hostActor;
	DEBUG_OPERATION(std::shared_ptr<boost::atomic<bool> > _pIsTrig);
};

template <typename... ARGS>
class trig_once_notifer: public TrigOnceBase_
{
	typedef std::tuple<ARGS&...> dst_receiver;

	friend my_actor;
public:
	trig_once_notifer() :_dstRec(0) {}
	~trig_once_notifer() {}
private:
	trig_once_notifer(const actor_handle& hostActor, dst_receiver* dstRec)
		:_dstRec(dstRec) {
		_hostActor = hostActor;
	}
public:
	template <typename...  Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		_trig_handler(*_dstRec, TRY_MOVE(args)...);
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		tick_handler();
	}

	template <typename... Args>
	void dispatch(Args&&... args) const
	{
		_dispatch_handler(*_dstRec, TRY_MOVE(args)...);
	}

	void dispatch() const
	{
		dispatch_handler();
	}

	std::function<void(ARGS...)> case_func() const
	{
		return std::function<void(ARGS...)>(*this);
	}
private:
	void reset() const
	{
		_hostActor.reset();
	}
private:
	dst_receiver* _dstRec;
};
//////////////////////////////////////////////////////////////////////////

template <typename... TYPES>
class callback_handler {};

template <typename... TYPES>
class asio_cb_handler {};

/*!
@brief 异步回调器，作为回调函数参数传入，回调后，自动返回到下一行语句继续执行
*/
template <typename... ARGS, typename... OUTS>
class callback_handler<types_pck<ARGS...>, types_pck<OUTS...>>: public TrigOnceBase_
{
	typedef std::tuple<OUTS&...> dst_receiver;

	friend my_actor;
public:
	template <typename... Outs>
	callback_handler(my_actor* host, Outs&... outs)
		:_early(true), _dstRef(outs...)
	{
		_hostActor = host->shared_from_this();
	}

	~callback_handler()
	{
		if (_early)
		{
			push_yield();
			_hostActor.reset();
		}
	}

	callback_handler(const callback_handler& s)
		:TrigOnceBase_(s), _early(false), _dstRef(s._dstRef) {}
public:
	template <typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		_trig_handler2(_dstRef, try_ref_move<ARGS>::move(TRY_MOVE(args))...);
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		tick_handler();
	}
private:
	void reset() const
	{
		if (!_early)
		{
			_hostActor.reset();
		}
	}

	void operator =(const callback_handler& s)
	{
		static_assert(false, "no copy");
	}
private:
	dst_receiver _dstRef;
	const bool _early;
};

/*!
@brief ASIO库异步回调器，作为回调函数参数传入，回调后，自动返回到下一行语句继续执行
*/
template <typename... ARGS, typename... OUTS>
class asio_cb_handler<types_pck<ARGS...>, types_pck<OUTS...>> : public TrigOnceBase_
{
	typedef std::tuple<OUTS&...> dst_receiver;

	friend my_actor;
public:
	template <typename... Outs>
	asio_cb_handler(my_actor* host, Outs&... outs)
		:_early(true), _dstRef(outs...)
	{
		_hostActor = host->shared_from_this();
	}

	~asio_cb_handler()
	{
		if (_early)
		{
			push_yield();
			_hostActor.reset();
		}
	}

	asio_cb_handler(const asio_cb_handler& s)
		:TrigOnceBase_(s), _early(false), _dstRef(s._dstRef) {}
public:
	template <typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		_dispatch_handler2(_dstRef, try_ref_move<ARGS>::move(TRY_MOVE(args))...);
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		dispatch_handler();
	}
private:
	void reset() const
	{
		if (!_early)
		{
			_hostActor.reset();
		}
	}

	void operator =(const asio_cb_handler& s)
	{
		static_assert(false, "no copy");
	}
private:
	dst_receiver _dstRef;
	const bool _early;
};

//////////////////////////////////////////////////////////////////////////
/*!
@brief 子Actor句柄，不可拷贝
*/
class child_actor_handle 
{
public:
	typedef std::shared_ptr<child_actor_handle> ptr;
private:
	friend my_actor;
	/*!
	@brief 子Actor句柄参数，child_actor_handle内使用
	*/
	struct child_actor_param
	{
#ifdef _DEBUG
		child_actor_param();
		child_actor_param(child_actor_param& s);
		~child_actor_param();
		child_actor_param& operator =(child_actor_param& s);
		bool _isCopy;
#endif
		actor_handle _actor;///<本Actor
		msg_list_shared_alloc<actor_handle>::iterator _actorIt;///<保存在父Actor集合中的节点
	};
private:
	child_actor_handle(child_actor_handle&);
	child_actor_handle& operator =(child_actor_handle&);
	void peel();
public:
	child_actor_handle();
	child_actor_handle(child_actor_param& s);
	~child_actor_handle();
	child_actor_handle& operator =(child_actor_param& s);
	actor_handle get_actor();
	my_actor* operator ->();
	static ptr make_ptr();
	bool empty();
private:
	DEBUG_OPERATION(msg_list_shared_alloc<std::function<void()> >::iterator _qh);
	bool _quited;///<检测是否已经关闭
	actor_trig_handle<> _quiteAth;
	child_actor_param _param;
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief Actor对象
*/
class my_actor
{
	struct suspend_resume_option 
	{
		bool _isSuspend;
		std::function<void ()> _h;
	};

	struct msg_pool_status 
	{
		struct id_key
		{
			id_key(size_t hash, const int id)
			:_typeHash(hash), _id(id) {}

			bool operator < (const id_key& r) const
			{
				return _typeHash < r._typeHash || (_typeHash == r._typeHash && _id < r._id);
			}

			const size_t _typeHash;
			const int _id;
		};

		msg_pool_status()
		:_msgTypeMap(_msgTypeMapAll) {}

		~msg_pool_status() {}

		struct pck_base 
		{
			pck_base() :_amutex(_strand) { assert(false); }

			pck_base(my_actor* hostActor)
			:_strand(hostActor->_strand), _amutex(_strand), _isHead(true), _hostActor(hostActor){}

			virtual ~pck_base() {}

			virtual void close() = 0;

			bool is_closed()
			{
				return !_hostActor;
			}

			void lock(my_actor* self)
			{
				_amutex.lock(self);
			}

			void unlock(my_actor* self)
			{
				_amutex.unlock(self);
			}

			shared_strand _strand;
			actor_mutex _amutex;
			bool _isHead;
			my_actor* _hostActor;
		};

		template <typename... ARGS>
		struct pck: public pck_base
		{
			pck(my_actor* hostActor)
			:pck_base(hostActor){}

			void close()
			{
				if (_msgPump)
				{
					_msgPump->close();
				}
				if (_msgPool)
				{
					_msgPool->_closed = true;
				}
				_hostActor = NULL;
			}

			std::shared_ptr<MsgPool_<ARGS...> > _msgPool;
			std::shared_ptr<MsgPump_<ARGS...> > _msgPump;
			std::shared_ptr<pck> _next;
		};

		void clear(my_actor* self)
		{
			for (auto it = _msgTypeMap.begin(); it != _msgTypeMap.end(); it++) { it->second->_amutex.quited_lock(self); }
			for (auto it = _msgTypeMap.begin(); it != _msgTypeMap.end(); it++) { it->second->close(); }
			for (auto it = _msgTypeMap.begin(); it != _msgTypeMap.end(); it++) { it->second->_amutex.quited_unlock(self); }
			_msgTypeMap.clear();
		}

		msg_map_shared_alloc<id_key, std::shared_ptr<pck_base> > _msgTypeMap;
		static msg_map_shared_alloc<id_key, std::shared_ptr<pck_base> >::shared_node_alloc _msgTypeMapAll;
	};

	template <typename DST, typename ARG>
	struct async_invoke_handler
	{
		async_invoke_handler(const actor_handle& host, DST& dst)
		:_sharedThis(host), _dstRec(dst) {}

		template <typename Arg>
		void operator ()(Arg&& arg)
		{
			_sharedThis->_trig_handler(_dstRec, TRY_MOVE(arg));
		}

		actor_handle _sharedThis;
		DST& _dstRec;
	};

	template <typename DST, typename ARG>
	struct async_invoke_handler<DST, ARG&>
	{
		async_invoke_handler(const actor_handle& host, DST& dst)
		:_sharedThis(host), _dstRec(dst) {}

		void operator ()(const TYPE_PIPE(ARG&)& arg)
		{
			_sharedThis->_trig_handler(_dstRec, arg);
		}

		actor_handle _sharedThis;
		DST& _dstRec;
	};

	template <typename DST, typename... ARGS>
	struct trig_cb_handler
	{
		template <typename Dst, typename... Args>
		trig_cb_handler(const actor_handle& host, Dst& dst, Args&&... args)
			:_sharedThis(host), _dst(dst), _args(TRY_MOVE(args)...) {}

		trig_cb_handler(const trig_cb_handler& s)
			:_sharedThis(std::move(s._sharedThis)), _dst(s._dst), _args(std::move(s._args)) {}

		void operator ()()
		{
			if (!_sharedThis->_quited)
			{
				TupleReceiverRef_(_dst, std::move(_args));
				_sharedThis->pull_yield();
			}
		}

		DST& _dst;
		mutable std::tuple<ARGS...> _args;
		mutable actor_handle _sharedThis;
	};

	template <typename DST, typename... ARGS>
	struct trig_cb_handler2
	{
		template <typename Dst, typename... Args>
		trig_cb_handler2(const actor_handle& host, Dst& dst, Args&&... args)
			:_sharedThis(host), _dst(dst), _args(TRY_MOVE(args)...) {}

		trig_cb_handler2(const trig_cb_handler2& s)
			:_sharedThis(std::move(s._sharedThis)), _dst(s._dst), _args(std::move(s._args)) {}

		void operator ()()
		{
			if (!_sharedThis->_quited)
			{
				TupleReceiverRef_(_dst, std::move(_args));
				_sharedThis->pull_yield();
			}
		}

		DST _dst;
		mutable std::tuple<ARGS...> _args;
		mutable actor_handle _sharedThis;
	};


	struct timer_state 
	{
		bool _timerSuspend : 1;
		bool _timerCompleted : 1;
		long long _timerTime;
		long long _timerStampBegin;
		long long _timerStampEnd;
		std::function<void()> _timerCb;
		ActorTimer_::timer_handle _timerHandle;
	};

	class boost_actor_run;
	friend boost_actor_run;
	friend child_actor_handle;
	friend MsgPumpBase_;
	friend actor_msg_handle_base;
	friend TrigOnceBase_;
	friend mutex_trig_notifer;
	friend mutex_trig_handle;
	friend io_engine;
	friend ActorTimer_;
	friend ActorMutex_;
	friend MutexBlock_;
public:
	/*!
	@brief 在{}一定范围内锁定当前Actor不被强制退出，如果锁定期间被挂起，将无法等待其退出
	*/
	class quit_guard
	{
	public:
		quit_guard(my_actor* self)
		:_self(self)
		{
			_locked = true;
			_self->lock_quit();
		}

		~quit_guard()
		{
			if (_locked)
			{
				//可能在此析构函数内抛出 force_quit_exception 异常，但在 unlock_quit 已经切换出堆栈，在切换回来后会安全的释放资源
				_self->unlock_quit();
			}
		}

		void lock()
		{
			assert(!_locked);
			_locked = true;
			_self->lock_quit();
		}

		void unlock()
		{
			assert(_locked);
			_locked = false;
			_self->unlock_quit();
		}
	private:
		quit_guard(const quit_guard&) {}
		void operator=(const quit_guard&) {}
		my_actor* _self;
		bool _locked;
	};

	/*!
	@brief Actor被强制退出的异常类型
	*/
	struct force_quit_exception { };

	/*!
	@brief 消息泵被断开
	*/
	struct pump_disconnected_exception { };

	/*!
	@brief Actor入口函数体
	*/
	typedef std::function<void (my_actor*)> main_func;

	/*!
	@brief actor id
	*/
	typedef long long id;
private:
	my_actor();
	my_actor(const my_actor&);
	my_actor& operator =(const my_actor&);
	~my_actor();
public:
	/*!
	@brief 创建一个Actor
	@param actorStrand Actor所依赖的strand
	@param mainFunc Actor执行入口
	@param stackSize Actor栈大小，默认64k字节，必须是4k的整数倍，最小4k，最大1M
	*/
	static actor_handle create(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);
public:
	/*!
	@brief 创建一个子Actor，父Actor终止时，子Actor也终止（在子Actor都完全退出后，父Actor才结束）
	@param actorStrand 子Actor依赖的strand
	@param mainFunc 子Actor入口函数
	@param stackSize Actor栈大小，4k的整数倍（最大1MB）
	@return 子Actor句柄，使用 child_actor_handle 接收返回值
	*/
	child_actor_handle::child_actor_param create_child_actor(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);
	child_actor_handle::child_actor_param create_child_actor(const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 开始运行子Actor，只能调用一次
	*/
	void child_actor_run(child_actor_handle& actorHandle);

	template <typename... Handles>
	void child_actor_run(Handles&... handles)
	{
		static_assert(sizeof...(Handles) > 1, "");
		child_actor_handle* handles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_run(*handles[i]);
		}
	}

	/*!
	@brief 开始运行一组子Actor，只能调用一次
	*/
	void child_actors_run(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 强制终止一个子Actor
	*/
	__yield_interrupt void child_actor_force_quit(child_actor_handle& actorHandle);

	template <typename... Handles>
	__yield_interrupt void child_actor_force_quit(Handles&... handles)
	{
		static_assert(sizeof...(Handles) > 1, "");
		child_actor_handle* handles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_force_quit(*handles[i]);
		}
	}

	__yield_interrupt void child_actors_force_quit(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 等待一个子Actor完成后返回
	@return 正常退出的返回true，否则false
	*/
	__yield_interrupt void child_actor_wait_quit(child_actor_handle& actorHandle);

	template <typename... Handles>
	__yield_interrupt void child_actor_wait_quit(Handles&... handles)
	{
		static_assert(sizeof...(Handles) > 1, "");
		child_actor_handle* handles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_wait_quit(*handles[i]);
		}
	}

	__yield_interrupt bool timed_child_actor_wait_quit(int tm, child_actor_handle& actorHandle);

	/*!
	@brief 等待一组子Actor完成后返回
	@return 都正常退出的返回true，否则false
	*/
	__yield_interrupt void child_actors_wait_quit(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 挂起子Actor
	*/
	__yield_interrupt void child_actor_suspend(child_actor_handle& actorHandle);

	template <typename... Handles>
	__yield_interrupt void child_actor_suspend(Handles&... handles)
	{
		static_assert(sizeof...(Handles) > 1, "");
		child_actor_handle* handles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_suspend(*handles[i]);
		}
	}
	
	/*!
	@brief 挂起一组子Actor
	*/
	__yield_interrupt void child_actors_suspend(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 恢复子Actor
	*/
	__yield_interrupt void child_actor_resume(child_actor_handle& actorHandle);

	template <typename... Handles>
	__yield_interrupt void child_actor_resume(Handles&... handles)
	{
		static_assert(sizeof...(Handles) > 1, "");
		child_actor_handle* handles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_resume(*handles[i]);
		}
	}
	
	/*!
	@brief 恢复一组子Actor
	*/
	__yield_interrupt void child_actors_resume(const list<child_actor_handle::ptr>& actorHandles);

	/*!
	@brief 创建另一个Actor，Actor执行完成后返回
	*/
	__yield_interrupt void run_child_actor_complete(const shared_strand& actorStrand, const main_func& h, size_t stackSize = DEFAULT_STACKSIZE);
	__yield_interrupt void run_child_actor_complete(const main_func& h, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 延时等待，Actor内部禁止使用操作系统API Sleep()
	@param ms 等待毫秒数，等于0时暂时放弃Actor执行，直到下次被调度器触发
	*/
	__yield_interrupt void sleep(int ms);
	__yield_interrupt void sleep_guard(int ms);

	/*!
	@brief 中断当前时间片，等到下次被调度(因为Actor是非抢占式调度，当有占用时间片较长的逻辑时，适当使用yield分割时间片)
	*/
	__yield_interrupt void yield();

	/*!
	@brief 中断时间片，当前Actor句柄在别的地方必须被持有
	*/
	__yield_interrupt void yield_guard();

	/*!
	@brief 获取父Actor
	*/
	actor_handle parent_actor();

	/*!
	@brief 获取子Actor
	*/
	const msg_list_shared_alloc<actor_handle>& child_actors();
public:
	typedef msg_list_shared_alloc<std::function<void()> >::iterator quit_iterator;

	/*!
	@brief 注册一个资源释放函数，在强制退出Actor时调用
	*/
	quit_iterator regist_quit_handler(const std::function<void ()>& quitHandler);

	/*!
	@brief 注销资源释放函数
	*/
	void cancel_quit_handler(quit_iterator qh);
public:
	/*!
	@brief 使用内部定时器延时触发某个函数，在触发完成之前不能多次调用
	@param ms 触发延时(毫秒)
	@param h 触发函数
	*/
	template <typename H>
	void delay_trig(int ms, H&& h)
	{
		assert_enter();
		if (ms > 0)
		{
			assert(_timer);
			timeout(ms, TRY_MOVE(h));
		} 
		else if (0 == ms)
		{
			_strand->post(TRY_MOVE(h));
		}
		else
		{
			assert(false);
		}
	}

	/*!
	@brief 取消内部定时器触发
	*/
	void cancel_delay_trig();
public:
	/*!
	@brief 发送一个异步函数到shared_strand中执行（如果是和self一样的shared_strand直接执行），配合quit_guard使用防止引用失效，完成后返回
	*/
	template <typename H>
	__yield_interrupt void send(const shared_strand& exeStrand, H&& h)
	{
		assert_enter();
		if (exeStrand != _strand)
		{
			actor_handle shared_this = shared_from_this();
			exeStrand->asyncInvokeVoid(TRY_MOVE(h), [shared_this]
			{
				shared_this->post_handler();
			});
			push_yield();
			return;
		}
		h();
	}

	template <typename Arg, typename H>
	__yield_interrupt Arg send(const shared_strand& exeStrand, H&& h)
	{
		assert_enter();
		if (exeStrand != _strand)
		{
			std::tuple<stack_obj<TYPE_PIPE(Arg)>> dstRec;
			exeStrand->asyncInvoke(TRY_MOVE(h), async_invoke_handler<std::tuple<stack_obj<TYPE_PIPE(Arg)>>, Arg>(shared_from_this(), dstRec));
			push_yield();
			return std::move(std::get<0>(dstRec).get());
		} 
		return h();
	}

	/*!
	@brief 往当前"系统线程"堆栈中抛出一个任务，完成后返回，（用于消耗堆栈高的函数）
	*/
	template <typename H>
	__yield_interrupt void run_in_thread_stack(H&& h)
	{
		assert_enter();
		actor_handle shared_this = shared_from_this();
		_strand->asyncInvokeVoid(TRY_MOVE(h), [shared_this]
		{
			shared_this->next_tick_handler();
		});
		push_yield();
	}

	template <typename Arg, typename H>
	__yield_interrupt Arg run_in_thread_stack(H&& h)
	{
		return async_send<Arg>(_strand, TRY_MOVE(h));
	}

	/*!
	@brief 强制将一个函数发送到一个shared_strand中执行（比如某个API会进行很多层次的堆栈调用，而当前Actor堆栈不够，可以用此切换到线程堆栈中直接执行），
		配合quit_guard使用防止引用失效，完成后返回
	*/
	template <typename H>
	__yield_interrupt void async_send(const shared_strand& exeStrand, H&& h)
	{
		assert_enter();
		actor_handle shared_this = shared_from_this();
		exeStrand->asyncInvokeVoid(TRY_MOVE(h), [shared_this]
		{
			shared_this->tick_handler();
		});
		push_yield();
	}

	template <typename Arg, typename H>
	__yield_interrupt Arg async_send(const shared_strand& exeStrand, H&& h)
	{
		assert_enter();
		std::tuple<stack_obj<TYPE_PIPE(Arg)>> dstRec;
		exeStrand->asyncInvoke(TRY_MOVE(h), async_invoke_handler<std::tuple<stack_obj<TYPE_PIPE(Arg)>>, Arg>(shared_from_this(), dstRec));
		push_yield();
		return std::move(std::get<0>(dstRec).get());
	}

	template <typename H>
	__yield_interrupt void async_send_self(H&& h)
	{
		async_send(_strand, TRY_MOVE(h));
	}

	template <typename Arg, typename H>
	__yield_interrupt Arg async_send_self(H&& h)
	{
		return async_send<Arg>(_strand, TRY_MOVE(h));
	}

	/*!
	@brief 调用一个异步函数，异步回调完成后返回
	*/
	template <typename H>
	__yield_interrupt void trig(const H& h)
	{
		assert_enter();
		h(trig_once_notifer<>(shared_from_this(), NULL));
		push_yield();
	}

	template <typename... DArgs, typename H>
	__yield_interrupt void trig(__out DArgs&... dargs, const H& h)
	{
		assert_enter();
		std::tuple<DArgs&...> res(dargs...);
		h(trig_once_notifer<DArgs...>(shared_from_this(), &res));
		push_yield();
	}

	template <typename R, typename H>
	__yield_interrupt R trig(const H& h)
	{
		R res;
		trig<R>(res, h);
		return res;
	}
private:
	void post_handler();
	void dispatch_handler();
	void tick_handler();
	void next_tick_handler();

	template <typename DST, typename... ARGS>
	void _trig_handler(DST& dstRec, ARGS&&... args)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				TupleReceiver_(dstRec, TRY_MOVE(args)...);
				next_tick_handler();
			}
		} 
		else
		{
			_strand->post(trig_cb_handler<DST, RM_CREF(ARGS)...>(shared_from_this(), dstRec, TRY_MOVE(args)...));
		}
	}

	template <typename DST, typename... ARGS>
	void _trig_handler2(DST& dstRec, ARGS&&... args)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				TupleReceiver_(dstRec, TRY_MOVE(args)...);
				next_tick_handler();
			}
		}
		else
		{
			_strand->post(trig_cb_handler2<DST, RM_CREF(ARGS)...>(shared_from_this(), dstRec, TRY_MOVE(args)...));
		}
	}

	template <typename DST, typename... ARGS>
	void _dispatch_handler(DST& dstRec, ARGS&&... args)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				TupleReceiver_(dstRec, TRY_MOVE(args)...);
				next_tick_handler();
			}
		} 
		else
		{
			_strand->dispatch(trig_cb_handler<DST, RM_CREF(ARGS)...>(shared_from_this(), dstRec, TRY_MOVE(args)...));
		}
	}

	template <typename DST, typename... ARGS>
	void _dispatch_handler2(DST& dstRec, ARGS&&... args)
	{
		if (_strand->running_in_this_thread())
		{
			if (!_quited)
			{
				TupleReceiver_(dstRec, TRY_MOVE(args)...);
				next_tick_handler();
			}
		} 
		else
		{
			_strand->dispatch(trig_cb_handler2<DST, RM_CREF(ARGS)...>(shared_from_this(), dstRec, TRY_MOVE(args)...));
		}
	}
private:
	template <typename AMH, typename DST>
	bool _timed_wait_msg(AMH& amh, DST& dstRec, int tm)
	{
		assert(amh._hostActor && amh._hostActor->self_id() == self_id());
		if (!amh.read_msg(dstRec))
		{
			if (0 != tm)
			{
				bool timed = false;
				if (tm > 0)
				{
					delay_trig(tm, [this, &timed]
					{
						if (!_quited)
						{
							timed = true;
							pull_yield();
						}
					});
				}
				push_yield();
				if (!timed)
				{
					if (tm > 0)
					{
						cancel_delay_trig();
					}
					return true;
				}
			}
			amh.stop_waiting();
			return false;
		}
		return true;
	}

	template <typename AMH, typename DST>
	void _wait_msg(AMH& amh, DST& dstRec)
	{
		assert(amh._hostActor && amh._hostActor->self_id() == self_id());
		if (!amh.read_msg(dstRec))
		{
			push_yield();
		}
	}
public:
	/*!
	@brief 创建一个消息通知函数
	*/
	template <typename... Args>
	actor_msg_notifer<Args...> make_msg_notifer(actor_msg_handle<Args...>& amh)
	{
		return amh.make_notifer(shared_from_this());
	}

	/*!
	@brief 关闭消息通知句柄
	*/
	void close_msg_notifer(actor_msg_handle_base& amh);

	/*!
	@brief 从消息句柄中提取消息
	@param tm 超时时间
	@return 超时完成返回false，成功提取消息返回true
	*/
	template <typename... Args, typename... Outs>
	__yield_interrupt bool timed_wait_msg(int tm, actor_msg_handle<Args...>& amh, Outs&... res)
	{
		assert_enter();
		DstReceiverRef_<types_pck<Args...>, types_pck<Outs...>> dstRec(res...);
		return _timed_wait_msg(amh, dstRec, tm);
	}

	__yield_interrupt bool timed_wait_msg(int tm, actor_msg_handle<>& amh);

	template <typename... Args, typename Handler>
	__yield_interrupt bool timed_wait_msg(int tm, actor_msg_handle<Args...>& amh, const Handler& h)
	{
		assert_enter();
		DstReceiverBuff_<Args...> dstRec;
		if (_timed_wait_msg(amh, dstRec, tm))
		{
			tuple_invoke(h, std::move(dstRec._dstBuff.get()));
			return true;
		}
		return false;
	}

	template <typename... Args, typename... Outs>
	__yield_interrupt bool try_wait_msg(actor_msg_handle<Args...>& amh, Outs&... res)
	{
		return timed_wait_msg(0, amh, res...);
	}

	__yield_interrupt bool try_wait_msg(actor_msg_handle<>& amh);

	template <typename... Args, typename Handler>
	__yield_interrupt bool try_wait_msg(actor_msg_handle<Args...>& amh, const Handler& h)
	{
		return timed_wait_msg(0, amh, h);
	}

	/*!
	@brief 从消息句柄中提取消息
	*/
	template <typename... Args, typename... Outs>
	__yield_interrupt void wait_msg(actor_msg_handle<Args...>& amh, Outs&... res)
	{
		timed_wait_msg(-1, amh, res...);
	}

	__yield_interrupt void wait_msg(actor_msg_handle<>& amh);

	template <typename Arg>
	__yield_interrupt Arg wait_msg(actor_msg_handle<Arg>& amh)
	{
		assert_enter();
		DstReceiverBuff_<Arg> dstRec;
		_wait_msg(amh, dstRec);
		return std::move(std::get<0>(dstRec._dstBuff.get()));
	}

	template <typename... Args, typename Handler>
	__yield_interrupt void wait_msg(actor_msg_handle<Args...>& amh, const Handler& h)
	{
		timed_wait_msg<Args...>(-1, amh, h);
	}
public:
	/*!
	@brief 创建上下文回调函数，直接作为回调函数使用，async_func(..., Handler self->make_context(...))
	*/
	template <typename... Args, typename... Outs>
	callback_handler<types_pck<Args...>, types_pck<Outs...>> make_context_as_type(Outs&... outs)
	{
		static_assert(sizeof...(Args) == sizeof...(Outs), "");
		assert_enter();
		return callback_handler<types_pck<Args...>, types_pck<Outs...>>(this, outs...);
	}

	template <typename... Outs>
	callback_handler<types_pck<Outs...>, types_pck<Outs...>> make_context(Outs&... outs)
	{
		assert_enter();
		return callback_handler<types_pck<Outs...>, types_pck<Outs...>>(this, outs...);
	}

	/*!
	@brief 创建ASIO库上下文回调函数，直接作为回调函数使用，async_func(..., Handler self->make_asio_context(...))
	*/
	template <typename... Args, typename... Outs>
	asio_cb_handler<types_pck<Args...>, types_pck<Outs...>> make_asio_context_as_type(Outs&... outs)
	{
		static_assert(sizeof...(Args) == sizeof...(Outs), "");
		assert_enter();
		return asio_cb_handler<types_pck<Args...>, types_pck<Outs...>>(this, outs...);
	}

	template <typename... Outs>
	asio_cb_handler<types_pck<Outs...>, types_pck<Outs...>> make_asio_context(Outs&... outs)
	{
		assert_enter();
		return asio_cb_handler<types_pck<Outs...>, types_pck<Outs...>>(this, outs...);
	}

	/*!
	@brief 创建一个消息触发函数，只有一次触发有效
	*/
	template <typename... Args>
	actor_trig_notifer<Args...> make_trig_notifer(actor_trig_handle<Args...>& ath)
	{
		return ath.make_notifer(shared_from_this());
	}

	/*!
	@brief 关闭消息触发句柄
	*/
	void close_trig_notifer(actor_msg_handle_base& ath);

	/*!
	@brief 从触发句柄中提取消息
	@param tm 超时时间
	@return 超时完成返回false，成功提取消息返回true
	*/
	template <typename... Args, typename... Outs>
	__yield_interrupt bool timed_wait_trig(int tm, actor_trig_handle<Args...>& ath, Outs&... res)
	{
		assert_enter();
		DstReceiverRef_<types_pck<Args...>, types_pck<Outs...>> dstRec(res...);
		return _timed_wait_msg(ath, dstRec, tm);
	}

	__yield_interrupt bool timed_wait_trig(int tm, actor_trig_handle<>& ath);

	template <typename... Args, typename Handler>
	__yield_interrupt bool timed_wait_trig(int tm, actor_trig_handle<Args...>& ath, const Handler& h)
	{
		assert_enter();
		DstReceiverBuff_<Args...> dstRec;
		if (_timed_wait_msg(ath, dstRec, tm))
		{
			tuple_invoke(h, std::move(dstRec._dstBuff.get()));
			return true;
		}
		return false;
	}

	template <typename... Args, typename... Outs>
	__yield_interrupt bool try_wait_trig(actor_trig_handle<Args...>& ath, Outs&... res)
	{
		return timed_wait_trig(0, ath, res...);
	}

	__yield_interrupt bool try_wait_trig(actor_trig_handle<>& ath);

	template <typename... Args, typename Handler>
	__yield_interrupt bool try_wait_trig(actor_trig_handle<Args...>& ath, const Handler& h)
	{
		return timed_wait_trig(0, ath, h);
	}

	/*!
	@brief 从触发句柄中提取消息
	*/
	template <typename... Args, typename... Outs>
	__yield_interrupt void wait_trig(actor_trig_handle<Args...>& ath, Outs&... res)
	{
		timed_wait_trig(-1, ath, res...);
	}

	__yield_interrupt void wait_trig(actor_trig_handle<>& ath);

	template <typename Arg>
	__yield_interrupt Arg wait_trig(actor_trig_handle<Arg>& ath)
	{
		assert_enter();
		DstReceiverBuff_<Arg> dstRec;
		_wait_msg(ath, dstRec);
		return std::move(std::get<0>(dstRec._dstBuff.get()));
	}

	template <typename... Args, typename Handler>
	__yield_interrupt void wait_trig(actor_trig_handle<Args...>& ath, const Handler& h)
	{
		timed_wait_trig<Args...>(-1, ath, h);
	}
private:
	/*!
	@brief 寻找出与模板参数类型匹配的消息池
	*/
	template <typename... Args>
	static std::shared_ptr<msg_pool_status::pck<Args...> > msg_pool_pck(const int id, my_actor* const host, bool make = true)
	{
		typedef msg_pool_status::pck<Args...> pck_type;
		msg_pool_status::id_key typeID(sizeof...(Args) != 0 ? typeid(pck_type).hash_code() : 0, id);
		if (make)
		{
			auto& res = host->_msgPoolStatus._msgTypeMap.insert(make_pair(typeID, std::shared_ptr<pck_type>())).first->second;
			if (!res)
			{
				res = std::shared_ptr<pck_type>(new pck_type(host));
			}
			return std::static_pointer_cast<pck_type>(res);
		}
		auto it = host->_msgPoolStatus._msgTypeMap.find(typeID);
		if (it != host->_msgPoolStatus._msgTypeMap.end())
		{
			return std::static_pointer_cast<pck_type>(it->second);
		}
		return std::shared_ptr<pck_type>();
	}

	/*!
	@brief 清除消息代理链
	*/
	template <typename... Args>
	static void clear_msg_list(my_actor* const host, const std::shared_ptr<msg_pool_status::pck<Args...>>& msgPck, bool poolDis = true)
	{
		std::shared_ptr<msg_pool_status::pck<Args...>> uStack[16];
		size_t stackl = 0;
		auto pckIt = msgPck;
		while (true)
		{
			if (pckIt->_next)
			{
				pckIt->_msgPool.reset();
				pckIt = pckIt->_next;
				pckIt->lock(host);
				assert(stackl < 15);
				uStack[stackl++] = pckIt;
			}
			else
			{
				if (!pckIt->is_closed())
				{
					if (pckIt->_msgPool)
					{
						if (poolDis)
						{
							auto& msgPool_ = pckIt->_msgPool;
							auto& msgPump_ = pckIt->_msgPump;
							host->send(msgPool_->_strand, [&msgPool_, &msgPump_]
							{
								assert(msgPool_->_msgPump == msgPump_);
								msgPool_->disconnect();
							});
						}
						pckIt->_msgPool.reset();
					}
					if (pckIt->_msgPump)
					{
						auto& msgPump_ = pckIt->_msgPump;
						host->send(msgPump_->_strand, [&msgPump_]
						{
							msgPump_->clear();
						});
					}
				}
				while (stackl)
				{
					uStack[--stackl]->unlock(host);
				}
				return;
			}
		}
	}

	/*!
	@brief 更新消息代理链
	*/
	template <typename... Args>
	actor_handle update_msg_list(const std::shared_ptr<msg_pool_status::pck<Args...>>& msgPck, const std::shared_ptr<MsgPool_<Args...>>& newPool)
	{
		typedef typename MsgPool_<Args...>::pump_handler pump_handler;

		std::shared_ptr<msg_pool_status::pck<Args...>> uStack[16];
		size_t stackl = 0;
		auto pckIt = msgPck;
		while (true)
		{
			if (pckIt->_next)
			{
				pckIt->_msgPool = newPool;
				pckIt = pckIt->_next;
				pckIt->lock(this);
				assert(stackl < 15);
				uStack[stackl++] = pckIt;
			} 
			else
			{
				if (!pckIt->is_closed())
				{
					if (pckIt->_msgPool)
					{
						auto& msgPool_ = pckIt->_msgPool;
						send(msgPool_->_strand, [&msgPool_]
						{
							msgPool_->disconnect();
						});
					}
					pckIt->_msgPool = newPool;
					if (pckIt->_msgPump)
					{
						auto& msgPump_ = pckIt->_msgPump;
						if (newPool)
						{
							auto ph = send<pump_handler>(newPool->_strand, [&newPool, &msgPump_]()->pump_handler
							{
								return newPool->connect_pump(msgPump_);
							});
							send(msgPump_->_strand, [&msgPump_, &ph]
							{
								msgPump_->connect(ph);
							});
						}
						else
						{
							send(msgPump_->_strand, [&msgPump_]
							{
								msgPump_->clear();
							});
						}
					}
					while (stackl)
					{
						uStack[--stackl]->unlock(this);
					}
					return pckIt->_hostActor->shared_from_this();
				}
				while (stackl)
				{
					uStack[--stackl]->unlock(this);
				}
				return actor_handle();
			}
		}
	}
private:
	/*!
	@brief 把本Actor内消息由伙伴Actor代理处理
	*/
	template <typename... Args>
	__yield_interrupt bool msg_agent_to(const int id, actor_handle childActor)
	{
		typedef std::shared_ptr<msg_pool_status::pck<Args...>> pck_type;

		assert_enter();
		assert(childActor);
		{
			auto isSelf = childActor->parent_actor();
			if (!isSelf || isSelf->self_id() != self_id())
			{
				assert(false);
				return false;
			}
		}
		quit_guard qg(this);
		auto childPck = send<pck_type>(childActor->self_strand(), [id, &childActor]()->pck_type
		{
			if (!childActor->is_quited())
			{
				return my_actor::msg_pool_pck<Args...>(id, childActor.get());
			}
			return pck_type();
		});
		if (childPck)
		{
			auto msgPck = msg_pool_pck<Args...>(id, this);
			msgPck->lock(this);
			childPck->lock(this);
			childPck->_isHead = false;
			actor_handle hostActor = update_msg_list<Args...>(childPck, msgPck->_msgPool);
			if (hostActor)
			{
				if (msgPck->_next && msgPck->_next != childPck)
				{
					msgPck->_next->lock(this);
					clear_msg_list<Args...>(this, msgPck->_next, false);
					msgPck->_next->unlock(this);
				}
				msgPck->_next = childPck;
				childPck->unlock(this);
				msgPck->unlock(this);
				return true;
			}
			childPck->unlock(this);
			msgPck->unlock(this);
		}
		return false;
	}
public:
	template <typename... Args>
	__yield_interrupt bool msg_agent_to(const int id, child_actor_handle& childActor)
	{
		return msg_agent_to<Args...>(id, childActor.get_actor());
	}

	template <typename... Args>
	__yield_interrupt bool msg_agent_to(child_actor_handle& childActor)
	{
		return msg_agent_to<Args...>(0, childActor.get_actor());
	}
public:
	/*!
	@brief 把消息指定到一个特定Actor函数体去处理
	@return 返回处理该消息的子Actor句柄
	*/
	template <typename... Args, typename Handler>
	__yield_interrupt child_actor_handle::child_actor_param msg_agent_to_actor(const int id, const shared_strand& strand, bool autoRun, const Handler& agentActor, size_t stackSize = DEFAULT_STACKSIZE)
	{
		child_actor_handle::child_actor_param childActor = create_child_actor(strand, [id, agentActor](my_actor* self)
		{
			agentActor(self, my_actor::_connect_msg_pump<Args...>(id, self));
		}, stackSize);
		msg_agent_to<Args...>(id, childActor._actor);
		if (autoRun)
		{
			childActor._actor->notify_run();
		}
		return childActor;
	}

	template <typename... Args, typename Handler>
	__yield_interrupt child_actor_handle::child_actor_param msg_agent_to_actor(const int id, bool autoRun, const Handler& agentActor, size_t stackSize = DEFAULT_STACKSIZE)
	{
		return msg_agent_to_actor<Args...>(id, self_strand(), autoRun, agentActor, stackSize);
	}

	template <typename... Args, typename Handler>
	__yield_interrupt child_actor_handle::child_actor_param msg_agent_to_actor(const shared_strand& strand, bool autoRun, const Handler& agentActor, size_t stackSize = DEFAULT_STACKSIZE)
	{
		return msg_agent_to_actor<Args...>(0, strand, autoRun, agentActor, stackSize);
	}

	template <typename... Args, typename Handler>
	__yield_interrupt child_actor_handle::child_actor_param msg_agent_to_actor(bool autoRun, const Handler& agentActor, size_t stackSize = DEFAULT_STACKSIZE)
	{
		return msg_agent_to_actor<Args...>(0, self_strand(), autoRun, agentActor, stackSize);
	}
public:
	/*!
	@brief 断开伙伴代理该消息
	*/
	template <typename... Args>
	__yield_interrupt void msg_agent_off(const int id = 0)
	{
		assert_enter();
		auto msgPck = msg_pool_pck<Args...>(id, this, false);
		if (msgPck)
		{
			quit_guard qg(this);
			msgPck->lock(this);
			if (msgPck->_next)
			{
				msgPck->_next->lock(this);
				clear_msg_list<Args...>(this, msgPck->_next);
				msgPck->_next->_isHead = true;
				msgPck->_next->unlock(this);
				msgPck->_next.reset();
			}
			msgPck->unlock(this);
		}
	}

	/*!
	@brief 重置消息管道，所有之前连接到当前管道的通知句柄将永久失效
	*/
	template <typename... Args>
	__yield_interrupt bool reset_msg_pipe(const int id = 0)
	{
		assert_enter();
		typedef msg_pool_status::pck<Args...> pck_type;
		msg_pool_status::id_key typeID(sizeof...(Args) != 0 ? typeid(pck_type).hash_code() : 0, id);
		auto it = _msgPoolStatus._msgTypeMap.find(typeID);
		if (_msgPoolStatus._msgTypeMap.end() != it)
		{
			quit_guard qg(this);
			std::shared_ptr<pck_type> msgPck = std::static_pointer_cast<pck_type>(it->second);
			msgPck->lock(this);
			if (msgPck->_next)
			{
				msgPck->_next->lock(this);
				clear_msg_list<Args...>(this, msgPck->_next);
				msgPck->_next->_isHead = true;
				msgPck->_next->unlock(this);
				msgPck->_next.reset();
			}
			if (msgPck->_msgPool)
			{
				msgPck->_msgPool->_closed = true;
			}
			clear_msg_list<Args...>(this, msgPck);
			_msgPoolStatus._msgTypeMap.erase(it);
			msgPck->unlock(this);
			return true;
		}
		return false;
	}
public:
	/*!
	@brief 连接消息通知到一个伙伴Actor，该Actor必须是子Actor或没有父Actor
	@param strand 消息调度器
	@param id 相同类型消息id
	@param makeNew false 如果存在返回之前，否则创建新的通知；true 强制创建新的通知，之前的将失效，且断开与buddyActor的关联
	@param fixedSize 消息队列内存池长度
	@warning 如果 makeNew = false 且该节点为父的代理，将创建失败
	@return 消息通知函数
	*/
	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const shared_strand& strand, const int id, const actor_handle& buddyActor, bool makeNew = false, size_t fixedSize = 16)
	{
		typedef MsgPool_<Args...> pool_type;
		typedef typename pool_type::pump_handler pump_handler;
		typedef std::shared_ptr<msg_pool_status::pck<Args...>> pck_type;

		assert_enter();
		{
			auto buddyParent = buddyActor->parent_actor();
			if (!(buddyActor && (!buddyParent || buddyParent->self_id() == self_id())))
			{
				assert(false);
				return post_actor_msg<Args...>();
			}
		}
#ifdef _DEBUG
		{
			auto pa = parent_actor();
			while (pa)
			{
				assert(pa->self_id() != buddyActor->self_id());
				pa = pa->parent_actor();
			}
		}
#endif
		auto msgPck = msg_pool_pck<Args...>(id, this);
		quit_guard qg(this);
		msgPck->lock(this);
		auto childPck = send<pck_type>(buddyActor->self_strand(), [id, &buddyActor]()->pck_type
		{
			if (!buddyActor->is_quited())
			{
				return my_actor::msg_pool_pck<Args...>(id, buddyActor.get());
			}
			return pck_type();
		});
		if (!childPck)
		{
			msgPck->unlock(this);
			return post_actor_msg<Args...>();
		}
		if (makeNew)
		{
			auto newPool = pool_type::make(strand, fixedSize);
			childPck->lock(this);
			childPck->_isHead = true;
			actor_handle hostActor = update_msg_list<Args...>(childPck, newPool);
			childPck->unlock(this);
			if (hostActor)
			{
				//如果当前消息节点在更新的消息链中，则断开，然后连接到本地消息泵
				if (msgPck->_next == childPck)
				{
					msgPck->_next.reset();
					if (msgPck->_msgPump)
					{
						auto& msgPump_ = msgPck->_msgPump;
						if (msgPck->_msgPool)
						{
							auto& msgPool_ = msgPck->_msgPool;
							msgPump_->connect(this->send<pump_handler>(msgPool_->_strand, [&msgPool_, &msgPump_]()->pump_handler
							{
								return msgPool_->connect_pump(msgPump_);
							}));
						}
						else
						{
							msgPump_->clear();
						}
					}
				}
				msgPck->unlock(this);
				return post_actor_msg<Args...>(newPool, hostActor);
			}
			msgPck->unlock(this);
			//消息链的代理人已经退出，失败
			return post_actor_msg<Args...>();
		}
		childPck->lock(this);
		if (childPck->_isHead)
		{
			assert(msgPck->_next != childPck);
			auto childPool = childPck->_msgPool;
			if (!childPool)
			{
				childPool = pool_type::make(strand, fixedSize);
			}
			actor_handle hostActor = update_msg_list<Args...>(childPck, childPool);
			if (hostActor)
			{
				childPck->unlock(this);
				msgPck->unlock(this);
				return post_actor_msg<Args...>(childPool, hostActor);
			}
			//消息链的代理人已经退出，失败
		}
		childPck->unlock(this);
		msgPck->unlock(this);
		return post_actor_msg<Args...>();
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const shared_strand& strand, const int id, child_actor_handle& childActor, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(strand, id, childActor.get_actor(), makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const shared_strand& strand, const actor_handle& buddyActor, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(strand, 0, buddyActor, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const shared_strand& strand, child_actor_handle& childActor, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(strand, 0, childActor.get_actor(), makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const int id, const actor_handle& buddyActor, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(buddyActor->self_strand(), id, buddyActor, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const int id, child_actor_handle& childActor, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(childActor->self_strand(), id, childActor.get_actor(), makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const actor_handle& buddyActor, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(buddyActor->self_strand(), 0, buddyActor, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(child_actor_handle& childActor, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(childActor->self_strand(), 0, childActor.get_actor(), makeNew, fixedSize);
	}

	/*!
	@brief 连接消息通知到自己的Actor
	@param strand 消息调度器
	@param id 相同类型消息id
	@param makeNew false 如果存在返回之前，否则创建新的通知；true 强制创建新的通知，之前的将失效，且断开与buddyActor的关联
	@param fixedSize 消息队列内存池长度
	@warning 如果该节点为父的代理，那么将创建失败
	@return 消息通知函数
	*/
	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to_self(const shared_strand& strand, const int id, bool makeNew = false, size_t fixedSize = 16)
	{
		typedef MsgPool_<Args...> pool_type;

		assert_enter();
		auto msgPck = msg_pool_pck<Args...>(id, this);
		quit_guard qg(this);
		msgPck->lock(this);
		if (msgPck->_isHead)
		{
			std::shared_ptr<pool_type> msgPool;
			if (makeNew || !msgPck->_msgPool)
			{
				msgPool = pool_type::make(strand, fixedSize);
			}
			else
			{
				msgPool = msgPck->_msgPool;
			}
			actor_handle hostActor = update_msg_list<Args...>(msgPck, msgPool);
			if (hostActor)
			{
				msgPck->unlock(this);
				return post_actor_msg<Args...>(msgPool, hostActor);
			}
		}
		msgPck->unlock(this);
		return post_actor_msg<Args...>();
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to_self(const shared_strand& strand, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to_self<Args...>(strand, 0, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to_self(const int id, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to_self<Args...>(self_strand(), id, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to_self(bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to_self<Args...>(self_strand(), 0, makeNew, fixedSize);
	}

	/*!
	@brief 创建一个消息通知函数，在该Actor所依赖的ios无关线程中使用，且在该Actor调用 notify_run() 之前
	@param strand 消息调度器
	@param id 相同类型消息id
	@param fixedSize 消息队列内存池长度
	@return 消息通知函数
	*/
	template <typename... Args>
	post_actor_msg<Args...> connect_msg_notifer(const shared_strand& strand, const int id, size_t fixedSize = 16)
	{
		typedef post_actor_msg<Args...> post_type;

		return _strand->syncInvoke<post_type>([&]()->post_type
		{
			typedef MsgPool_<Args...> pool_type;
			if (!parent_actor() && !is_started() && !is_quited())
			{
				auto msgPck = msg_pool_pck<Args...>(id, this);
				msgPck->_msgPool = pool_type::make(strand, fixedSize);
				return post_type(msgPck->_msgPool, shared_from_this());
			}
			assert(false);
			return post_type();
		});
	}

	template <typename... Args>
	post_actor_msg<Args...> connect_msg_notifer(const shared_strand& strand, size_t fixedSize = 16)
	{
		return connect_msg_notifer<Args...>(strand, 0, fixedSize);
	}

	template <typename... Args>
	post_actor_msg<Args...> connect_msg_notifer(const int id, size_t fixedSize = 16)
	{
		return connect_msg_notifer<Args...>(self_strand(), id, fixedSize);
	}

	template <typename... Args>
	post_actor_msg<Args...> connect_msg_notifer(size_t fixedSize = 16)
	{
		return connect_msg_notifer<Args...>(self_strand(), 0, fixedSize);
	}
	//////////////////////////////////////////////////////////////////////////

	/*!
	@brief 连接消息泵到消息池
	@return 返回消息泵句柄
	*/
	template <typename... Args>
	__yield_interrupt msg_pump_handle<Args...> connect_msg_pump(const int id = 0)
	{
		return _connect_msg_pump<Args...>(id, this);
	}
private:
	template <typename... Args>
	static msg_pump_handle<Args...> _connect_msg_pump(const int id, my_actor* const host)
	{
		typedef MsgPump_<Args...> pump_type;
		typedef MsgPool_<Args...> pool_type;
		typedef typename pool_type::pump_handler pump_handler;

		host->assert_enter();
		auto msgPck = msg_pool_pck<Args...>(id, host);
		quit_guard qg(host);
		msgPck->lock(host);
		if (msgPck->_next)
		{
			msgPck->_next->lock(host);
			clear_msg_list<Args...>(host, msgPck->_next);
			msgPck->_next->unlock(host);
			msgPck->_next.reset();
		}
		if (!msgPck->_msgPump)
		{
			msgPck->_msgPump = pump_type::make(host->shared_from_this());
		}
		auto msgPump = msgPck->_msgPump;
		auto msgPool = msgPck->_msgPool;
		if (msgPool)
		{
			msgPump->connect(host->send<pump_handler>(msgPool->_strand, [&msgPck]()->pump_handler
			{
				return msgPck->_msgPool->connect_pump(msgPck->_msgPump);
			}));
		}
		else
		{
			msgPump->clear();
		}
		msgPck->unlock(host);
		msg_pump_handle<Args...> mh;
		mh._handle = msgPump.get();
		return mh;
	}
private:
	template <typename PUMP, typename DST>
	bool _timed_pump_msg(const PUMP& pump, DST& dstRec, int tm, bool checkDis)
	{
		assert(pump->_hostActor && pump->_hostActor->self_id() == self_id());
		if (!pump->read_msg(dstRec))
		{
			if (checkDis && pump->isDisconnected())
			{
				pump->stop_waiting();
				throw pump_disconnected_exception();
			}
			if (0 != tm)
			{
				pump->_checkDis = checkDis;
				bool timed = false;
				if (tm > 0)
				{
					delay_trig(tm, [this, &timed]
					{
						if (!_quited)
						{
							timed = true;
							pull_yield();
						}
					});
				}
				push_yield();
				if (!timed)
				{
					if (tm > 0)
					{
						cancel_delay_trig();
					}
					if (pump->_checkDis)
					{
						assert(checkDis);
						pump->_checkDis = false;
						throw pump_disconnected_exception();
					}
					return true;
				}
			}
			pump->_checkDis = false;
			pump->stop_waiting();
			return false;
		}
		return true;
	}

	template <typename PUMP, typename DST>
	bool _try_pump_msg(const PUMP& pump, DST& dstRec, bool checkDis)
	{
		assert(pump->_hostActor && pump->_hostActor->self_id() == self_id());
		if (!pump->try_read(dstRec))
		{
			if (checkDis && pump->isDisconnected())
			{
				throw pump_disconnected_exception();
			}
			return false;
		}
		return true;
	}

	template <typename PUMP, typename DST>
	void _pump_msg(const PUMP& pump, DST& dstRec, bool checkDis)
	{
		assert(pump->_hostActor && pump->_hostActor->self_id() == self_id());
		if (!pump->read_msg(dstRec))
		{
			if (checkDis && pump->isDisconnected())
			{
				pump->_waiting = false;
				pump->_dstRec = NULL;
				throw pump_disconnected_exception();
			}
			pump->_checkDis = checkDis;
			push_yield();
			if (pump->_checkDis)
			{
				assert(checkDis);
				pump->_checkDis = false;
				throw pump_disconnected_exception();
			}
		}
	}
public:

	/*!
	@brief 从消息泵中提取消息
	@param tm 超时时间
	@param checkDis 检测是否被断开连接，是就抛出 pump_disconnected_exception 异常
	@return 超时完成返回false，成功取到消息返回true
	*/
	template <typename... Args, typename... Outs>
	__yield_interrupt bool timed_pump_msg(int tm, bool checkDis, const msg_pump_handle<Args...>& pump, Outs&... res)
	{
		assert_enter();
		DstReceiverRef_<types_pck<Args...>, types_pck<Outs...>> dstRec(res...);
		return _timed_pump_msg(pump, dstRec, tm, checkDis);
	}

	template <typename... Args, typename... Outs>
	__yield_interrupt bool timed_pump_msg(int tm, const msg_pump_handle<Args...>& pump, Outs&... res)
	{
		return timed_pump_msg(tm, false, pump, res...);
	}

	__yield_interrupt bool timed_pump_msg(int tm, bool checkDis, const msg_pump_handle<>& pump);

	__yield_interrupt bool timed_pump_msg(int tm, const msg_pump_handle<>& pump);

	template <typename... Args, typename Handler>
	__yield_interrupt bool timed_pump_msg(int tm, bool checkDis, const msg_pump_handle<Args...>& pump, const Handler& h)
	{
		assert_enter();
		DstReceiverBuff_<Args...> dstRec;
		if (_timed_pump_msg(pump, dstRec, tm, checkDis))
		{
			tuple_invoke(h, std::move(dstRec._dstBuff.get()));
			return true;
		}
		return false;
	}

	template <typename... Args, typename Handler>
	__yield_interrupt bool timed_pump_msg(int tm, const msg_pump_handle<Args...>& pump, const Handler& h)
	{
		return timed_pump_msg(tm, false, pump, h);
	}

	/*!
	@brief 尝试从消息泵中提取消息
	*/
	template <typename... Args, typename... Outs>
	__yield_interrupt bool try_pump_msg(bool checkDis, const msg_pump_handle<Args...>& pump, Outs&... res)
	{
		assert_enter();
		DstReceiverRef_<types_pck<Args...>, types_pck<Outs...>> dstRec(res...);
		return _try_pump_msg(pump, dstRec, checkDis);
	}

	template <typename... Args, typename... Outs>
	__yield_interrupt bool try_pump_msg(const msg_pump_handle<Args...>& pump, Outs&... res)
	{
		return try_pump_msg(false, pump, res...);
	}

	__yield_interrupt bool try_pump_msg(bool checkDis, const msg_pump_handle<>& pump);

	__yield_interrupt bool try_pump_msg(const msg_pump_handle<>& pump);

	template <typename... Args, typename Handler>
	__yield_interrupt bool try_pump_msg(bool checkDis, const msg_pump_handle<Args...>& pump, const Handler& h)
	{
		assert_enter();
		DstReceiverBuff_<Args...> dstRec;
		if (_try_pump_msg(pump, dstRec, checkDis))
		{
			tuple_invoke(h, std::move(dstRec._dstBuff.get()));
			return true;
		}
		return false;
	}

	template <typename... Args, typename Handler>
	__yield_interrupt bool try_pump_msg(const msg_pump_handle<Args...>& pump, const Handler& h)
	{
		return try_pump_msg(false, pump, h);
	}

	/*!
	@brief 从消息泵中提取消息
	*/
	template <typename... Args, typename... Outs>
	__yield_interrupt void pump_msg(bool checkDis, const msg_pump_handle<Args...>& pump, Outs&... res)
	{
		timed_pump_msg(-1, checkDis, pump, res...);
	}

	template <typename... Args, typename... Outs>
	__yield_interrupt void pump_msg(const msg_pump_handle<Args...>& pump, Outs&... res)
	{
		pump_msg(false, pump, res...);
	}

	template <typename Arg>
	__yield_interrupt Arg pump_msg(bool checkDis, const msg_pump_handle<Arg>& pump)
	{
		assert_enter();
		DstReceiverBuff_<Arg> dstRec;
		_pump_msg(pump, dstRec, checkDis);
		return std::move(std::get<0>(dstRec._dstBuff.get()));
	}

	template <typename Arg>
	__yield_interrupt Arg pump_msg(const msg_pump_handle<Arg>& pump)
	{
		return pump_msg(false, pump);
	}

	__yield_interrupt void pump_msg(bool checkDis, const msg_pump_handle<>& pump);

	__yield_interrupt void pump_msg(const msg_pump_handle<>& pump);

	template <typename... Args, typename Handler>
	__yield_interrupt void pump_msg(bool checkDis, const msg_pump_handle<Args...>& pump, const Handler& h)
	{
		timed_pump_msg<Args...>(-1, checkDis, pump, h);
	}

	template <typename... Args, typename Handler>
	__yield_interrupt void pump_msg(const msg_pump_handle<Args...>& pump, const Handler& h)
	{
		pump_msg(false, pump, h);
	}

	/*!
	@brief 获取当前消息准确数
	*/
	template <typename... Args>
	__yield_interrupt size_t pump_length(const msg_pump_handle<Args...>& pump)
	{
		return pump._handle->size();
	}

	/*!
	@brief 获取当前消息大概数
	*/
	template <typename... Args>
	size_t pump_snap_length(const msg_pump_handle<Args...>& pump)
	{
		return pump._handle->snap_size();
	}
public:
	/*!
	@brief 查询当前消息由谁代理
	*/
	template <typename... Args>
	__yield_interrupt actor_handle msg_agent_handle(const int id, const actor_handle& buddyActor)
	{
		typedef std::shared_ptr<msg_pool_status::pck<Args...>> pck_type;

		quit_guard qg(this);
		auto msgPck = send<pck_type>(buddyActor->self_strand(), [id, &buddyActor]()->pck_type
		{
			if (!buddyActor->is_quited())
			{
				return my_actor::msg_pool_pck<Args...>(id, buddyActor.get(), false);
			}
			return pck_type();
		});
		if (msgPck)
		{
			std::shared_ptr<msg_pool_status::pck<Args...>> uStack[16];
			size_t stackl = 0;
			msgPck->lock(this);
			auto pckIt = msgPck;
			while (true)
			{
				if (pckIt->_next)
				{
					assert(stackl < 15);
					pckIt = pckIt->_next;
					uStack[stackl++] = pckIt;
					pckIt->lock(this);
				}
				else
				{
					actor_handle r;
					if (!pckIt->is_closed())
					{
						r = pckIt->_hostActor->shared_from_this();
					}
					while (stackl)
					{
						uStack[--stackl]->unlock(this);
					}
					msgPck->unlock(this);
					return r;
				}
			}
		}
		return actor_handle();
	}

	template <typename... Args>
	__yield_interrupt actor_handle msg_agent_handle(const int id, child_actor_handle& buddyActor)
	{
		return msg_agent_handle<Args...>(id, buddyActor.get_actor());
	}

	template <typename... Args>
	__yield_interrupt actor_handle msg_agent_handle(const actor_handle& buddyActor)
	{
		return msg_agent_handle<Args...>(0, buddyActor);
	}

	template <typename... Args>
	__yield_interrupt actor_handle msg_agent_handle(child_actor_handle& buddyActor)
	{
		return msg_agent_handle<Args...>(0, buddyActor.get_actor());
	}
private:
	template <size_t N>
	static bool _mutex_ready(MutexBlock_* const (&mbs)[N])
	{
		bool r = false;
		for (size_t i = 0; i < N; i++)
		{
			r |= mbs[i]->ready();
		}
		return r;
	}

	template <size_t N>
	static bool _mutex_ready2(const size_t st, MutexBlock_* const (&mbs)[N])
	{
		assert(st < N);
		size_t i = st;
		while (!mbs[i]->ready())
		{
			i = (i + 1) % N;
			if (st == i)
			{
				return false;
			}
		}
		return true;
	}

	template <size_t N>
	static void _mutex_cancel(MutexBlock_* const (&mbs)[N])
	{
		for (size_t i = 0; i < N; i++)
		{
			mbs[i]->cancel();
		}
	}

	template <size_t N>
	static bool _mutex_go(MutexBlock_* const (&mbs)[N])
	{
		for (size_t i = 0; i < N; i++)
		{
			if (mbs[i]->go())
			{
				return true;
			}
		}
		return false;
	}

	template <size_t N>
	static bool _cmp_snap_id(MutexBlock_* const (&mbs)[N])
	{
		for (size_t i = 0; i < N - 1; i++)
		{
			size_t t = mbs[i]->snap_id();
			for (size_t j = i + 1; j < N; j++)
			{
				if (mbs[j]->snap_id() == t)
				{
					return false;
				}
			}
		}
		return true;
	}
public:
	/*!
	@brief 运行互斥消息执行块（阻塞）
	*/
	template <typename... MutexBlocks>
	__yield_interrupt void run_mutex_blocks(MutexBlocks&... mbs)
	{
		static_assert(sizeof...(MutexBlocks) > 0, "");
		quit_guard qg(this);
		MutexBlock_* mbList[sizeof...(MutexBlocks)] = { &mbs... };
		assert(_cmp_snap_id(mbList));//判断有没有重复参数
		do
		{
			DEBUG_OPERATION(auto nt = yield_count());
			if (!_mutex_ready(mbList))
			{
				assert(yield_count() == nt);
				qg.unlock();
				push_yield();
				qg.lock();
				DEBUG_OPERATION(nt = yield_count());
			}
			_mutex_cancel(mbList);
			assert(yield_count() == nt);
		} while (!_mutex_go(mbList));
	}

	/*!
	@brief 运行互斥消息执行块（阻塞），每次只取一条消息
	*/
	template <typename... MutexBlocks>
	__yield_interrupt void run_mutex_blocks2(MutexBlocks&... mbs)
	{
		static_assert(sizeof...(MutexBlocks) > 0, "");
		quit_guard qg(this);
		size_t i = -1;
		MutexBlock_* mbList[sizeof...(MutexBlocks)] = { &mbs... };
		assert(_cmp_snap_id(mbList));//判断有没有重复参数
		do
		{
			DEBUG_OPERATION(auto nt = yield_count());
			i = sizeof...(MutexBlocks)-1 != i ? i + 1 : 0;
			if (!_mutex_ready2(i, mbList))
			{
				assert(yield_count() == nt);
				qg.unlock();
				push_yield();
				qg.lock();
				DEBUG_OPERATION(nt = yield_count());
			}
			_mutex_cancel(mbList);
			assert(yield_count() == nt);
		} while (!_mutex_go(mbList));
	}

	/*!
	@brief 运行互斥消息执行块（阻塞，带优先级），每次只取一条消息
	*/
	template <typename... MutexBlocks>
	__yield_interrupt void run_mutex_blocks3(MutexBlocks&... mbs)
	{
		static_assert(sizeof...(MutexBlocks) > 0, "");
		quit_guard qg(this);
		const size_t m = 2 * sizeof...(MutexBlocks) + 1;
		const size_t cmax = sizeof...(MutexBlocks) * (sizeof...(MutexBlocks) + 1) / 2;
		size_t ct = 0;
		MutexBlock_* mbList[sizeof...(MutexBlocks)] = { &mbs... };
		assert(_cmp_snap_id(mbList));//判断有没有重复参数
		do
		{
			DEBUG_OPERATION(auto nt = yield_count());
			ct = cmax != ct ? ct + 1 : 1;
			const size_t i = (m + 1 - (size_t)std::sqrt(m * m - 8 * ct)) / 2 - 1;
			if (!_mutex_ready2(i, mbList))
			{
				assert(yield_count() == nt);
				qg.unlock();
				push_yield();
				qg.lock();
				DEBUG_OPERATION(nt = yield_count());
			}
			_mutex_cancel(mbList);
			assert(yield_count() == nt);
		} while (!_mutex_go(mbList));
	}
public:
	/*!
	@brief 获取当前代码运行在哪个Actor下
	@return 不运行在Actor下或没启动 CHECK_SELF 返回NULL
	*/
	static my_actor* self_actor();

	/*!
	@brief 检测当前是否在该Actor内运行
	*/
	void check_self();

	/*!
	@brief 测试当前下的Actor栈是否安全
	*/
	void check_stack();

	/*!
	@brief 获取当前Actor剩余安全栈空间
	*/
	size_t stack_free_space();

	/*!
	@brief 获取当前Actor调度器
	*/
	const shared_strand& self_strand();

	/*!
	@brief 获取io_service调度器
	*/
	boost::asio::io_service& self_io_service();

	/*!
	@brief 返回本对象的智能指针
	*/
	actor_handle shared_from_this();

	/*!
	@brief 获取当前ActorID号
	*/
	id self_id();

	/*!
	@brief 获取Actor切换计数
	*/
	size_t yield_count();

	/*!
	@brief Actor切换计数清零
	*/
	void reset_yield();

	/*!
	@brief 开始运行建立好的Actor
	*/
	void notify_run();

	/*!
	@brief 强制退出该Actor，不可滥用，有可能会造成资源泄漏
	*/
	void notify_quit();

	/*!
	@brief 强制退出该Actor，完成后回调
	*/
	void notify_quit(const std::function<void ()>& h);

	/*!
	@brief Actor是否已经开始运行
	*/
	bool is_started();

	/*!
	@brief Actor是否已经退出
	*/
	bool is_quited();

	/*!
	@brief 是否是强制退出（在Actor确认退出后调用）
	*/
	bool is_force();

	/*!
	@brief 是否在Actor中
	*/
	bool in_actor();

	/*!
	@brief lock_quit后检测是否收到退出消息
	*/
	bool quit_msg();

	/*!
	@brief 锁定当前Actor，暂时不让强制退出
	*/
	void lock_quit();

	/*!
	@brief 解除退出锁定
	*/
	void unlock_quit();

	/*!
	@brief 是否锁定了退出
	*/
	bool is_locked_quit();

	/*!
	@brief 暂停Actor
	*/
	void notify_suspend();
	void notify_suspend(const std::function<void ()>& h);

	/*!
	@brief 恢复已暂停Actor
	*/
	void notify_resume();
	void notify_resume(const std::function<void ()>& h);

	/*!
	@brief 切换挂起/非挂起状态
	*/
	void switch_pause_play();
	void switch_pause_play(const std::function<void (bool)>& h);

	/*!
	@brief 等待Actor退出，在Actor所依赖的ios无关线程中使用
	*/
	void outside_wait_quit();

	/*!
	@brief 添加一个Actor结束回调
	*/
	void append_quit_callback(const std::function<void ()>& h);

	/*!
	@brief 启动一堆Actor
	*/
	void actors_start_run(const list<actor_handle>& anotherActors);

	/*!
	@brief 强制退出另一个Actor，并且等待完成
	*/
	__yield_interrupt void actor_force_quit(const actor_handle& anotherActor);
	__yield_interrupt void actors_force_quit(const list<actor_handle>& anotherActors);

	/*!
	@brief 等待另一个Actor结束后返回
	*/
	__yield_interrupt void actor_wait_quit(const actor_handle& anotherActor);
	__yield_interrupt void actors_wait_quit(const list<actor_handle>& anotherActors);
	__yield_interrupt bool timed_actor_wait_quit(int tm, const actor_handle& anotherActor);

	/*!
	@brief 挂起另一个Actor，等待其所有子Actor都调用后才返回
	*/
	__yield_interrupt void actor_suspend(const actor_handle& anotherActor);
	__yield_interrupt void actors_suspend(const list<actor_handle>& anotherActors);

	/*!
	@brief 恢复另一个Actor，等待其所有子Actor都调用后才返回
	*/
	__yield_interrupt void actor_resume(const actor_handle& anotherActor);
	__yield_interrupt void actors_resume(const list<actor_handle>& anotherActors);

	/*!
	@brief 对另一个Actor进行挂起/恢复状态切换
	@return 都已挂起返回true，否则false
	*/
	__yield_interrupt bool actor_switch(const actor_handle& anotherActor);
	__yield_interrupt bool actors_switch(const list<actor_handle>& anotherActors);

	void assert_enter();
private:
	template <typename H>
	void timeout(int ms, H&& h)
	{
		assert_enter();
		assert(ms > 0);
		assert(_timerState._timerCompleted);
		_timerState._timerTime = (long long)ms * 1000;
		_timerState._timerCb = TRY_MOVE(h);
		_timerState._timerStampBegin = get_tick_us();
		_timerState._timerCompleted = false;
		_timerState._timerHandle = _timer->timeout(_timerState._timerTime, shared_from_this());
	}

	void timeout_handler();
	void cancel_timer();
	void suspend_timer();
	void resume_timer();
	void force_quit(const std::function<void ()>& h);
	void suspend(const std::function<void ()>& h);
	void resume(const std::function<void ()>& h);
	void suspend();
	void resume();
	void run_one();
	void pull_yield_tls();
	void pull_yield();
	void pull_yield_after_quited();
	void push_yield();
	void push_yield_after_quited();
	void force_quit_cb_handler();
	void exit_callback();
	void child_suspend_cb_handler();
	void child_resume_cb_handler();
public:
#ifdef CHECK_ACTOR_STACK
	bool _checkStackFree;//是否检测堆栈过多
	std::shared_ptr<list<stack_line_info>> _createStack;//当前Actor创建时的调用堆栈
#endif
private:
	void* _actorPull;///<Actor中断点恢复
	void* _actorPush;///<Actor中断点
	void* _stackTop;///<Actor栈顶
	id _selfID;///<ActorID
	size_t _stackSize;///<Actor栈大小
	shared_strand _strand;///<Actor调度器
	bool _inActor : 1;///<当前正在Actor内部执行标记
	bool _started : 1;///<已经开始运行的标记
	bool _quited : 1;///<_mainFunc已经不再执行
	bool _exited : 1;///<完全退出
	bool _suspended : 1;///<Actor挂起标记
	bool _hasNotify : 1;///<当前Actor挂起，有外部触发准备进入Actor标记
	bool _isForce : 1;///<是否是强制退出的标记，成功调用了force_quit
	bool _notifyQuited : 1;///<当前Actor被锁定后，收到退出消息
	size_t _lockQuit;///<锁定当前Actor，如果当前接收到退出消息，暂时不退，等到解锁后退出
	size_t _yieldCount;//yield计数
	size_t _childOverCount;///<子Actor退出时计数
	size_t _childSuspendResumeCount;///<子Actor挂起/恢复计数
	main_func _mainFunc;///<Actor入口
	msg_list_shared_alloc<suspend_resume_option> _suspendResumeQueue;///<挂起/恢复操作队列
	msg_list_shared_alloc<std::function<void()> > _exitCallback;///<Actor结束后的回调函数，强制退出返回false，正常退出返回true
	msg_list_shared_alloc<std::function<void()> > _quitHandlerList;///<Actor退出时强制调用的函数，后注册的先执行
	msg_list_shared_alloc<actor_handle> _childActorList;///<子Actor集合，子Actor都退出后，父Actor才能退出
	msg_pool_status _msgPoolStatus;//消息池列表
	actor_handle _parentActor;///<父Actor，子Actor都析构后，父Actor才能析构
	timer_state _timerState;///<定时器状态
	ActorTimer_* _timer;///<定时器
	std::weak_ptr<my_actor> _weakThis;
#ifdef CHECK_SELF
#ifndef ENALBE_TLS_CHECK_SELF
	msg_map<void*, my_actor*>::iterator _btIt;
	msg_map<void*, my_actor*>::iterator _topIt;
#endif
#endif
	static msg_list_shared_alloc<my_actor::suspend_resume_option>::shared_node_alloc _suspendResumeQueueAll;
	static msg_list_shared_alloc<std::function<void()> >::shared_node_alloc _quitExitCallbackAll;
	static msg_list_shared_alloc<actor_handle>::shared_node_alloc _childActorListAll;
};

#endif