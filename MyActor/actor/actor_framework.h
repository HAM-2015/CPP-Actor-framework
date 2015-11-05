#ifndef __ACTOR_FRAMEWORK_H
#define __ACTOR_FRAMEWORK_H

#include <list>
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
#include "lambda_ref.h"

class my_actor;
typedef std::shared_ptr<my_actor> actor_handle;//Actor句柄

class mutex_trig_notifer;
class mutex_trig_handle;
class ActorMutex_;

using namespace std;

//此函数会上下文切换
#define __yield_interrupt

#if (_DEBUG || DEBUG)
/*!
@brief 用于检测在Actor内调用的函数是否触发了强制退出
*/
#define BEGIN_CHECK_FORCE_QUIT try {
#define END_CHECK_FORCE_QUIT } catch (my_actor::force_quit_exception&) {assert(false);}

/*!
@brief 用于锁定actor不强制退出
*/
#define LOCK_QUIT(__self__) __self__->lock_quit(); try {
#define UNLOCK_QUIT(__self__) } catch (...) { assert (false); } __self__->unlock_quit();

#else

#define BEGIN_CHECK_FORCE_QUIT
#define END_CHECK_FORCE_QUIT

#define LOCK_QUIT(__self__) __self__->lock_quit();
#define UNLOCK_QUIT(__self__) __self__->unlock_quit();

#endif

//检测 pump_msg 是否有 pump_disconnected_exception 异常抛出，因为在 catch 内部不能安全的进行coro切换
#define CATCH_PUMP_DISCONNECTED CATCH_FOR(my_actor::pump_disconnected_exception)


//////////////////////////////////////////////////////////////////////////

template <typename... ARGS>
class msg_pump_handle;
class CheckLost_;
class CheckPumpLost_;
class actor_msg_handle_base;
class MsgPoolBase_;

struct ActorFunc_
{
	static void lock_quit(my_actor* host);
	static void unlock_quit(my_actor* host);
	static actor_handle shared_from_this(my_actor* host);
	static const actor_handle& parent_actor(my_actor* host);
	static const shared_strand& self_strand(my_actor* host);
	static void pull_yield(my_actor* host);
	static void push_yield(my_actor* host);
	static bool is_quited(my_actor* host);
	static std::shared_ptr<bool> new_bool(bool b = false);
	template <typename R, typename H>
	static R send(my_actor* host, const shared_strand& exeStrand, H&& h);
	template <typename... Args>
	static msg_pump_handle<Args...> connect_msg_pump(const int id, my_actor* const host, bool checkLost);
	template <typename DST, typename... ARGS>
	static void _trig_handler(my_actor* host, DST& dstRec, ARGS&&... args);
	template <typename DST, typename... ARGS>
	static void _trig_handler2(my_actor* host, DST& dstRec, ARGS&&... args);
	template <typename DST, typename... ARGS>
	static void _dispatch_handler(my_actor* host, DST& dstRec, ARGS&&... args);
	template <typename DST, typename... ARGS>
	static void _dispatch_handler2(my_actor* host, DST& dstRec, ARGS&&... args);
#ifdef ENABLE_CHECK_LOST
	static std::shared_ptr<CheckLost_> new_check_lost(const shared_strand& strand, actor_msg_handle_base* handle);
	static std::shared_ptr<CheckPumpLost_> new_check_pump_lost(const actor_handle& hostActor, MsgPoolBase_* pool);
#endif
};
//////////////////////////////////////////////////////////////////////////

struct msg_lost_exception {};

#ifdef ENABLE_CHECK_LOST
class CheckLost_
{
	friend ActorFunc_;
	FRIEND_SHARED_PTR(CheckLost_);
private:
	CheckLost_(const shared_strand& strand, actor_msg_handle_base* handle);
	~CheckLost_();
private:
	shared_strand _strand;
	std::shared_ptr<bool> _closed;
	actor_msg_handle_base* _handle;
};

class CheckPumpLost_
{
	friend ActorFunc_;
	FRIEND_SHARED_PTR(CheckPumpLost_);
private:
	CheckPumpLost_(const actor_handle& hostActor, MsgPoolBase_* pool);
	~CheckPumpLost_();
private:
	actor_handle _hostActor;
	MsgPoolBase_* _pool;
};
#endif

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
	friend CheckLost_;
protected:
	actor_msg_handle_base();
	virtual ~actor_msg_handle_base(){}
public:
	virtual void close() = 0;
	virtual size_t size() = 0;
	void check_lost(bool b = true);
private:
	virtual void lost_msg();
protected:
	void set_actor(const actor_handle& hostActor);
	virtual void stop_waiting() = 0;
	virtual void throw_lost_exception() = 0;
protected:
	my_actor* _hostActor;
	std::shared_ptr<bool> _closed;
	DEBUG_OPERATION(shared_strand _strand);
	bool _waiting : 1;
	bool _losted : 1;
	bool _checkLost : 1;
};

template <typename... ARGS>
struct MsgNotiferBaseMsgCapture_;

template <typename... ARGS>
class ActorMsgHandlePush_ : public actor_msg_handle_base
{
	friend my_actor;
	friend MsgNotiferBase_<ARGS...>;
	friend MsgNotiferBaseMsgCapture_<ARGS...>;
	typedef DstReceiverBase_<TYPE_PIPE(ARGS)...> dst_receiver;
protected:
	virtual void push_msg(std::tuple<TYPE_PIPE(ARGS)...>&) = 0;
	virtual bool read_msg(dst_receiver& dst) = 0;
};

template <>
class ActorMsgHandlePush_<> : public actor_msg_handle_base
{
	friend my_actor;
	friend MsgNotiferBase_<>;
	friend MsgNotiferBaseMsgCapture_<>;
	typedef DstReceiverBase_<> dst_receiver;
protected:
	virtual void push_msg() = 0;
	virtual bool read_msg() = 0;
};


template <typename... ARGS>
struct MsgNotiferBaseMsgCapture_
{
	typedef ActorMsgHandlePush_<ARGS...> msg_handle;

	template <typename... Args>
	MsgNotiferBaseMsgCapture_(msg_handle* msgHandle, const actor_handle& host, const std::shared_ptr<bool>& closed, Args&&... args)
		:_msgHandle(msgHandle), _hostActor(host), _closed(closed), _args(TRY_MOVE(args)...) {}

	template <typename... Args>
	MsgNotiferBaseMsgCapture_(const MsgNotiferBaseMsgCapture_<Args...>& s)
		:_msgHandle(s._msgHandle), _hostActor(s._hostActor), _closed(s._closed), _args(std::move(s._args)) {}

	template <typename... Args>
	MsgNotiferBaseMsgCapture_(MsgNotiferBaseMsgCapture_<Args...>&& s)
		:_msgHandle(s._msgHandle), _hostActor(s._hostActor), _closed(s._closed), _args(std::move(s._args)) {}

	template <typename... Args>
	void operator =(const MsgNotiferBaseMsgCapture_<Args...>& s)
	{
		_msgHandle = s._msgHandle;
		_hostActor = s._hostActor;
		_closed = s._closed;
		_args = std::move(s._args);
	}

	template <typename... Args>
	void operator =(MsgNotiferBaseMsgCapture_<Args...>&& s)
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

	msg_handle* _msgHandle;
	actor_handle _hostActor;
	std::shared_ptr<bool> _closed;
	mutable std::tuple<TYPE_PIPE(ARGS)...> _args;
};

template <>
struct MsgNotiferBaseMsgCapture_<>
{
	typedef ActorMsgHandlePush_<> msg_handle;

	MsgNotiferBaseMsgCapture_(msg_handle* msgHandle, const actor_handle& host, const std::shared_ptr<bool>& closed)
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

template <typename... ARGS>
class MsgNotiferBase_
{
	typedef ActorMsgHandlePush_<ARGS...> msg_handle;
protected:
	MsgNotiferBase_()
		:_msgHandle(NULL){}

	MsgNotiferBase_(msg_handle* msgHandle, bool checkLost)
		:_msgHandle(msgHandle),
		_hostActor(ActorFunc_::shared_from_this(_msgHandle->_hostActor)),
		_closed(msgHandle->_closed)		
	{
		assert(msgHandle->_strand == ActorFunc_::self_strand(_hostActor.get()));
#ifdef ENABLE_CHECK_LOST
		if (checkLost)
		{
			_autoCheckLost = ActorFunc_::new_check_lost(ActorFunc_::self_strand(_hostActor.get()), msgHandle);
		}
#else
		assert(checkLost);
#endif
	}
public:
	template <typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		assert(!empty());
		if (!(*_closed))
		{
			ActorFunc_::self_strand(_hostActor.get())->try_tick(MsgNotiferBaseMsgCapture_<ARGS...>(_msgHandle, _hostActor, _closed, TRY_MOVE(args)...));
		}
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		assert(!empty());
		if (!(*_closed))
		{
			ActorFunc_::self_strand(_hostActor.get())->try_tick(MsgNotiferBaseMsgCapture_<>(_msgHandle, _hostActor, _closed));
		}
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
#ifdef ENABLE_CHECK_LOST
		_autoCheckLost.reset();
#endif
	}

	operator bool() const
	{
		return !empty();
	}
private:
	msg_handle* _msgHandle;
	actor_handle _hostActor;
	std::shared_ptr<bool> _closed;
#ifdef ENABLE_CHECK_LOST
	std::shared_ptr<CheckLost_> _autoCheckLost;
#endif
};

template <typename... ARGS>
class actor_msg_notifer : public MsgNotiferBase_<ARGS...>
{
	friend actor_msg_handle<ARGS...>;
public:
	actor_msg_notifer()	{}
private:
	actor_msg_notifer(ActorMsgHandlePush_<ARGS...>* msgHandle, bool checkLost)
		:MsgNotiferBase_<ARGS...>(msgHandle, checkLost) {}
};

template <typename... ARGS>
class actor_trig_notifer : public MsgNotiferBase_<ARGS...>
{
	friend actor_trig_handle<ARGS...>;
public:
	actor_trig_notifer() {}
private:
	actor_trig_notifer(ActorMsgHandlePush_<ARGS...>* msgHandle, bool checkLost)
		:MsgNotiferBase_<ARGS...>(msgHandle, checkLost) {}
};

template <typename... ARGS>
class actor_msg_handle : public ActorMsgHandlePush_<ARGS...>
{
	typedef ActorMsgHandlePush_<ARGS...> Parent;
	typedef std::tuple<TYPE_PIPE(ARGS)...> msg_type;
	typedef DstReceiverBase_<TYPE_PIPE(ARGS)...> dst_receiver;
	typedef actor_msg_notifer<ARGS...> msg_notifer;

	friend mutex_block_msg<ARGS...>;
	friend my_actor;
public:
	struct lost_exception : msg_lost_exception {};
public:
	actor_msg_handle(size_t fixedSize = 16)
		:_msgBuff(fixedSize), _dstRec(NULL) {}

	~actor_msg_handle()
	{
		close();
	}
private:
	msg_notifer make_notifer(const actor_handle& hostActor, bool checkLost)
	{
		close();
		Parent::set_actor(hostActor);
		Parent::_closed = ActorFunc_::new_bool();
		return msg_notifer(this, checkLost);
	}

	void push_msg(msg_type& msg)
	{
		assert(Parent::_strand->running_in_this_thread());
		if (!ActorFunc_::is_quited(Parent::_hostActor))
		{
			if (Parent::_waiting)
			{
				Parent::_waiting = false;
				assert(_msgBuff.empty());
				assert(_dstRec);
				_dstRec->move_from(msg);
				_dstRec = NULL;
				ActorFunc_::pull_yield(Parent::_hostActor);
				return;
			}
			_msgBuff.push_back(std::move(msg));
		}
	}

	bool read_msg(dst_receiver& dst)
	{
		assert(Parent::_strand->running_in_this_thread());
		assert(Parent::_closed);
		assert(!*Parent::_closed);
		if (!_msgBuff.empty())
		{
			dst.move_from(_msgBuff.front());
			_msgBuff.pop_front();
			return true;
		}
		_dstRec = &dst;
		Parent::_waiting = true;
		return false;
	}

	void stop_waiting()
	{
		Parent::_waiting = false;
		_dstRec = NULL;
	}

