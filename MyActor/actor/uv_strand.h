#ifndef __UV_STRAND_H
#define __UV_STRAND_H

#include "shared_strand.h"

#ifdef ENABLE_UV_ACTOR
#include "uv.h"

class bind_uv_run;
class uv_strand;
typedef std::shared_ptr<uv_strand> shared_uv_strand;

#define BEGIN_CLOSE_UV(__uv_strand__) \
if (__uv_strand__ && !(__uv_strand__)->released() && !(__uv_strand__)->is_wait_close()) { \
uv_strand* const ___uv_strand = (__uv_strand__).get(); {

#define WAIT_CLOSE_UV };___uv_strand->async_check_close([&]{

#define END_CLOSE_UV });}

class uv_strand : public boost_strand
{
	friend boost_strand;
	FRIEND_SHARED_PTR(uv_strand);

	struct uv_tls
	{
		uv_tls();
		~uv_tls();

		static uv_tls* push_stack(uv_strand*);
		static uv_strand* pop_stack();
		static uv_strand* pop_stack(uv_tls*);
		static bool running_in_this_thread(uv_strand*);
		static size_t running_depth();
		static void init();
		static void reset();

		msg_list<uv_strand*, pool_alloc<mem_alloc2<> > > _uvStack;
		void* _tlsBuff[64];
		int _count;
	};

	struct wrap_handler_face : public op_queue::face
	{
		virtual void invoke() = 0;
	};

	template <typename Handler>
	struct wrap_handler : public wrap_handler_face
	{
		template <typename H>
		wrap_handler(H&& h)
			:_handler(std::forward<H>(h)) {}

		void invoke()
		{
			CHECK_EXCEPTION(_handler);
			this->~wrap_handler();
		}

		Handler _handler;
	};

	template <typename Handler>
	wrap_handler_face* make_wrap_handler(reusable_mem_mt<>& reuMem, Handler&& handler)
	{
		typedef wrap_handler<RM_CREF(Handler)> handler_type;
		return new(reuMem.allocate(sizeof(handler_type)))handler_type(std::forward<Handler>(handler));
	}
private:
	uv_strand();
	~uv_strand();
public:
	static shared_uv_strand create(io_engine& ioEngine, uv_loop_t* uvLoop = uv_default_loop());
	void release();
	bool released();
	shared_strand clone();
	bool in_this_ios();
	bool running_in_this_thread();
	bool only_self();
	bool sync_safe();
	bool is_running();
	void enter_wait_close();
	bool is_wait_close();
	uv_loop_t* uv_loop();
private:
	template <typename Handler>
	void _dispatch_uv(Handler&& handler)
	{
		_post_uv(std::forward<Handler>(handler));
	}

	template <typename Handler>
	void _post_uv(Handler&& handler)
	{
		append_task(make_wrap_handler(_reuMem, std::forward<Handler>(handler)));
	}

	void post_task_event();
	void append_task(wrap_handler_face*);
	void run_one_task();
	void check_close();
public:
	template <typename Handler>
	std::function<void()> wrap_check_close(Handler&& handler)
	{
		assert(in_this_ios());
		_waitCount++;
		return FUNCTION_ALLOCATOR(std::function<void()>, wrap(std::bind([this](Handler& handler)
		{
			CHECK_EXCEPTION(handler);
			check_close();
		}, std::forward<Handler>(handler))), (reusable_alloc<void, reusable_mem_mt<>>(_reuMem)));
	}

	std::function<void()> wrap_check_close();

	template <typename Handler>
	void async_check_close(Handler&& handler)
	{
		assert(in_this_ios());
		if (_waitCount)
		{
			_waitClose = true;
			setImmediate(std::bind([this](Handler& handler)
			{
				async_check_close(std::forward<Handler>(handler));
			}, std::forward<Handler>(handler)));
		}
		else
		{
			enter_wait_close();
			CHECK_EXCEPTION(handler);
		}
	}
public:
	template <typename Handler>
	bool noTopCall(Handler&& handler)
	{
		assert(in_this_ios());
		if (!released() && running_in_this_thread())
		{
			uv_strand::setImmediate(std::forward<Handler>(handler), _uvLoop);
			return false;
		}
		CHECK_EXCEPTION(handler);
		return true;
	}

	template <typename Handler>
	static void setImmediate(Handler&& handler, uv_loop_t* uvLoop = uv_default_loop())
	{
		typedef wrap_handler<RM_CREF(Handler)> handler_type;
		uv_work_t* uvReq = new uv_work_t;
		uvReq->data = new(malloc(sizeof(handler_type)))handler_type(std::forward<Handler>(handler));
		uv_queue_work(uvLoop, uvReq, [](uv_work_t*){}, [](uv_work_t* uv_op, int status)
		{
			((handler_type*)uv_op->data)->invoke();
			free(uv_op->data);
			delete uv_op;
		});
	}
private:
	uv_loop_t* _uvLoop;
	uv_work_t* _uvReq;
	run_thread::thread_id _threadID;
	reusable_mem_mt<> _reuMem;
	std::mutex _queueMutex;
	std::condition_variable _doRunConVar;
	op_queue _waitQueue;
	op_queue _readyQueue;
	int _waitCount;
	bool _doRunSign;
	bool _waitClose;
	bool _doRunWait;
	bool _locked;
};

#endif
#endif