	void throw_lost_exception()
	{
		throw lost_exception();
	}

	void close()
	{
		if (Parent::_closed)
		{
			assert(Parent::_strand->running_in_this_thread());
			*Parent::_closed = true;
			Parent::_closed.reset();
		}
		_dstRec = NULL;
		Parent::_waiting = false;
		Parent::_losted = false;
		Parent::_checkLost = false;
		_msgBuff.clear();
		Parent::_hostActor = NULL;
	}

	size_t size()
	{
		assert(Parent::_strand->running_in_this_thread());
		return _msgBuff.size();
	}
private:
	dst_receiver* _dstRec;
	msg_queue<msg_type> _msgBuff;
};

template <>
class actor_msg_handle<> : public ActorMsgHandlePush_<>
{
	typedef ActorMsgHandlePush_<> Parent;
	typedef actor_msg_notifer<> msg_notifer;

	friend mutex_block_msg<>;
	friend my_actor;
public:
	struct lost_exception : msg_lost_exception {};
public:
	~actor_msg_handle()
	{
		close();
	}
private:
	msg_notifer make_notifer(const actor_handle& hostActor, bool checkLost)
	{
		close();
		Parent::set_actor(hostActor);
		Parent::_closed = ActorFunc_::new_bool();
		return msg_notifer(this, checkLost);
	}

	void push_msg()
	{
		assert(Parent::_strand->running_in_this_thread());
		if (!ActorFunc_::is_quited(Parent::_hostActor))
		{
			if (Parent::_waiting)
			{
				Parent::_waiting = false;
				if (_dstRec)
				{
					*_dstRec = true;
					_dstRec = NULL;
				}
				ActorFunc_::pull_yield(Parent::_hostActor);
				return;
			}
			_msgCount++;
		}
	}

	bool read_msg()
	{
		assert(Parent::_strand->running_in_this_thread());
		assert(Parent::_closed);
		assert(!*Parent::_closed);
		assert(!_dstRec);
		if (_msgCount)
		{
			_msgCount--;
			return true;
		}
		Parent::_waiting = true;
		return false;
	}

	bool read_msg(bool& dst)
	{
		assert(Parent::_strand->running_in_this_thread());
		assert(Parent::_closed);
		assert(!*Parent::_closed);
		if (_msgCount)
		{
			_msgCount--;
			dst = true;
			return true;
		}
		_dstRec = &dst;
		Parent::_waiting = true;
		return false;
	}

	void stop_waiting()
	{
		Parent::_waiting = false;
		_dstRec = NULL;
	}

	void throw_lost_exception()
	{
		throw lost_exception();
	}

	void close()
	{
		if (Parent::_closed)
		{
			assert(Parent::_strand->running_in_this_thread());
			*Parent::_closed = true;
			Parent::_closed.reset();
		}
		_dstRec = NULL;
		_msgCount = 0;
		Parent::_waiting = false;
		Parent::_losted = false;
		Parent::_checkLost = false;
		Parent::_hostActor = NULL;
	}

	size_t size()
	{
		assert(Parent::_strand->running_in_this_thread());
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
	typedef ActorMsgHandlePush_<ARGS...> Parent;
	typedef std::tuple<TYPE_PIPE(ARGS)...> msg_type;
	typedef DstReceiverBase_<TYPE_PIPE(ARGS)...> dst_receiver;
	typedef actor_trig_notifer<ARGS...> msg_notifer;

	friend mutex_block_trig<ARGS...>;
	friend my_actor;
public:
	struct lost_exception : msg_lost_exception {};
public:
	actor_trig_handle()
		:_hasMsg(false), _dstRec(NULL) {}

	~actor_trig_handle()
	{
		close();
	}
private:
	msg_notifer make_notifer(const actor_handle& hostActor, bool checkLost)
	{
		close();
		Parent::set_actor(hostActor);
		Parent::_closed = ActorFunc_::new_bool();
		return msg_notifer(this, checkLost);
	}

	void push_msg(msg_type& msg)
	{
		assert(Parent::_strand->running_in_this_thread());
		*Parent::_closed = true;
		if (!ActorFunc_::is_quited(Parent::_hostActor))
		{
			if (Parent::_waiting)
			{
				Parent::_waiting = false;
				assert(_dstRec);
				_dstRec->move_from(msg);
				_dstRec = NULL;
				ActorFunc_::pull_yield(Parent::_hostActor);
				return;
			}
			_hasMsg = true;
			new(_msgBuff)msg_type(std::move(msg));
		}
	}

	bool read_msg(dst_receiver& dst)
	{
		assert(Parent::_strand->running_in_this_thread());
		assert(Parent::_closed);
		if (_hasMsg)
		{
			_hasMsg = false;
			dst.move_from(*(msg_type*)_msgBuff);
			((msg_type*)_msgBuff)->~msg_type();
			return true;
		}
		assert(!*Parent::_closed);
		_dstRec = &dst;
		Parent::_waiting = true;
		return false;
	}

	void stop_waiting()
	{
		Parent::_waiting = false;
		_dstRec = NULL;
	}

	void throw_lost_exception()
	{
		throw lost_exception();
	}

	void close()
	{
		if (Parent::_closed)
		{
			assert(Parent::_strand->running_in_this_thread());
			*Parent::_closed = true;
			Parent::_closed.reset();
		}
		if (_hasMsg)
		{
			_hasMsg = false;
			((msg_type*)_msgBuff)->~msg_type();
		}
		_dstRec = NULL;
		Parent::_waiting = false;
		Parent::_losted = false;
		Parent::_checkLost = false;
		Parent::_hostActor = NULL;
	}
public:
	bool has()
	{
		assert(Parent::_strand->running_in_this_thread());
		return _hasMsg;
	}

	size_t size()
	{
		assert(Parent::_strand->running_in_this_thread());
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
	typedef ActorMsgHandlePush_<> Parent;
	typedef actor_trig_notifer<> msg_notifer;

	friend mutex_block_trig<>;
	friend my_actor;
public:
	struct lost_exception : msg_lost_exception {};
public:
	actor_trig_handle()
		:_hasMsg(false){}

	~actor_trig_handle()
	{
		close();
	}
private:
	msg_notifer make_notifer(const actor_handle& hostActor, bool checkLost)
	{
		close();
		Parent::set_actor(hostActor);
		Parent::_closed = ActorFunc_::new_bool();
		return msg_notifer(this, checkLost);
	}

	void push_msg()
	{
		assert(Parent::_strand->running_in_this_thread());
		*Parent::_closed = true;
		if (!ActorFunc_::is_quited(Parent::_hostActor))
		{
			if (Parent::_waiting)
			{
				Parent::_waiting = false;
				if (_dstRec)
				{
					*_dstRec = true;
					_dstRec = NULL;
				}
				ActorFunc_::pull_yield(Parent::_hostActor);
				return;
			}
			_hasMsg = true;
		}
	}

	bool read_msg()
	{
		assert(Parent::_strand->running_in_this_thread());
		assert(Parent::_closed);
		assert(!_dstRec);
		if (_hasMsg)
		{
			_hasMsg = false;
			return true;
		}
		assert(!*Parent::_closed);
		Parent::_waiting = true;
		return false;
	}

	bool read_msg(bool& dst)
	{
		assert(Parent::_strand->running_in_this_thread());
		assert(Parent::_closed);
		if (_hasMsg)
		{
			_hasMsg = false;
			dst = true;
			return true;
		}
		assert(!*Parent::_closed);
		_dstRec = &dst;
		Parent::_waiting = true;
		return false;
	}

	void stop_waiting()
	{
		Parent::_waiting = false;
		_dstRec = NULL;
	}

	void throw_lost_exception()
	{
		throw lost_exception();
	}

	void close()
	{
		if (Parent::_closed)
		{
			assert(Parent::_strand->running_in_this_thread());
			*Parent::_closed = true;
			Parent::_closed.reset();
		}
		_dstRec = NULL;
		_hasMsg = false;
		Parent::_waiting = false;
		Parent::_losted = false;
		Parent::_checkLost = false;
		Parent::_hostActor = NULL;
	}
public:
	bool has()
	{
		assert(Parent::_strand->running_in_this_thread());
		return _hasMsg;
	}

	size_t size()
	{
		assert(Parent::_strand->running_in_this_thread());
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
	my_actor* _hostActor;
};

template <typename... ARGS>
class MsgPump_ : public MsgPumpBase_
{
	typedef MsgPumpBase_ Parent;
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
	FRIEND_SHARED_PTR(MsgPump_<ARGS...>);
private:
	MsgPump_()
	{
		DEBUG_OPERATION(_pClosed = ActorFunc_::new_bool());
	}

	~MsgPump_()
	{
		assert(!_hasMsg);
		DEBUG_OPERATION(*_pClosed = true);
	}
private:
	static std::shared_ptr<MsgPump_<ARGS...>> make(const actor_handle& hostActor, bool checkLost)
	{
#ifndef ENABLE_CHECK_LOST
		assert(checkLost);
#endif
		std::shared_ptr<MsgPump_<ARGS...>> res(new MsgPump_<ARGS...>());
		res->_weakThis = res;
		res->_hasMsg = false;
		res->_waiting = false;
		res->_checkDis = false;
		res->_losted = false;
		res->_checkLost = checkLost;
		res->_pumpCount = 0;
		res->_dstRec = NULL;
		res->_hostActor = hostActor.get();
		res->_strand = ActorFunc_::self_strand(hostActor.get());
		return res;
	}

	void receiver(msg_type&& msg)
	{
		if (Parent::_hostActor)
		{
			assert(!_hasMsg);
			_losted = false;
			_pumpCount++;
			if (_dstRec)
			{
				_dstRec->move_from(msg);
				_dstRec = NULL;
				if (_waiting)
				{
					_waiting = false;
					_checkDis = false;
					ActorFunc_::pull_yield(_hostActor);
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

	void lost_msg(const actor_handle& hostActor)
	{
		if (_strand->running_in_this_thread())
		{
			_losted = true;
			if (_waiting)
			{
				_waiting = false;
				ActorFunc_::pull_yield(_hostActor);
			} 
		}
		else
		{
			auto shared_this = _weakThis.lock();
			_strand->post([shared_this, hostActor]()
			{
				shared_this->_losted = true;
				if (shared_this->_waiting)
				{
					shared_this->_waiting = false;
					ActorFunc_::pull_yield(shared_this->_hostActor);
				}
			});
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
						_waiting = true;
						ActorFunc_::push_yield(_hostActor);
						assert(_hasMsg);
						assert(!_dstRec);
						assert(!_waiting);
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
		_checkDis = false;
		_dstRec = NULL;
	}

	void connect(const pump_handler& pumpHandler, const bool& losted)
	{
		assert(_strand->running_in_this_thread());
		assert(_hostActor);
		_pumpHandler = pumpHandler;
		_pumpCount = 0;
		_losted = losted;
		if (_waiting)
		{
			if (_losted && _checkLost)
			{
				if (_waiting)
				{
					_waiting = false;
					ActorFunc_::pull_yield(_hostActor);
				}
			} 
			else
			{
				_pumpHandler.post_pump(_pumpCount);
			}
		}
	}

	void clear()
	{
		assert(_strand->running_in_this_thread());
		assert(_hostActor);
		_pumpHandler.clear();
		if (_hostActor && !ActorFunc_::is_quited(_hostActor) && _checkDis)
		{
			assert(_waiting);
			_waiting = false;
			_dstRec = NULL;
			ActorFunc_::pull_yield(_hostActor);
		}
	}

	void backflow(stack_obj<msg_type>& suck)
	{
		if (_hasMsg)
		{
			_hasMsg = false;
			suck.create(std::move(*(msg_type*)_msgSpace));
			((msg_type*)_msgSpace)->~msg_type();
		}
	}

	void close()
	{
		assert(_strand->running_in_this_thread());
		if (_hasMsg)
		{
			((msg_type*)_msgSpace)->~msg_type();
		}
		_hasMsg = false;
		_dstRec = NULL;
		_pumpCount = 0;
		_waiting = false;
		_checkDis = false;
		_losted = false;
		_checkLost = false;
		_pumpHandler.clear();
		Parent::_hostActor = NULL;
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
	bool _checkLost : 1;
	bool _losted : 1;
	DEBUG_OPERATION(std::shared_ptr<bool> _pClosed);
};

class MsgPoolBase_
{
	friend my_actor;
	friend CheckPumpLost_;
protected:
	virtual void lost_msg(const actor_handle& hostActor) = 0;
};

template <typename... ARGS>
class MsgPool_ : public MsgPoolBase_
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

		bool try_pump(my_actor* host, dst_receiver& dst, unsigned char pumpID, bool& wait)
		{
			assert(_thisPool);
			auto& refThis_ = *this;
			ActorFunc_::lock_quit(host);
			OUT_OF_SCOPE({ ActorFunc_::unlock_quit(host); });
			return ActorFunc_::send<bool>(host, _thisPool->_strand, [&dst, &wait, pumpID, refThis_]()->bool
			{
				bool ok = false;
				auto& thisPool_ = refThis_._thisPool;
				if (refThis_._msgPump == thisPool_->_msgPump)
				{
					auto& msgBuff = thisPool_->_msgBuff;
					if (pumpID == thisPool_->_sendCount)
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
						assert(!thisPool_->_waiting);
						assert(pumpID + 1 == thisPool_->_sendCount);
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
			ActorFunc_::lock_quit(host);
			OUT_OF_SCOPE({ ActorFunc_::unlock_quit(host); });
			return ActorFunc_::send<size_t>(host, _thisPool->_strand, [pumpID, refThis_]()->size_t
			{
				auto& thisPool_ = refThis_._thisPool;
				if (refThis_._msgPump == thisPool_->_msgPump)
				{
					auto& msgBuff = thisPool_->_msgBuff;
					if (pumpID == thisPool_->_sendCount)
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
	FRIEND_SHARED_PTR(MsgPool_<ARGS...>);
private:
	MsgPool_(size_t fixedSize)
		:_msgBuff(fixedSize)
	{

	}

	~MsgPool_()
	{

	}
private:
	static std::shared_ptr<MsgPool_<ARGS...>> make(const shared_strand& strand, size_t fixedSize)
	{
		std::shared_ptr<MsgPool_<ARGS...>> res(new MsgPool_<ARGS...>(fixedSize));
		res->_weakThis = res;
		res->_strand = strand;
		res->_waiting = false;
		res->_closed = false;
		res->_losted = false;
		res->_sendCount = 0;
		return res;
	}

	void send_msg(msg_type&& mt, const actor_handle& hostActor)
	{
		if (_closed) return;

		if (_waiting)
		{
			_waiting = false;
			_losted = false;
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
			_losted = false;
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

	void lost_msg(const actor_handle& hostActor)
	{
		if (!_closed)
		{
			auto shared_this = _weakThis.lock();
			_strand->try_tick([shared_this, hostActor]()
			{
				if (!shared_this->_closed && shared_this->_msgPump)
				{
					shared_this->_msgPump->lost_msg(hostActor);
				}
				else
				{
					shared_this->_losted = true;
				}
			});
		}
	}

	pump_handler connect_pump(const std::shared_ptr<msg_pump_type>& msgPump, bool& losted)
	{
		assert(msgPump);
		assert(_strand->running_in_this_thread());
		_msgPump = msgPump;
		pump_handler compHandler;
		compHandler._thisPool = _weakThis.lock();
		compHandler._msgPump = msgPump;
		_sendCount = 0;
		_waiting = false;
		losted = _losted;
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

	void backflow(stack_obj<msg_type>& suck)
	{
		assert(_strand->running_in_this_thread());
		assert(suck.has());
		_msgBuff.push_front(std::move(suck.get()));
	}
private:
	std::weak_ptr<MsgPool_> _weakThis;
	shared_strand _strand;
	std::shared_ptr<msg_pump_type> _msgPump;
	msg_queue<msg_type> _msgBuff;
	unsigned char _sendCount;
	bool _waiting : 1;
	bool _closed : 1;
	bool _losted : 1;
};

class MsgPumpVoid_;

class MsgPoolVoid_ :public MsgPoolBase_
{
	typedef post_actor_msg<> post_type;
	typedef void msg_type;
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
	void lost_msg(const actor_handle& hostActor);
	pump_handler connect_pump(const std::shared_ptr<msg_pump_type>& msgPump, bool& losted);
	void disconnect();
	void expand_fixed(size_t fixedSize){};
	void backflow(stack_obj<msg_type>& suck);
protected:
	std::weak_ptr<MsgPoolVoid_> _weakThis;
	shared_strand _strand;
	std::shared_ptr<msg_pump_type> _msgPump;
	size_t _msgBuff;
	unsigned char _sendCount;
	bool _waiting : 1;
	bool _closed : 1;
	bool _losted : 1;
};

class MsgPumpVoid_ : public MsgPumpBase_
{
	typedef MsgPoolVoid_ msg_pool_type;
	typedef void msg_type;
	typedef MsgPoolVoid_::pump_handler pump_handler;

	friend my_actor;
	friend MsgPoolVoid_;
	friend mutex_block_pump<>;
	friend pump_handler;
protected:
	MsgPumpVoid_(const actor_handle& hostActor, bool checkLost);
	virtual ~MsgPumpVoid_();
protected:
	void receiver();
	void lost_msg(const actor_handle& hostActor);
	void receive_msg_tick(const actor_handle& hostActor);
	void receive_msg(const actor_handle& hostActor);
	bool read_msg();
	bool read_msg(bool& dst);
	bool try_read();
	size_t size();
	size_t snap_size();
	void stop_waiting();
	void connect(const pump_handler& pumpHandler, const bool& losted);
	void clear();
	void close();
	void backflow(stack_obj<msg_type>& suck);
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
	bool _checkLost : 1;
	bool _losted : 1;
};

template <>
class MsgPool_<> : public MsgPoolVoid_
{
	friend my_actor;
	FRIEND_SHARED_PTR(MsgPool_<>);
private:
	typedef std::shared_ptr<MsgPool_> handle;

	MsgPool_(const shared_strand& strand)
		:MsgPoolVoid_(strand){}

	~MsgPool_(){}

	static handle make(const shared_strand& strand, size_t fixedSize)
	{
		handle res(new MsgPool_(strand));
		res->_weakThis = res;
		return res;
	}
};

template <>
class MsgPump_<> : public MsgPumpVoid_
{
	friend my_actor;
	friend mutex_block_pump<>;
	FRIEND_SHARED_PTR(MsgPump_<>);
public:
	typedef MsgPump_* handle;
private:
	MsgPump_(const actor_handle& hostActor, bool checkLost)
		:MsgPumpVoid_(hostActor, checkLost)
	{
#ifndef ENABLE_CHECK_LOST
		assert(checkLost);
#endif
		DEBUG_OPERATION(_pClosed = ActorFunc_::new_bool());
	}

	~MsgPump_()
	{
		DEBUG_OPERATION(*_pClosed = true);
	}

	static std::shared_ptr<MsgPump_> make(const actor_handle& hostActor, bool checkLost)
	{
		std::shared_ptr<MsgPump_> res(new MsgPump_(hostActor, checkLost));
		res->_weakThis = res;
		return res;
	}

	DEBUG_OPERATION(std::shared_ptr<bool> _pClosed);
};

template <typename... ARGS>
class msg_pump_handle
{
	friend my_actor;
	friend mutex_block_pump<ARGS...>;

	typedef MsgPump_<ARGS...> pump;

	pump* operator ->() const
	{
		assert(!*_pClosed);
		return _handle;
	}

#if (_DEBUG || DEBUG)
	bool check_closed() const
	{
		return !_pClosed || *_pClosed;
	}
#endif

	pump* _handle;
	DEBUG_OPERATION(std::shared_ptr<bool> _pClosed);
public:
	struct lost_exception : public msg_lost_exception {};
};

template <typename... ARGS>
class post_actor_msg
{
	typedef MsgPool_<ARGS...> msg_pool_type;
public:
	post_actor_msg(){}
	post_actor_msg(const std::shared_ptr<msg_pool_type>& msgPool, const actor_handle& hostActor, bool checkLost)
		:_msgPool(msgPool), _hostActor(hostActor)
	{
#ifdef ENABLE_CHECK_LOST
		if (checkLost)
		{
			_autoCheckLost = ActorFunc_::new_check_pump_lost(hostActor, msgPool.get());
		}
#else
		assert(checkLost);
#endif
	}
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
#ifdef ENABLE_CHECK_LOST
		_autoCheckLost.reset();//必须在_msgPool.reset()上面
#endif
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
#ifdef ENABLE_CHECK_LOST
	std::shared_ptr<CheckPumpLost_> _autoCheckLost;//必须在_msgPool下面
#endif
};
//////////////////////////////////////////////////////////////////////////

class MutexBlock_
{
	friend my_actor;
private:
	virtual bool ready() = 0;
	virtual void cancel() = 0;
	virtual bool go(bool& isRun) = 0;
	virtual size_t snap_id() = 0;
	virtual long long host_id() = 0;
	virtual void check_lost() = 0;

	MutexBlock_(const MutexBlock_&) {}
	void operator =(const MutexBlock_&) {}
protected:
	MutexBlock_() {}
	long long actor_id(my_actor* host);
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

	void check_lost()
	{
		if (_msgHandle._checkLost && _msgHandle._losted)
		{
			throw typename actor_msg_handle<ARGS...>::lost_exception();
		}
	}

	bool go(bool& isRun)
	{
		if (_msgBuff.has())
		{
			BEGIN_CHECK_EXCEPTION;
			isRun = true;
			bool r = tuple_invoke<bool>(_handler, std::move(_msgBuff._dstBuff.get()));
			_msgBuff.clear();
			return r;
			END_CHECK_EXCEPTION;
		}
		isRun = false;
		return false;
	}

	size_t snap_id()
	{
		return (size_t)&_msgHandle;
	}

	long long host_id()
	{
		return MutexBlock_::actor_id(_msgHandle._hostActor);
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

	void check_lost()
	{
		if (_msgHandle._checkLost && _msgHandle._losted)
		{
			throw typename actor_trig_handle<ARGS...>::lost_exception();
		}
	}

	bool go(bool& isRun)
	{
		if (_msgBuff.has())
		{
			BEGIN_CHECK_EXCEPTION;
			isRun = true;
			_triged = true;
			bool r = tuple_invoke<bool>(_handler, std::move(_msgBuff._dstBuff.get()));
			_msgBuff.clear();
			return r;
			END_CHECK_EXCEPTION;
		}
		isRun = false;
		return false;
	}

	size_t snap_id()
	{
		return (size_t)&_msgHandle;
	}

	long long host_id()
	{
		return MutexBlock_::actor_id(_msgHandle._hostActor);
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
	mutex_block_pump(my_actor* host, Handler&& handler, bool checkLost = false)
		:_handler(TRY_MOVE(handler))
	{
		_msgHandle = ActorFunc_::connect_msg_pump<ARGS...>(0, host, checkLost);
	}

	template <typename Handler>
	mutex_block_pump(const int id, my_actor* host, Handler&& handler, bool checkLost = false)
		: _handler(TRY_MOVE(handler))
	{
		_msgHandle = ActorFunc_::connect_msg_pump<ARGS...>(id, host, checkLost);
	}

	template <typename Handler>
	mutex_block_pump(const pump_handle& pump, Handler&& handler)
		: _msgHandle(pump), _handler(TRY_MOVE(handler)) {}
private:
	bool ready()
	{
		assert(!_msgBuff.has());
		assert(!_msgHandle.check_closed());
		return _msgHandle._handle->read_msg(_msgBuff);
	}

	void cancel()
	{
		assert(!_msgHandle.check_closed());
		_msgHandle._handle->stop_waiting();
	}

	void check_lost()
	{
		if (_msgHandle->_checkLost && _msgHandle->_losted)
		{
			throw typename msg_pump_handle<ARGS...>::lost_exception();
		}
	}

	bool go(bool& isRun)
	{
		assert(!_msgHandle.check_closed());
		if (_msgBuff.has())
		{
			BEGIN_CHECK_EXCEPTION;
			isRun = true;
			bool r = tuple_invoke<bool>(_handler, std::move(_msgBuff._dstBuff.get()));
			_msgBuff.clear();
			return r;
			END_CHECK_EXCEPTION;
		}
		isRun = false;
		return false;
	}

	size_t snap_id()
	{
		assert(!_msgHandle.check_closed());
		return (size_t)_msgHandle._handle;
	}

	long long host_id()
	{
		return MutexBlock_::actor_id(_msgHandle->_hostActor);
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

	void check_lost()
	{
		if (_msgHandle._checkLost && _msgHandle._losted)
		{
			throw actor_msg_handle<>::lost_exception();
		}
	}

	bool go(bool& isRun)
	{
		if (_has)
		{
			BEGIN_CHECK_EXCEPTION;
			isRun = true;
			_has = false;
			return _handler();
			END_CHECK_EXCEPTION;
		}
		isRun = false;
		return false;
	}

	size_t snap_id()
	{
		return (size_t)&_msgHandle;
	}

	long long host_id()
	{
		return MutexBlock_::actor_id(_msgHandle._hostActor);
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

	void check_lost()
	{
		if (_msgHandle._checkLost && _msgHandle._losted)
		{
			throw actor_trig_handle<>::lost_exception();
		}
	}

	bool go(bool& isRun)
	{
		if (_has)
		{
			BEGIN_CHECK_EXCEPTION;
			isRun = true;
			_triged = true;
			_has = false;
			return _handler();
			END_CHECK_EXCEPTION;
		}
		isRun = false;
		return false;
	}

	size_t snap_id()
	{
		return (size_t)&_msgHandle;
	}

	long long host_id()
	{
		return MutexBlock_::actor_id(_msgHandle._hostActor);
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
	mutex_block_pump(my_actor* host, Handler&& handler, bool checkLost = false)
		:_handler(TRY_MOVE(handler)), _has(false)
	{
		_msgHandle = ActorFunc_::connect_msg_pump(0, host, checkLost);
	}

	template <typename Handler>
	mutex_block_pump(const int id, my_actor* host, Handler&& handler, bool checkLost = false)
		: _handler(TRY_MOVE(handler)), _has(false)
	{
		_msgHandle = ActorFunc_::connect_msg_pump(id, host, checkLost);
	}

	template <typename Handler>
	mutex_block_pump(const pump_handle& pump, Handler&& handler)
		: _msgHandle(pump), _handler(TRY_MOVE(handler)), _has(false) {}
private:
	bool ready()
	{
		assert(!_has);
		assert(!_msgHandle.check_closed());
		return _msgHandle._handle->read_msg(_has);
	}

	void cancel()
	{
		assert(!_msgHandle.check_closed());
		_msgHandle._handle->stop_waiting();
	}

	void check_lost()
	{
		if (_msgHandle->_checkLost && _msgHandle->_losted)
		{
			throw msg_pump_handle<>::lost_exception();
		}
	}

	bool go(bool& isRun)
	{
		assert(!_msgHandle.check_closed());
		if (_has)
		{
			BEGIN_CHECK_EXCEPTION;
			isRun = true;
			_has = false;
			return _handler();
			END_CHECK_EXCEPTION;
		}
		isRun = false;
		return false;
	}

	size_t snap_id()
	{
		assert(!_msgHandle.check_closed());
		return (size_t)_msgHandle._handle;
	}

	long long host_id()
	{
		return MutexBlock_::actor_id(_msgHandle->_hostActor);
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
	{}

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
		ActorFunc_::_trig_handler(_hostActor.get(), dstRec, TRY_MOVE(args)...);
		reset();
	}

	template <typename DST, typename... Args>
	void _trig_handler2(DST& dstRec, Args&&... args) const
	{
		assert(!_pIsTrig->exchange(true));
		assert(_hostActor);
		ActorFunc_::_trig_handler2(_hostActor.get(), dstRec, TRY_MOVE(args)...);
		reset();
	}

	template <typename DST, typename... Args>
	void _dispatch_handler(DST& dstRec, Args&&... args) const
	{
		assert(!_pIsTrig->exchange(true));
		assert(_hostActor);
		ActorFunc_::_dispatch_handler(_hostActor.get(), dstRec, TRY_MOVE(args)...);
		reset();
	}

	template <typename DST, typename... Args>
	void _dispatch_handler2(DST& dstRec, Args&&... args) const
	{
		assert(!_pIsTrig->exchange(true));
		assert(_hostActor);
		ActorFunc_::_dispatch_handler2(_hostActor.get(), dstRec, TRY_MOVE(args)...);
		reset();
	}

	void tick_handler() const;
	void dispatch_handler() const;
	virtual void reset() const = 0;
private:
	void operator =(const TrigOnceBase_&);
protected:
	mutable actor_handle _hostActor;
	DEBUG_OPERATION(std::shared_ptr<boost::atomic<bool> > _pIsTrig);
};


template <typename... ARGS>
class trig_once_notifer : public TrigOnceBase_
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

template <typename... TYPES>
class sync_cb_handler {};

/*!
@brief 异步回调器，作为回调函数参数传入，回调后，自动返回到下一行语句继续执行
*/
template <typename... ARGS, typename... OUTS>
class callback_handler<types_pck<ARGS...>, types_pck<OUTS...>>: public TrigOnceBase_
{
	typedef TrigOnceBase_ Parent;
	typedef std::tuple<OUTS&...> dst_receiver;

	friend my_actor;
public:
	template <typename... Outs>
	callback_handler(my_actor* host, Outs&... outs)
		:_early(true), _dstRef(outs...)
	{
		Parent::_hostActor = ActorFunc_::shared_from_this(host);
	}

	~callback_handler()
	{
		if (_early)
		{
			ActorFunc_::push_yield(Parent::_hostActor.get());
			Parent::_hostActor.reset();
		}
	}

	callback_handler(const callback_handler& s)
		:TrigOnceBase_(s), _early(false), _dstRef(s._dstRef) {}
public:
	template <typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		Parent::_trig_handler2(_dstRef, try_ref_move<ARGS>::move(TRY_MOVE(args))...);
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		Parent::tick_handler();
	}
private:
	void reset() const
	{
		if (!_early)
		{
			Parent::_hostActor.reset();
		}
	}

	void operator =(const callback_handler& s)
	{
		assert(false);
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
	typedef TrigOnceBase_ Parent;
	typedef std::tuple<OUTS&...> dst_receiver;

	friend my_actor;
public:
	template <typename... Outs>
	asio_cb_handler(my_actor* host, Outs&... outs)
		:_early(true), _dstRef(outs...)
	{
		Parent::_hostActor = ActorFunc_::shared_from_this(host);
	}

	~asio_cb_handler()
	{
		if (_early)
		{
			ActorFunc_::push_yield(Parent::_hostActor.get());
			Parent::_hostActor.reset();
		}
	}

	asio_cb_handler(const asio_cb_handler& s)
		:TrigOnceBase_(s), _early(false), _dstRef(s._dstRef) {}
public:
	template <typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		Parent::_dispatch_handler2(_dstRef, try_ref_move<ARGS>::move(TRY_MOVE(args))...);
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		Parent::dispatch_handler();
	}
private:
	void reset() const
	{
		if (!_early)
		{
			Parent::_hostActor.reset();
		}
	}

	void operator =(const asio_cb_handler& s)
	{
		assert(false);
	}
private:
	dst_receiver _dstRef;
	const bool _early;
};
//////////////////////////////////////////////////////////////////////////

/*!
@brief 同步回调传入返回值
*/
template <typename R = void>
struct sync_result
{
	sync_result()
	:_res(NULL) {}

	template <typename... Args>
	void return_(Args&&... args)
	{
		assert(_res);
		_res->create(TRY_MOVE(args)...);
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
			_con.notify_one();
		}
		_res = NULL;
	}

	template <typename T>
	void operator =(T&& r)
	{
		assert(_res);
		_res->create(TRY_MOVE(r));
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
			_con.notify_one();
		}
		_res = NULL;
	}

	boost::mutex _mutex;
	boost::condition_variable _con;
	stack_obj<R>* _res;
};

template <>
struct sync_result<void>
{
	sync_result()
	DEBUG_OPERATION(:_res(false)){}

	void return_()
	{
		assert(_res);
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
			_con.notify_one();
		}
		DEBUG_OPERATION(_res = false);
	}

	boost::mutex _mutex;
	boost::condition_variable _con;
	DEBUG_OPERATION(bool _res);
};

/*!
@brief 同步回调器，作为回调函数参数传入，回调后，自动返回到下一行语句继续执行
*/
template <typename R, typename... ARGS, typename... OUTS>
class sync_cb_handler<R, types_pck<ARGS...>, types_pck<OUTS...>> : public TrigOnceBase_
{
	typedef TrigOnceBase_ Parent;
	typedef std::tuple<OUTS&...> dst_receiver;

	friend my_actor;
public:
	template <typename... Outs>
	sync_cb_handler(my_actor* host, sync_result<R>& res, Outs&... outs)
		:_early(true), _result(res), _dstRef(outs...)
	{
		Parent::_hostActor = ActorFunc_::shared_from_this(host);
	}

	~sync_cb_handler()
	{
		if (_early)
		{
			ActorFunc_::push_yield(Parent::_hostActor.get());
			Parent::_hostActor.reset();
		}
	}

	sync_cb_handler(const sync_cb_handler& s)
		:TrigOnceBase_(s), _early(false), _result(s._result), _dstRef(s._dstRef) {}
public:
	template <typename... Args>
	R operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		assert(!ActorFunc_::self_strand(Parent::_hostActor.get())->in_this_ios());
		stack_obj<R> res;
		_result._res = &res;
		{
			boost::unique_lock<boost::mutex> ul(_result._mutex);
			Parent::_dispatch_handler2(_dstRef, try_ref_move<ARGS>::move(TRY_MOVE(args))...);
			_result._con.wait(ul);
		}
		return (R&&)res.get();
	}

	R operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		assert(!ActorFunc_::self_strand(Parent::_hostActor.get())->in_this_ios());
		stack_obj<R> res;
		_result._res = &res;
		{
			boost::unique_lock<boost::mutex> ul(_result._mutex);
			Parent::dispatch_handler();
			_result._con.wait(ul);
		}
		return (R&&)res.get();
	}
private:
	void reset() const
	{
		if (!_early)
		{
			Parent::_hostActor.reset();
		}
	}

	void operator =(const sync_cb_handler& s)
	{
		assert(false);
	}
private:
	dst_receiver _dstRef;
	const bool _early;
	sync_result<R>& _result;
};

template <typename... ARGS, typename... OUTS>
class sync_cb_handler<void, types_pck<ARGS...>, types_pck<OUTS...>> : public TrigOnceBase_
{
	typedef TrigOnceBase_ Parent;
	typedef std::tuple<OUTS&...> dst_receiver;

	friend my_actor;
public:
	template <typename... Outs>
	sync_cb_handler(my_actor* host, sync_result<void>& res, Outs&... outs)
		:_early(true), _result(res), _dstRef(outs...)
	{
		Parent::_hostActor = ActorFunc_::shared_from_this(host);
	}

	~sync_cb_handler()
	{
		if (_early)
		{
			ActorFunc_::push_yield(Parent::_hostActor.get());
			Parent::_hostActor.reset();
		}
	}

	sync_cb_handler(const sync_cb_handler& s)
		:TrigOnceBase_(s), _early(false), _result(s._result), _dstRef(s._dstRef) {}
public:
	template <typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(ARGS) == sizeof...(Args), "");
		assert(!ActorFunc_::self_strand(Parent::_hostActor.get())->in_this_ios());
		DEBUG_OPERATION(_result._res = true);
		{
			boost::unique_lock<boost::mutex> ul(_result._mutex);
			Parent::_dispatch_handler2(_dstRef, try_ref_move<ARGS>::move(TRY_MOVE(args))...);
			_result._con.wait(ul);
		}
	}

	void operator()() const
	{
		static_assert(sizeof...(ARGS) == 0, "");
		assert(!ActorFunc_::self_strand(Parent::_hostActor.get())->in_this_ios());
		DEBUG_OPERATION(_result._res = true);
		{
			boost::unique_lock<boost::mutex> ul(_result._mutex);
			Parent::dispatch_handler();
			_result._con.wait(ul);
		}
	}
private:
	void reset() const
	{
		if (!_early)
		{
			Parent::_hostActor.reset();
		}
	}

	void operator =(const sync_cb_handler& s)
	{
		assert(false);
	}
private:
	dst_receiver _dstRef;
	const bool _early;
	sync_result<void>& _result;
};
//////////////////////////////////////////////////////////////////////////

template <typename R, typename Handler>
class wrapped_sync_handler
{
public:
	template <typename H>
	wrapped_sync_handler(H&& h, sync_result<R>& res)
		:_handler(TRY_MOVE(h)), _result(&res)
	{
		DEBUG_OPERATION(_pIsTrig = std::shared_ptr<boost::atomic<bool> >(new boost::atomic<bool>(false)));
	}

	wrapped_sync_handler(wrapped_sync_handler&& s)
		:_handler(std::move(s._handler)), _result(s._result)
	{
		DEBUG_OPERATION(_pIsTrig = s._pIsTrig);
	}

	void operator =(wrapped_sync_handler&& s)
	{
		_handler = std::move(s._handler);
		_result = s._result;
		DEBUG_OPERATION(_pIsTrig = s._pIsTrig);
	}

	template <typename... Args>
	R operator()(Args&&... args)
	{
		stack_obj<R> res;
		_result->_res = &res;
		assert(!_pIsTrig->exchange(true));
		{
			boost::unique_lock<boost::mutex> ul(_result->_mutex);
			_handler(TRY_MOVE(args)...);
			_result->_con.wait(ul);
		}
		DEBUG_OPERATION(_pIsTrig->exchange(false));
		return (R&&)res.get();
	}

	template <typename... Args>
	R operator()(Args&&... args) const
	{
		stack_obj<R> res;
		_result->_res = &res;
		assert(!_pIsTrig->exchange(true));
		{
			boost::unique_lock<boost::mutex> ul(_result->_mutex);
			_handler(TRY_MOVE(args)...);
			_result->_con.wait(ul);
		}
		DEBUG_OPERATION(_pIsTrig->exchange(false));
		return (R&&)res.get();
	}
private:
	Handler _handler;
	sync_result<R>* _result;
	DEBUG_OPERATION(std::shared_ptr<boost::atomic<bool> > _pIsTrig);
};

template <typename Handler>
class wrapped_sync_handler<void, Handler>
{
public:
	template <typename H>
	wrapped_sync_handler(H&& h, sync_result<void>& res)
		:_handler(TRY_MOVE(h)), _result(&res)
	{
		DEBUG_OPERATION(_pIsTrig = std::shared_ptr<boost::atomic<bool> >(new boost::atomic<bool>(false)));
	}

	wrapped_sync_handler(wrapped_sync_handler&& s)
		:_handler(std::move(s._handler)), _result(s._result)
	{
		DEBUG_OPERATION(_pIsTrig = s._pIsTrig);
	}

	void operator =(wrapped_sync_handler&& s)
	{
		_handler = std::move(s._handler);
		_result = s._result;
		DEBUG_OPERATION(_pIsTrig = s._pIsTrig);
	}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		DEBUG_OPERATION(_result->_res = true);
		assert(!_pIsTrig->exchange(true));
		{
			boost::unique_lock<boost::mutex> ul(_result->_mutex);
			_handler(TRY_MOVE(args)...);
			_result->_con.wait(ul);
		}
		DEBUG_OPERATION(_pIsTrig->exchange(false));
	}

	template <typename... Args>
	void operator()(Args&&... args) const
	{
		DEBUG_OPERATION(_result->_res = true);
		assert(!_pIsTrig->exchange(true));
		{
			boost::unique_lock<boost::mutex> ul(_result->_mutex);
			_handler(TRY_MOVE(args)...);
			_result->_con.wait(ul);
		}
		DEBUG_OPERATION(_pIsTrig->exchange(false));
	}
private:
	Handler _handler;
	sync_result<void>* _result;
	DEBUG_OPERATION(std::shared_ptr<boost::atomic<bool> > _pIsTrig);
};

/*!
@brief 包装一个handler到与当前ios无关的线程中同步调用
*/
template <typename R = void, typename Handler>
wrapped_sync_handler<R, Handler> wrap_sync(sync_result<R>& res, Handler&& h)
{
	return wrapped_sync_handler<R, Handler>(TRY_MOVE(h), res);
}
//////////////////////////////////////////////////////////////////////////

class child_actor_handle;

/*!
@brief 子Actor句柄参数，child_actor_handle内使用
*/
struct child_actor_param
{
	friend my_actor;
	friend child_actor_handle;
#if (_DEBUG || DEBUG)
	~child_actor_param();
private:
	child_actor_param();
	child_actor_param(const child_actor_param& s);
	child_actor_param& operator =(const child_actor_param& s);
	mutable bool _isCopy;
#endif
	mutable actor_handle _actor;///<本Actor
	mutable msg_list_shared_alloc<actor_handle>::iterator _actorIt;///<保存在父Actor集合中的节点
};

/*!
@brief 子Actor句柄，不可拷贝
*/
class child_actor_handle
{
public:
	typedef std::shared_ptr<child_actor_handle> ptr;
	friend my_actor;
public:
	child_actor_handle();
	child_actor_handle(const child_actor_handle&);
	child_actor_handle(const child_actor_param& s);
	~child_actor_handle();
	void operator =(const child_actor_param& s);
	const actor_handle& get_actor();
	my_actor* operator ->();
	static ptr make_ptr();
	bool empty();
	void operator =(const child_actor_handle&);
	void peel();
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
		std::function<void()> _h;
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
			virtual void clear() = 0;

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
		struct pck : public pck_base
		{
			typedef MsgPool_<ARGS...> pool_type;
			typedef MsgPump_<ARGS...> pump_type;

			pck(my_actor* hostActor)
				:pck_base(hostActor){}

			void close()
			{
				assert(!_next || !_next->_next);
				if (_msgPool)
				{
					if (_isHead)
					{
						_msgPool->_closed = true;
					}
					else
					{
						_hostActor->send_after_quited(_msgPool->_strand, [&]
						{
							assert(_msgPool->_msgPump == _msgPump);
							_msgPool->disconnect();
						});
					}
				}
				if (_msgPump)
				{
					if (!_isHead && _msgPump->_hasMsg)
					{
						typedef typename pump_type::msg_type msg_type;
						assert(_msgPool);
						stack_obj<msg_type> suck;
						_msgPump->backflow(suck);
						_hostActor->send_after_quited(_msgPool->_strand, [&]
						{
							_msgPool->backflow(suck);
						});
					}
					_msgPump->close();
				}
				_msgPump.reset();
				_msgPool.reset();
				_next.reset();
				_hostActor = NULL;
				_isHead = false;
			}

			void clear()
			{
				if (_isHead && _msgPool)
				{
					_msgPool->_closed = true;
				}
				if (_msgPump)
				{
					_msgPump->close();
				}
				_msgPump.reset();
				_msgPool.reset();
				_next.reset();
				_hostActor = NULL;
				_isHead = false;
			}

			std::shared_ptr<pool_type> _msgPool;
			std::shared_ptr<pump_type> _msgPump;
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

	template <typename Handler>
	struct wrap_delay_trig
	{
		template <typename H>
		wrap_delay_trig(const actor_handle& self, H&& h)
			:_count(self->_timerState._timerCount), _lockSelf(self), _h(TRY_MOVE(h)) {}

		wrap_delay_trig(wrap_delay_trig&& s)
			:_count(s._count), _lockSelf(s._lockSelf), _h(TRY_MOVE(s._h)) {}

		wrap_delay_trig(const wrap_delay_trig& s)
			:_count(s._count), _lockSelf(s._lockSelf), _h(TRY_MOVE(s._h)) {}

		void operator ()()
		{
			assert(_lockSelf->self_strand()->running_in_this_thread());
			if (!_lockSelf->_quited && _count == _lockSelf->_timerState._timerCount)
			{
				_h();
			}
		}

		const int _count;
		actor_handle _lockSelf;
		mutable Handler _h;
	};

	struct wrap_run_one
	{
		wrap_run_one(const actor_handle& self)
		:_lockSelf(self) {}

		void operator()()
		{
			_lockSelf->run_one();
		}

		actor_handle _lockSelf;
	};

	struct timer_state
	{
		int _timerCount;
		long long _timerTime;
		long long _timerStampBegin;
		long long _timerStampEnd;
		std::function<void()> _timerCb;
		ActorTimer_::timer_handle _timerHandle;
		bool _timerSuspend : 1;
		bool _timerCompleted : 1;
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
	friend ActorFunc_;
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

	template <typename... Args>
	struct pump_disconnected : pump_disconnected_exception {};

	/*!
	@brief Actor入口函数体
	*/
	typedef std::function<void(my_actor*)> main_func;

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
	child_actor_param create_child_actor(const shared_strand& actorStrand, const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);
	child_actor_param create_child_actor(const main_func& mainFunc, size_t stackSize = DEFAULT_STACKSIZE);

	/*!
	@brief 开始运行子Actor，只能调用一次
	*/
	void child_actor_run(child_actor_handle& actorHandle);

	template <typename... Handles>
	void child_actor_run(Handles&... handles)
	{
		static_assert(sizeof...(Handles) > 1, "");
		child_actor_handle* phandles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_run(*phandles[i]);
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
		child_actor_handle* phandles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_force_quit(*phandles[i]);
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
		child_actor_handle* phandles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_wait_quit(*phandles[i]);
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
		child_actor_handle* phandles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_suspend(*phandles[i]);
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
		child_actor_handle* phandles[sizeof...(Handles)] = { &handles... };
		for (int i = 0; i < sizeof...(Handles); i++)
		{
			child_actor_resume(*phandles[i]);
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
	@brief 尝试yield，如果当前yield计数和上次try_yield没变，就yield一次
	*/
	__yield_interrupt void try_yield();

	/*!
	@brief 锁定退出后，中断时间片
	*/
	__yield_interrupt void yield_guard();

	/*!
	@brief 尝试yield_guard，如果当前yield计数和上次try_yield没变，就yield_guard一次
	*/
	__yield_interrupt void try_yield_guard();

	/*!
	@brief 获取父Actor
	*/
	const actor_handle& parent_actor();

	/*!
	@brief 获取子Actor
	*/
	const msg_list_shared_alloc<actor_handle>& child_actors();
public:
	typedef msg_list_shared_alloc<std::function<void()> >::iterator quit_iterator;

	/*!
	@brief 注册一个资源释放函数，在强制退出Actor时调用
	*/
	quit_iterator regist_quit_handler(const std::function<void()>& quitHandler);

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
			assert(_timerState._timerCompleted);
			_timerState._timerTime = 0;
			_timerState._timerCompleted = false;
			_strand->post(wrap_delay_trig<RM_CREF(H)>(shared_from_this(), TRY_MOVE(h)));
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
				shared_this->_strand->post(wrap_run_one(shared_this));
			});
			push_yield();
			return;
		}
		h();
	}

	template <typename R, typename H>
	__yield_interrupt R send(const shared_strand& exeStrand, H&& h)
	{
		assert_enter();
		if (exeStrand != _strand)
		{
			std::tuple<stack_obj<TYPE_PIPE(R)>> dstRec;
			exeStrand->asyncInvoke(TRY_MOVE(h), async_invoke_handler<std::tuple<stack_obj<TYPE_PIPE(R)>>, R>(shared_from_this(), dstRec));
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
			shared_this->_strand->next_tick(wrap_run_one(shared_this));
		});
		push_yield();
	}

	template <typename R, typename H>
	__yield_interrupt R run_in_thread_stack(H&& h)
	{
		return async_send<R>(_strand, TRY_MOVE(h));
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
			shared_this->_strand->try_tick(wrap_run_one(shared_this));
		});
		push_yield();
	}

	template <typename R, typename H>
	__yield_interrupt R async_send(const shared_strand& exeStrand, H&& h)
	{
		assert_enter();
		std::tuple<stack_obj<TYPE_PIPE(R)>> dstRec;
		exeStrand->asyncInvoke(TRY_MOVE(h), async_invoke_handler<std::tuple<stack_obj<TYPE_PIPE(R)>>, R>(shared_from_this(), dstRec));
		push_yield();
		return std::move(std::get<0>(dstRec).get());
	}

	template <typename H>
	__yield_interrupt void async_send_self(H&& h)
	{
		async_send(_strand, TRY_MOVE(h));
	}

	template <typename R, typename H>
	__yield_interrupt R async_send_self(H&& h)
	{
		return async_send<R>(_strand, TRY_MOVE(h));
	}
private:
	template <typename H>
	__yield_interrupt void send_after_quited(const shared_strand& exeStrand, H&& h)
	{
		if (exeStrand != _strand)
		{
			DEBUG_OPERATION(bool setb = false);
			actor_handle shared_this = shared_from_this();
			exeStrand->asyncInvokeVoid(TRY_MOVE(h), [&, shared_this]
			{
				shared_this->_strand->post([&, shared_this]
				{
					assert(!shared_this->_inActor);
					DEBUG_OPERATION(setb = true);
					shared_this->pull_yield_after_quited();
				});
			});
			push_yield_after_quited();
			assert(setb);
			return;
		}
		h();
	}
public:
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
	__yield_interrupt void trig(DArgs&... dargs, const H& h)
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
	template <typename DST, typename... Args>
	bool _timed_wait_msg(ActorMsgHandlePush_<Args...>& amh, DST& dstRec, const int tm)
	{
		assert(amh._hostActor && amh._hostActor->self_id() == self_id());
		if (!amh.read_msg(dstRec))
		{
			OUT_OF_SCOPE(
			{
				amh.stop_waiting();
			});
			if (amh._checkLost && amh._losted)
			{
				amh.throw_lost_exception();
			}
			if (tm > 0)
			{
				bool timed = false;
				delay_trig(tm, [this, &timed]
				{
					timed = true;
					pull_yield();
				});
				push_yield();
				if (timed)
				{
					return false;
				}
				cancel_delay_trig();
			}
			else if (tm < 0)
			{
				push_yield();
			}
			else
			{
				return false;
			}
			if (amh._checkLost && amh._losted)
			{
				amh.throw_lost_exception();
			}
		}
		return true;
	}

	template <typename DST, typename... Args>
	void _wait_msg(ActorMsgHandlePush_<Args...>& amh, DST& dstRec)
	{
		_timed_wait_msg(amh, dstRec, -1);
	}
public:
	/*!
	@brief 创建一个消息通知函数
	*/
	template <typename... Args>
	actor_msg_notifer<Args...> make_msg_notifer_to_self(actor_msg_handle<Args...>& amh, bool checkLost = false)
	{
		return amh.make_notifer(shared_from_this(), checkLost);
	}

	/*!
	@brief 创建一个消息通知函数到buddyActor
	*/
	template <typename... Args>
	actor_msg_notifer<Args...> make_msg_notifer_to(const actor_handle& buddyActor, actor_msg_handle<Args...>& amh, bool checkLost = false)
	{
		assert_enter();
		return amh.make_notifer(buddyActor, checkLost);
	}

	template <typename... Args>
	actor_msg_notifer<Args...> make_msg_notifer_to(my_actor* buddyActor, actor_msg_handle<Args...>& amh, bool checkLost = false)
	{
		return make_msg_notifer_to<Args...>(buddyActor->shared_from_this(), amh, checkLost);
	}

	template <typename... Args>
	actor_msg_notifer<Args...> make_msg_notifer_to(child_actor_handle& childActor, actor_msg_handle<Args...>& amh, bool checkLost = false)
	{
		return make_msg_notifer_to<Args...>(childActor.get_actor(), amh, checkLost);
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

	template <typename R>
	__yield_interrupt R wait_msg(actor_msg_handle<R>& amh)
	{
		assert_enter();
		DstReceiverBuff_<R> dstRec;
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
	@brief 创建同步上下文回调函数，直接作为回调函数使用，async_func(..., Handler self->make_sync_context(sync_result, ...))
	*/
	template <typename R = void, typename... Args, typename... Outs>
	sync_cb_handler<R, types_pck<Outs...>, types_pck<Outs...>> make_sync_context_as_type(sync_result<R>& res, Outs&... outs)
	{
		static_assert(sizeof...(Args) == sizeof...(Outs), "");
		assert_enter();
		return sync_cb_handler<R, types_pck<Outs...>, types_pck<Outs...>>(this, res, outs...);
	}

	template <typename R = void, typename... Outs>
	sync_cb_handler<R, types_pck<Outs...>, types_pck<Outs...>> make_sync_context(sync_result<R>& res, Outs&... outs)
	{
		assert_enter();
		return sync_cb_handler<R, types_pck<Outs...>, types_pck<Outs...>>(this, res, outs...);
	}

	/*!
	@brief 创建一个消息触发函数，只有一次触发有效
	*/
	template <typename... Args>
	actor_trig_notifer<Args...> make_trig_notifer_to_self(actor_trig_handle<Args...>& ath, bool checkLost = false)
	{
		return ath.make_notifer(shared_from_this(), checkLost);
	}

	/*!
	@brief 创建一个消息触发函数到buddyActor，只有一次触发有效
	*/
	template <typename... Args>
	actor_trig_notifer<Args...> make_trig_notifer_to(const actor_handle& buddyActor, actor_trig_handle<Args...>& ath, bool checkLost = false)
	{
		assert_enter();
		return ath.make_notifer(buddyActor, checkLost);
	}

	template <typename... Args>
	actor_trig_notifer<Args...> make_trig_notifer_to(my_actor* buddyActor, actor_trig_handle<Args...>& ath, bool checkLost = false)
	{
		return make_trig_notifer_to<Args...>(buddyActor->shared_from_this(), ath, checkLost);
	}

	template <typename... Args>
	actor_trig_notifer<Args...> make_trig_notifer_to(child_actor_handle& childActor, actor_trig_handle<Args...>& ath, bool checkLost = false)
	{
		return make_trig_notifer_to<Args...>(childActor.get_actor(), ath, checkLost);
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

	template <typename R>
	__yield_interrupt R wait_trig(actor_trig_handle<R>& ath)
	{
		assert_enter();
		DstReceiverBuff_<R> dstRec;
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
	static std::shared_ptr<msg_pool_status::pck<Args...> > msg_pool_pck(const int id, my_actor* const host, const bool make = true)
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
			assert(std::dynamic_pointer_cast<pck_type>(res));
			return std::static_pointer_cast<pck_type>(res);
		}
		auto it = host->_msgPoolStatus._msgTypeMap.find(typeID);
		if (it != host->_msgPoolStatus._msgTypeMap.end())
		{
			assert(std::dynamic_pointer_cast<pck_type>(it->second));
			return std::static_pointer_cast<pck_type>(it->second);
		}
		return std::shared_ptr<pck_type>();
	}

	template <typename... Args>
	static void disconnect_pump(my_actor* const host, const std::shared_ptr<MsgPool_<Args...>>& msgPool, const std::shared_ptr<MsgPump_<Args...>>& msgPump)
	{
		typedef typename MsgPump_<Args...>::msg_type msg_type;

		if (msgPool)
		{
			host->send(msgPool->_strand, [&]
			{
				assert(!msgPool->_msgPump || msgPool->_msgPump == msgPump);
				msgPool->disconnect();
			});
		}
		if (msgPump)
		{
			stack_obj<msg_type> suck;
			host->send(msgPump->_strand, [&msgPump, &suck]
			{
				msgPump->backflow(suck);
				msgPump->clear();
			});
			if (suck.has())
			{
				assert(msgPool);
				host->send(msgPool->_strand, [&msgPool, &suck]
				{
					msgPool->backflow(suck);
				});
			}
		}
	}

	/*!
	@brief 清除消息代理链
	*/
	template <typename... Args>
	static void clear_msg_list(my_actor* const host, const std::shared_ptr<msg_pool_status::pck<Args...>>& msgPck)
	{
		std::shared_ptr<msg_pool_status::pck<Args...>> uStack[16];

		size_t stackl = 0;
		auto pckIt = msgPck;
		while (pckIt->_next)
		{
			assert(!pckIt->_next->_isHead);
			pckIt->_msgPool.reset();
			pckIt = pckIt->_next;
			pckIt->lock(host);
			assert(stackl < 15);
			uStack[stackl++] = pckIt;
		}
		if (!pckIt->is_closed())
		{
			auto& msgPool_ = pckIt->_msgPool;
			disconnect_pump<Args...>(host, msgPool_, pckIt->_msgPump);
			msgPool_.reset();
		}
		while (stackl)
		{
			uStack[--stackl]->unlock(host);
		}
	}

	/*!
	@brief 更新消息代理链
	*/
	template <typename... Args>
	void update_msg_list(const std::shared_ptr<msg_pool_status::pck<Args...>>& msgPck, const std::shared_ptr<MsgPool_<Args...>>& newPool)
	{
		typedef typename MsgPool_<Args...>::pump_handler pump_handler;
		typedef typename MsgPump_<Args...>::msg_type msg_type;

		std::shared_ptr<msg_pool_status::pck<Args...>> uStack[16];
		size_t stackl = 0;
		auto pckIt = msgPck;
		while (pckIt->_next)
		{
			assert(!pckIt->_next->_isHead);
			pckIt->_msgPool = newPool;
			pckIt = pckIt->_next;
			pckIt->lock(this);
			assert(stackl < 15);
			uStack[stackl++] = pckIt;
		}
		if (!pckIt->is_closed())
		{
			auto& msgPool_ = pckIt->_msgPool;
			auto& msgPump_ = pckIt->_msgPump;
			disconnect_pump<Args...>(this, msgPool_, msgPump_);
			msgPool_ = newPool;
			if (pckIt->_msgPump)
			{
				if (msgPool_)
				{
					bool losted = false;
					auto ph = send<pump_handler>(msgPool_->_strand, [&pckIt, &losted]()->pump_handler
					{
						return pckIt->_msgPool->connect_pump(pckIt->_msgPump, losted);
					});
					send(msgPump_->_strand, [&msgPump_, &ph, &losted]
					{
						msgPump_->connect(ph, losted);
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
		}
		while (stackl)
		{
			uStack[--stackl]->unlock(this);
		}
	}
private:
	/*!
	@brief 把本Actor内消息由伙伴Actor代理处理
	*/
	template <typename... Args>
	__yield_interrupt bool msg_agent_to(const int id, const actor_handle& childActor)
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
		pck_type msgPck = msg_pool_pck<Args...>(id, this);
		msgPck->lock(this);
		pck_type childPck;
		while (true)
		{
			childPck = send<pck_type>(childActor->self_strand(), [id, &childActor]()->pck_type
			{
				if (!childActor->is_quited())
				{
					return my_actor::msg_pool_pck<Args...>(id, childActor.get());
				}
				return pck_type();
			});
			if (!childPck || msgPck->_next == childPck)
			{
				msgPck->unlock(this);
				return false;
			}
			childPck->lock(this);
			if (!childPck->is_closed())
			{
				break;
			}
			childPck->unlock(this);
		}
		auto msgPool = msgPck->_msgPool;
		auto& next_ = msgPck->_next;
		if (next_)
		{
			next_->lock(this);
			assert(!next_->_isHead);
			clear_msg_list<Args...>(this, next_);
		}
		else
		{
			clear_msg_list<Args...>(this, msgPck);
			msgPck->_msgPool = msgPool;
		}
		update_msg_list<Args...>(childPck, msgPool);
		if (next_)
		{
			next_->_isHead = true;
			next_->unlock(this);
		}
		next_ = childPck;
		childPck->_isHead = false;
		childPck->unlock(this);
		msgPck->unlock(this);
		return true;
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
	__yield_interrupt child_actor_param msg_agent_to_actor(const int id, const shared_strand& strand, bool autoRun, const Handler& agentActor, size_t stackSize = DEFAULT_STACKSIZE)
	{
		child_actor_param childActor = create_child_actor(strand, [id, agentActor](my_actor* self)
		{
			msg_pump_handle<Args...> pump = my_actor::_connect_msg_pump<Args...>(id, self);
			agentActor(self, pump);
		}, stackSize);
		msg_agent_to<Args...>(id, childActor._actor);
		if (autoRun)
		{
			childActor._actor->notify_run();
		}
		return childActor;
	}

	template <typename... Args, typename Handler>
	__yield_interrupt child_actor_param msg_agent_to_actor(const int id, bool autoRun, const Handler& agentActor, size_t stackSize = DEFAULT_STACKSIZE)
	{
		return msg_agent_to_actor<Args...>(id, self_strand(), autoRun, agentActor, stackSize);
	}

	template <typename... Args, typename Handler>
	__yield_interrupt child_actor_param msg_agent_to_actor(const shared_strand& strand, bool autoRun, const Handler& agentActor, size_t stackSize = DEFAULT_STACKSIZE)
	{
		return msg_agent_to_actor<Args...>(0, strand, autoRun, agentActor, stackSize);
	}

	template <typename... Args, typename Handler>
	__yield_interrupt child_actor_param msg_agent_to_actor(bool autoRun, const Handler& agentActor, size_t stackSize = DEFAULT_STACKSIZE)
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
			auto& next_ = msgPck->_next;
			msgPck->lock(this);
			if (next_)
			{
				next_->lock(this);
				assert(!next_->_isHead);
				clear_msg_list<Args...>(this, next_);
				next_->_isHead = true;
				next_->unlock(this);
				next_.reset();
			}
			msgPck->unlock(this);
		}
	}

	/*!
	@brief 重置消息管道，之前该类型下connect_msg_notifer到本地的通知句柄将永久失效，本地必须重新connect_msg_pump，发送者必须重新connect_msg_notifer才能重新开始
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
			assert(std::dynamic_pointer_cast<pck_type>(it->second));
			std::shared_ptr<pck_type> msgPck = std::static_pointer_cast<pck_type>(it->second);
			msgPck->lock(this);
			auto msgPool = msgPck->_msgPool;
			clear_msg_list<Args...>(this, msgPck);
			msgPck->_msgPool = msgPool;
			msgPck->clear();
			_msgPoolStatus._msgTypeMap.erase(it);
			msgPck->unlock(this);
			return true;
		}
		return false;
	}
public:
	/*!
	@brief 连接消息通知到一个伙伴Actor，该Actor必须是子Actor或根Actor
	@param strand 消息调度器
	@param id 相同类型消息id
	@param makeNew false 如果该消息是个根消息就成功，否则失败；true 强制创建新的通知，之前的通知和代理将失效
	@param fixedSize 消息队列内存池长度
	@warning 如果 makeNew = false 且该节点为父的代理，将创建失败
	@return 消息通知函数
	*/
	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const shared_strand& strand, const int id, const actor_handle& buddyActor, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		typedef MsgPool_<Args...> pool_type;
		typedef typename pool_type::pump_handler pump_handler;
		typedef std::shared_ptr<msg_pool_status::pck<Args...>> pck_type;

		assert_enter();
#if (_DEBUG || DEBUG)
		{
			assert(!buddyActor->parent_actor() || buddyActor->parent_actor()->self_id() == self_id());
			auto selfParent = parent_actor();
			while (selfParent)
			{
				assert(selfParent->self_id() != buddyActor->self_id());
				selfParent = selfParent->parent_actor();
			}
		}
#endif
		quit_guard qg(this);
		pck_type msgPck = msg_pool_pck<Args...>(id, this);
		msgPck->lock(this);
		pck_type buddyPck;
		while (true)
		{
			buddyPck = send<pck_type>(buddyActor->self_strand(), [id, &buddyActor]()->pck_type
			{
				if (!buddyActor->is_quited())
				{
					return my_actor::msg_pool_pck<Args...>(id, buddyActor.get());
				}
				return pck_type();
			});
			if (!buddyPck)
			{
				msgPck->unlock(this);
				return post_actor_msg<Args...>();
			}
			buddyPck->lock(this);
			if (!buddyPck->is_closed())
			{
				break;
			}
			buddyPck->unlock(this);
		}
		if (makeNew)
		{
			if (buddyPck->_msgPool)
			{
				buddyPck->_msgPool->_closed = true;
			}
			auto newPool = pool_type::make(strand, fixedSize);
			update_msg_list<Args...>(buddyPck, newPool);
			buddyPck->_isHead = true;
			buddyPck->unlock(this);
			if (msgPck->_next == buddyPck)
			{//之前的代理将被取消
				msgPck->_next.reset();
				if (msgPck->_msgPump)
				{
					auto& msgPump_ = msgPck->_msgPump;
					auto& msgPool_ = msgPck->_msgPool;
					if (msgPool_)
					{
						bool losted = false;
						msgPump_->connect(this->send<pump_handler>(msgPool_->_strand, [&msgPck, &losted]()->pump_handler
						{
							return msgPck->_msgPool->connect_pump(msgPck->_msgPump, losted);
						}), losted);
					}
					else
					{
						msgPump_->clear();
					}
				}
			}
			msgPck->unlock(this);
			return post_actor_msg<Args...>(newPool, buddyActor, chekcLost);
		}
		if (buddyPck->_isHead)
		{
			auto& buddyPool = buddyPck->_msgPool;
			if (!buddyPool)
			{
				buddyPool = pool_type::make(strand, fixedSize);
				update_msg_list<Args...>(buddyPck, buddyPool);
			}
			buddyPck->unlock(this);
			msgPck->unlock(this);
			return post_actor_msg<Args...>(buddyPool, buddyActor, chekcLost);
		}
		buddyPck->unlock(this);
		msgPck->unlock(this);
		return post_actor_msg<Args...>();
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const shared_strand& strand, const int id, child_actor_handle& childActor, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(strand, id, childActor.get_actor(), chekcLost, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const shared_strand& strand, const actor_handle& buddyActor, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(strand, 0, buddyActor, chekcLost, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const shared_strand& strand, child_actor_handle& childActor, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(strand, 0, childActor.get_actor(), makeNew, chekcLost, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const int id, const actor_handle& buddyActor, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(buddyActor->self_strand(), id, buddyActor, chekcLost, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const int id, child_actor_handle& childActor, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(childActor->self_strand(), id, childActor.get_actor(), chekcLost, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(const actor_handle& buddyActor, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(buddyActor->self_strand(), 0, buddyActor, chekcLost, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to(child_actor_handle& childActor, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(childActor->self_strand(), 0, childActor.get_actor(), chekcLost, makeNew, fixedSize);
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
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to_self(const shared_strand& strand, const int id, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to<Args...>(strand, id, shared_from_this(), chekcLost, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to_self(const shared_strand& strand, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to_self<Args...>(strand, 0, chekcLost, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to_self(const int id, bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to_self<Args...>(self_strand(), id, chekcLost, makeNew, fixedSize);
	}

	template <typename... Args>
	__yield_interrupt post_actor_msg<Args...> connect_msg_notifer_to_self(bool chekcLost = false, bool makeNew = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer_to_self<Args...>(self_strand(), 0, chekcLost, makeNew, fixedSize);
	}

	/*!
	@brief 创建一个消息通知函数，在该Actor所依赖的ios无关线程中使用，且在该Actor调用 notify_run() 之前
	@param strand 消息调度器
	@param id 相同类型消息id
	@param fixedSize 消息队列内存池长度
	@return 消息通知函数
	*/
	template <typename... Args>
	post_actor_msg<Args...> connect_msg_notifer(const shared_strand& strand, const int id, bool chekcLost = false, size_t fixedSize = 16)
	{
		typedef post_actor_msg<Args...> post_type;

		return _strand->syncInvoke<post_type>([&]()->post_type
		{
			typedef MsgPool_<Args...> pool_type;
			if (!parent_actor() && !is_started() && !is_quited())
			{
				auto msgPck = msg_pool_pck<Args...>(id, this);
				msgPck->_msgPool = pool_type::make(strand, fixedSize);
				return post_type(msgPck->_msgPool, shared_from_this(), chekcLost);
			}
			assert(false);
			return post_type();
		});
	}

	template <typename... Args>
	post_actor_msg<Args...> connect_msg_notifer(const shared_strand& strand, bool chekcLost = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer<Args...>(strand, 0, chekcLost, fixedSize);
	}

	template <typename... Args>
	post_actor_msg<Args...> connect_msg_notifer(const int id, bool chekcLost = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer<Args...>(self_strand(), id, chekcLost, fixedSize);
	}

	template <typename... Args>
	post_actor_msg<Args...> connect_msg_notifer(bool chekcLost = false, size_t fixedSize = 16)
	{
		return connect_msg_notifer<Args...>(self_strand(), 0, chekcLost, fixedSize);
	}
	//////////////////////////////////////////////////////////////////////////

	/*!
	@brief 连接消息泵到消息池
	@return 返回消息泵句柄
	*/
	template <typename... Args>
	__yield_interrupt msg_pump_handle<Args...> connect_msg_pump(const int id = 0, bool checkLost = false)
	{
		return _connect_msg_pump<Args...>(id, this, checkLost);
	}

	template <typename... Args>
	__yield_interrupt msg_pump_handle<Args...> connect_msg_pump(bool checkLost)
	{
		return _connect_msg_pump<Args...>(0, this, checkLost);
	}
private:
	template <typename... Args>
	static msg_pump_handle<Args...> _connect_msg_pump(const int id, my_actor* const host, bool checkLost)
	{
		typedef MsgPump_<Args...> pump_type;
		typedef MsgPool_<Args...> pool_type;
		typedef typename pool_type::pump_handler pump_handler;

		host->assert_enter();
		quit_guard qg(host);
		auto msgPck = msg_pool_pck<Args...>(id, host);
		msgPck->lock(host);
		if (msgPck->_next)
		{
			msgPck->_next->lock(host);
			clear_msg_list<Args...>(host, msgPck->_next);
			msgPck->_next->unlock(host);
			msgPck->_next.reset();
		}
		auto& msgPump_ = msgPck->_msgPump;
		auto& msgPool_ = msgPck->_msgPool;
		if (!msgPump_)
		{
			msgPump_ = pump_type::make(host->shared_from_this(), checkLost);
		}
		else
		{
			msgPump_->_checkLost = checkLost;
		}
		if (msgPool_)
		{
			bool losted = false;
			msgPump_->connect(host->send<pump_handler>(msgPool_->_strand, [&msgPck, &losted]()->pump_handler
			{
				return msgPck->_msgPool->connect_pump(msgPck->_msgPump, losted);
			}), losted);
		}
		else
		{
			msgPump_->clear();
		}
		msgPck->unlock(host);
		msg_pump_handle<Args...> mh;
		mh._handle = msgPump_.get();
		DEBUG_OPERATION(mh._pClosed = msgPump_->_pClosed);
		return mh;
	}
private:
	template <typename DST, typename... Args>
	bool _timed_pump_msg(const msg_pump_handle<Args...>& pump, DST& dstRec, int tm, bool checkDis)
	{
		assert(!pump.check_closed());
		assert(pump->_hostActor && pump->_hostActor->self_id() == self_id());
		if (!pump->read_msg(dstRec))
		{
			OUT_OF_SCOPE(
			{
				pump->stop_waiting();
			});
			if (checkDis && pump->isDisconnected())
			{
				throw pump_disconnected<Args...>();
			}
			if (pump->_checkLost && pump->_losted)
			{
				throw typename msg_pump_handle<Args...>::lost_exception();
			}
			pump->_checkDis = checkDis;
			if (tm >= 0)
			{
				bool timed = false;
				delay_trig(tm, [this, &timed]
				{
					timed = true;
					pull_yield();
				});
				push_yield();
				if (timed)
				{
					return false;
				}
				cancel_delay_trig();
			}
			else
			{
				push_yield();
			}
			if (pump->_checkDis)
			{
				assert(checkDis);
				throw pump_disconnected<Args...>();
			}
			if (pump->_checkLost && pump->_losted)
			{
				throw typename msg_pump_handle<Args...>::lost_exception();
			}
		}
		return true;
	}

	template <typename DST, typename... Args>
	bool _try_pump_msg(const msg_pump_handle<Args...>& pump, DST& dstRec, bool checkDis)
	{
		assert(!pump.check_closed());
		assert(pump->_hostActor && pump->_hostActor->self_id() == self_id());
		OUT_OF_SCOPE(
		{
			pump->stop_waiting();
		});
		if (!pump->try_read(dstRec))
		{
			if (checkDis && pump->isDisconnected())
			{
				throw pump_disconnected<Args...>();
			}
			if (pump->_checkLost && pump->_losted)
			{
				throw typename msg_pump_handle<Args...>::lost_exception();
			}
			return false;
		}
		return true;
	}

	template <typename DST, typename... Args>
	void _pump_msg(const msg_pump_handle<Args...>& pump, DST& dstRec, bool checkDis)
	{
		_timed_pump_msg(pump, dstRec, -1, checkDis);
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

	template <typename R>
	__yield_interrupt R pump_msg(bool checkDis, const msg_pump_handle<R>& pump)
	{
		assert_enter();
		DstReceiverBuff_<R> dstRec;
		_pump_msg(pump, dstRec, checkDis);
		return std::move(std::get<0>(dstRec._dstBuff.get()));
	}

	template <typename R>
	__yield_interrupt R pump_msg(const msg_pump_handle<R>& pump)
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
		assert(!pump.check_closed());
		return pump._handle->size();
	}

	/*!
	@brief 获取当前消息大概数
	*/
	template <typename... Args>
	size_t pump_snap_length(const msg_pump_handle<Args...>& pump)
	{
		assert(!pump.check_closed());
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

	template <typename... Args>
	__yield_interrupt actor_handle msg_agent_handle(const int id = 0)
	{
		return msg_agent_handle<Args...>(id, shared_from_this());
	}
private:
	static bool _mutex_ready(MutexBlock_** const mbs, const size_t N)
	{
		bool r = false;
		for (size_t i = 0; i < N; i++)
		{
			r |= mbs[i]->ready();
		}
		return r;
	}

	static bool _mutex_ready2(const size_t st, MutexBlock_** const mbs, const size_t N)
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

	static void _mutex_cancel(MutexBlock_** const mbs, const size_t N)
	{
		for (size_t i = 0; i < N; i++)
		{
			mbs[i]->cancel();
		}
	}

	static bool _mutex_go(MutexBlock_** const mbs, const size_t N)
	{
		for (size_t i = 0; i < N; i++)
		{
			bool isRun = false;
			if (mbs[i]->go(isRun))
			{
				return true;
			}
		}
		return false;
	}

	static void _mutex_check_lost(MutexBlock_** const mbs, const size_t N)
	{
		for (size_t i = 0; i < N; i++)
		{
			mbs[i]->check_lost();
		}
	}

	static bool _mutex_go_count(size_t& runCount, MutexBlock_** const mbs, const size_t N)
	{
		for (size_t i = 0; i < N; i++)
		{
			bool isRun = false;
			if (mbs[i]->go(isRun))
			{
				assert(isRun);
				runCount++;
				return true;
			}
			if (isRun)
			{
				runCount++;
			}
		}
		return false;
	}

	static bool _cmp_snap_id(MutexBlock_** const mbs, const size_t N)
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

	static void _check_host_id(my_actor* self, MutexBlock_** const mbs, const size_t N)
	{
		for (size_t i = 0; i < N; i++)
		{
			assert(mbs[i]->host_id() == self->self_id());
		}
	}

	template <typename Ready>
	__yield_interrupt void _run_mutex_blocks(Ready&& mutexReady, MutexBlock_** const mbList, const size_t N)
	{
		quit_guard qg(this);
		DEBUG_OPERATION(_check_host_id(this, mbList, N));//判断句柄是不是都是自己的
		assert(_cmp_snap_id(mbList, N));//判断有没有重复参数
		OUT_OF_SCOPE(
		{
			_mutex_cancel(mbList, N);
		});
		do
		{
			DEBUG_OPERATION(auto nt = yield_count());
			if (!mutexReady())
			{
				assert(yield_count() == nt);
				qg.unlock();
				push_yield();
				_mutex_check_lost(mbList, N);
				qg.lock();
				DEBUG_OPERATION(nt = yield_count());
			}
			_mutex_cancel(mbList, N);
			assert(yield_count() == nt);
		} while (!_mutex_go(mbList, N));
	}

	template <typename Ready>
	__yield_interrupt size_t _timed_run_mutex_blocks(const int tm, Ready&& mutexReady, MutexBlock_** const mbList, const size_t N)
	{
		quit_guard qg(this);
		size_t runCount = 0;
		DEBUG_OPERATION(_check_host_id(this, mbList, N));//判断句柄是不是都是自己的
		assert(_cmp_snap_id(mbList, N));//判断有没有重复参数
		OUT_OF_SCOPE(
		{
			_mutex_cancel(mbList, N);
		});
		do
		{
			DEBUG_OPERATION(auto nt = yield_count());
			if (!mutexReady())
			{
				assert(yield_count() == nt);
				qg.unlock();
				if (tm >= 0)
				{
					bool timed = false;
					delay_trig(tm, [this, &timed]
					{
						timed = true;
						pull_yield();
					});
					push_yield();
					if (timed)
					{
						break;
					}
					cancel_delay_trig();
				}
				else
				{
					push_yield();
				}
				_mutex_check_lost(mbList, N);
				qg.lock();
				DEBUG_OPERATION(nt = yield_count());
			}
			_mutex_cancel(mbList, N);
			assert(yield_count() == nt);
		} while (!_mutex_go_count(runCount, mbList, N));
		return runCount;
	}
public:
	/*!
	@brief 运行互斥消息执行块（阻塞）
	*/
	template <typename... MutexBlocks>
	__yield_interrupt void run_mutex_blocks(MutexBlocks&&... mbs)
	{
		const size_t N = sizeof...(MutexBlocks);
		static_assert(N > 0, "");
		MutexBlock_* mbList[N] = { &mbs... };
		_run_mutex_blocks([&]()->bool
		{
			return _mutex_ready(mbList, N);
		}, mbList, N);
	}

	/*!
	@brief 运行互斥消息执行块（阻塞），每次从头开始优先只取一条消息
	*/
	template <typename... MutexBlocks>
	__yield_interrupt void run_mutex_blocks1(MutexBlocks&&... mbs)
	{
		const size_t N = sizeof...(MutexBlocks);
		static_assert(N > 0, "");
		MutexBlock_* mbList[N] = { &mbs... };
		_run_mutex_blocks([&]()->bool
		{
			return _mutex_ready2(0, mbList, N);
		}, mbList, N);
	}

	/*!
	@brief 运行互斥消息执行块（阻塞），每次依次往后开始优先只取一条消息
	*/
	template <typename... MutexBlocks>
	__yield_interrupt void run_mutex_blocks2(MutexBlocks&&... mbs)
	{
		const size_t N = sizeof...(MutexBlocks);
		static_assert(N > 0, "");
		MutexBlock_* mbList[N] = { &mbs... };
		size_t i = -1;
		_run_mutex_blocks([&]()->bool
		{
			i = N - 1 != i ? i + 1 : 0;
			return _mutex_ready2(i, mbList, N);
		}, mbList, N);
	}

	/*!
	@brief 运行互斥消息执行块（阻塞，带优先级），每次只取一条消息
	*/
	template <typename... MutexBlocks>
	__yield_interrupt void run_mutex_blocks3(MutexBlocks&&... mbs)
	{
		const size_t N = sizeof...(MutexBlocks);
		static_assert(N > 0, "");
		MutexBlock_* mbList[N] = { &mbs... };
		const size_t m = 2 * N + 1;
		const size_t cmax = N* (N + 1) / 2;
		size_t ct = 0;
		_run_mutex_blocks([&]()->bool
		{
			ct = cmax != ct ? ct + 1 : 1;
			const size_t i = (m + 1 - (size_t)std::sqrt(m * m - 8 * ct)) / 2 - 1;
			return _mutex_ready2(i, mbList, N);
		}, mbList, N);
	}

	/*!
	@brief 超时等待运行互斥消息执行块（阻塞）
	*/
	template <typename... MutexBlocks>
	__yield_interrupt size_t timed_run_mutex_blocks(const int tm, MutexBlocks&&... mbs)
	{
		const size_t N = sizeof...(MutexBlocks);
		static_assert(N > 0, "");
		MutexBlock_* mbList[N] = { &mbs... };
		return _timed_run_mutex_blocks(tm, [&]()->bool
		{
			return _mutex_ready(mbList, N);
		}, mbList, N);
	}

	/*!
	@brief 超时等待运行互斥消息执行块（阻塞），每次从头开始优先只取一条消息
	*/
	template <typename... MutexBlocks>
	__yield_interrupt size_t timed_run_mutex_blocks1(const int tm, MutexBlocks&&... mbs)
	{
		const size_t N = sizeof...(MutexBlocks);
		static_assert(N > 0, "");
		MutexBlock_* mbList[N] = { &mbs... };
		return _timed_run_mutex_blocks(tm, [&]()->bool
		{
			return _mutex_ready2(0, mbList, N);
		}, mbList, N);
	}

	/*!
	@brief 超时等待运行互斥消息执行块（阻塞），每次依次往后开始优先只取一条消息
	*/
	template <typename... MutexBlocks>
	__yield_interrupt size_t timed_run_mutex_blocks2(const int tm, MutexBlocks&&... mbs)
	{
		const size_t N = sizeof...(MutexBlocks);
		static_assert(N > 0, "");
		MutexBlock_* mbList[N] = { &mbs... };
		size_t i = -1;
		return _timed_run_mutex_blocks(tm, [&]()->bool
		{
			i = N - 1 != i ? i + 1 : 0;
			return _mutex_ready2(i, mbList, N);
		}, mbList, N);
	}

	/*!
	@brief 超时等待运行互斥消息执行块（阻塞，带优先级），每次只取一条消息
	*/
	template <typename... MutexBlocks>
	__yield_interrupt size_t timed_run_mutex_blocks3(const int tm, MutexBlocks&&... mbs)
	{
		const size_t N = sizeof...(MutexBlocks);
		static_assert(N > 0, "");
		MutexBlock_* mbList[N] = { &mbs... };
		const size_t m = 2 * N + 1;
		const size_t cmax = N* (N + 1) / 2;
		size_t ct = 0;
		return _timed_run_mutex_blocks(tm, [&]()->bool
		{
			ct = cmax != ct ? ct + 1 : 1;
			const size_t i = (m + 1 - (size_t)std::sqrt(m * m - 8 * ct)) / 2 - 1;
			return _mutex_ready2(i, mbList, N);
		}, mbList, N);
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
	void notify_quit(const std::function<void()>& h);

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
	void notify_suspend(const std::function<void()>& h);

	/*!
	@brief 恢复已暂停Actor
	*/
	void notify_resume();
	void notify_resume(const std::function<void()>& h);

	/*!
	@brief 切换挂起/非挂起状态
	*/
	void switch_pause_play();
	void switch_pause_play(const std::function<void(bool)>& h);

	/*!
	@brief 等待Actor退出，在Actor所依赖的ios无关线程中使用
	*/
	void outside_wait_quit();

	/*!
	@brief 添加一个Actor结束回调
	*/
	void append_quit_callback(const std::function<void()>& h);

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
	void force_quit(const std::function<void()>& h);
	void suspend(const std::function<void()>& h);
	void resume(const std::function<void()>& h);
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
	size_t _lastYield;//记录上次try_yield的计数
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

template <typename R, typename H>
R ActorFunc_::send(my_actor* host, const shared_strand& exeStrand, H&& h)
{
	assert(host);
	return host->send<R>(exeStrand, TRY_MOVE(h));
}

template <typename... Args>
msg_pump_handle<Args...> ActorFunc_::connect_msg_pump(const int id, my_actor* const host, bool checkLost)
{
	assert(host);
	return my_actor::_connect_msg_pump<Args...>(id, host, checkLost);
}

template <typename DST, typename... ARGS>
void ActorFunc_::_trig_handler(my_actor* host, DST& dstRec, ARGS&&... args)
{
	assert(host);
	host->_trig_handler(dstRec, TRY_MOVE(args)...);
}

template <typename DST, typename... ARGS>
void ActorFunc_::_trig_handler2(my_actor* host, DST& dstRec, ARGS&&... args)
{
	assert(host);
	host->_trig_handler2(dstRec, TRY_MOVE(args)...);
}

template <typename DST, typename... ARGS>
void ActorFunc_::_dispatch_handler(my_actor* host, DST& dstRec, ARGS&&... args)
{
	assert(host);
	host->_dispatch_handler(dstRec, TRY_MOVE(args)...);
}

template <typename DST, typename... ARGS>
void ActorFunc_::_dispatch_handler2(my_actor* host, DST& dstRec, ARGS&&... args)
{
	assert(host);
	host->_dispatch_handler2(dstRec, TRY_MOVE(args)...);
}

#endif