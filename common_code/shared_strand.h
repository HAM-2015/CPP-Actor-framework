#ifndef __SHARED_STRAND_H
#define __SHARED_STRAND_H


#ifdef ENABLE_STRAND_IMPL_POOL
#include "strand_ex.h"
#else
//#define BOOST_ASIO_ENABLE_SEQUENTIAL_STRAND_ALLOCATION
//#define BOOST_ASIO_STRAND_IMPLEMENTATIONS	11//单个ios中strand最大数目（超过会重叠）
#include <boost/asio/strand.hpp>
#endif //end ENABLE_SHARE_STRAND_IMPL

#include <boost/asio/io_service.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <functional>
#include <memory>
#include "ios_proxy.h"
#include "msg_queue.h"
#include "wrapped_post_handler.h"
#include "wrapped_try_tick_handler.h"
#include "wrapped_dispatch_handler.h"
#include "wrapped_next_tick_handler.h"

class actor_timer;

class boost_strand;
typedef std::shared_ptr<boost_strand> shared_strand;

#define SPACE_SIZE		(sizeof(void*)*16)

#ifdef ENABLE_NEXT_TICK

#define RUN_HANDLER handler_capture<RM_REF(Handler)>(TRY_MOVE(handler), this)

#else //ENABLE_NEXT_TICK

#define RUN_HANDLER TRY_MOVE(handler)

#endif //ENABLE_NEXT_TICK


#define UI_POST()\
if (_strand)\
{\
	_strand->post(RUN_HANDLER); \
}\
else\
{\
	_post(TRY_MOVE(handler)); \
};


#ifdef ENABLE_NEXT_TICK

#define APPEND_TICK()	\
	static_assert(sizeof(wrap_next_tick_space) == sizeof(wrap_next_tick_handler<RM_REF(Handler)>), "next tick wrap error");\
	typedef wrap_next_tick_handler<RM_REF(Handler), sizeof(RM_REF(Handler)) <= SPACE_SIZE> wrap_tick_type;\
	_nextTickQueue.push_back(wrap_tick_type::create(TRY_MOVE(handler), _nextTickAlloc));

#else //ENABLE_NEXT_TICK

#define APPEND_TICK()	post(TRY_MOVE(handler));

#endif //ENABLE_NEXT_TICK


#define UI_NEXT_TICK()\
if (_strand)\
{\
	APPEND_TICK(); \
}\
else\
{\
	_post(TRY_MOVE(handler)); \
}

/*!
@brief 重新定义dispatch实现，所有不同strand中以消息方式进行函数调用
*/
class boost_strand
{
#ifdef ENABLE_STRAND_IMPL_POOL
	typedef strand_ex strand_type;
#else
	typedef boost::asio::strand strand_type;
#endif

#ifdef ENABLE_NEXT_TICK

	template <typename H>
	struct handler_capture
	{
		template <typename Handler>
		handler_capture(Handler&& handler, boost_strand* strand)
			:_handler(TRY_MOVE(handler)), _strand(strand) {}

		handler_capture(const handler_capture& s)
			:_handler(std::move(s._handler)), _strand(s._strand) {static_assert(false, "no copy"); }

		handler_capture(handler_capture&& s)
			:_handler(std::move(s._handler)), _strand(s._strand) {}

		void operator =(const handler_capture& s)
		{
			static_assert(false, "no copy");
			_handler = std::move(s._handler);
			_strand = s._strand;
		}

		void operator =(handler_capture&& s)
		{
			_handler = std::move(s._handler);
			_strand = s._strand;
		}

		void operator ()()
		{
			bool checkDestroy = false;
			_strand->_pCheckDestroy = &checkDestroy;
			_handler();
			if (!checkDestroy)
			{
				_strand->_pCheckDestroy = NULL;
				if (!_strand->_nextTickQueue.empty())
				{
					_strand->run_tick();
				}
			}
		}

		mutable H _handler;
		boost_strand* _strand;
	};

	struct wrap_next_tick_base
	{
		virtual void invoke() = 0;
		virtual void* destroy() = 0;
	};

	struct wrap_next_tick_space : public wrap_next_tick_base
	{
		unsigned char space[SPACE_SIZE];
	};

	template <typename H, bool = true>
	struct wrap_next_tick_handler : public wrap_next_tick_space
	{
		template <typename Handler>
		wrap_next_tick_handler(Handler&& h)
		{
			assert(sizeof(H) <= sizeof(space));
			new(space)H(TRY_MOVE(h));
		}

		template <typename Handler>
		static inline wrap_next_tick_base* create(Handler&& handler, mem_alloc<wrap_next_tick_space>& alloc)
		{
			if (!alloc.full())
			{
				return new(alloc.allocate())wrap_next_tick_handler<RM_REF(Handler)>(TRY_MOVE(handler));
			}
			else
			{
				return new wrap_next_tick_handler<RM_REF(Handler), false>(TRY_MOVE(handler));
			}
		}

		void invoke()
		{
			(*(H*)space)();
		}

		void* destroy()
		{
			((H*)space)->~H();
			return this;
		}
	};

	template <typename H>
	struct wrap_next_tick_handler<H, false> : public wrap_next_tick_base
	{
		template <typename Handler>
		wrap_next_tick_handler(Handler&& h)
			:_h(TRY_MOVE(h)) {}

		template <typename Handler>
		static inline wrap_next_tick_base* create(Handler&& handler, mem_alloc<wrap_next_tick_space>& alloc)
		{
			return new wrap_next_tick_handler<RM_REF(Handler), false>(TRY_MOVE(handler));
		}

		void invoke()
		{
			_h();
		}

		void* destroy()
		{
			delete this;
			return NULL;
		}

		H _h;
	};
#endif //ENABLE_NEXT_TICK

protected:
	boost_strand();
	virtual ~boost_strand();
public:
	static shared_strand create(ios_proxy& iosProxy, bool makeTimer = true);
public:
	/*!
	@brief 如果在本strand中调用则直接执行，否则添加到队列中等待执行
	@param handler 被调用函数
	*/
	template <typename Handler>
	void dispatch(Handler&& handler)
	{
		if (running_in_this_thread())
		{
			handler();
		} 
		else
		{
			post(TRY_MOVE(handler));
		}
	}

	/*!
	@brief 添加一个任务到 strand 队列
	*/
	template <typename Handler>
	void post(Handler&& handler)
	{
#ifdef ENABLE_MFC_ACTOR
		UI_POST();
#elif ENABLE_WX_ACTOR
		UI_POST();
#else
		_strand->post(RUN_HANDLER);
#endif
	}

	/*!
	@brief 添加一个任务到 next_tick 队列
	*/
	template <typename Handler>
	void next_tick(Handler&& handler)
	{
		assert(running_in_this_thread());
#ifdef ENABLE_MFC_ACTOR
		UI_NEXT_TICK();
#elif ENABLE_WX_ACTOR
		UI_NEXT_TICK();
#else
		APPEND_TICK();
#endif
	}

	/*!
	@brief 尝试添加一个任务到 next_tick 队列
	*/
	template <typename Handler>
	void try_tick(Handler&& handler)
	{
#ifdef ENABLE_NEXT_TICK
		if (running_in_this_thread())
		{
			next_tick(TRY_MOVE(handler));
		} 
		else
#endif //ENABLE_NEXT_TICK
		{
			post(TRY_MOVE(handler));
		}
	}

	/*!
	@brief 把被调用函数包装到dispatch中，用于不同strand间消息传递
	*/
	template <typename Handler>
	wrapped_dispatch_handler<boost_strand, RM_CREF(Handler)> wrap(Handler&& handler)
	{
		return wrapped_dispatch_handler<boost_strand, RM_CREF(Handler)>(this, TRY_MOVE(handler));
	}

	/*!
	@brief 把被调用函数包装到dispatch中，用于不同strand间消息传递，调用后参数将强制被右值引用，且只能调用一次
	*/
	template <typename Handler>
	wrapped_dispatch_handler<boost_strand, RM_CREF(Handler), true> suck_wrap(Handler&& handler)
	{
		return wrapped_dispatch_handler<boost_strand, RM_CREF(Handler), true>(this, TRY_MOVE(handler));
	}
	
	/*!
	@brief 把被调用函数包装到post中
	*/
	template <typename Handler>
	wrapped_post_handler<boost_strand, RM_CREF(Handler)> wrap_post(Handler&& handler)
	{
		return wrapped_post_handler<boost_strand, RM_CREF(Handler)>(this, TRY_MOVE(handler));
	}

	/*!
	@brief 把被调用函数包装到post中，调用后参数将强制被右值引用，且只能调用一次
	*/
	template <typename Handler>
	wrapped_post_handler<boost_strand, RM_CREF(Handler), true> suck_wrap_post(Handler&& handler)
	{
		return wrapped_post_handler<boost_strand, RM_CREF(Handler), true>(this, TRY_MOVE(handler));
	}

	/*!
	@brief 把被调用函数包装到next_tick中
	*/
	template <typename Handler>
	wrapped_next_tick_handler<boost_strand, RM_CREF(Handler)> wrap_next_tick(Handler&& handler)
	{
		return wrapped_next_tick_handler<boost_strand, RM_CREF(Handler)>(this, TRY_MOVE(handler));
	}

	/*!
	@brief 把被调用函数包装到next_tick中，调用后参数将强制被右值引用，且只能调用一次
	*/
	template <typename Handler>
	wrapped_next_tick_handler<boost_strand, RM_CREF(Handler), true> suck_wrap_next_tick(Handler&& handler)
	{
		return wrapped_next_tick_handler<boost_strand, RM_CREF(Handler), true>(this, TRY_MOVE(handler));
	}

	/*!
	@brief 把被调用函数包装到tick中
	*/
	template <typename Handler>
	wrapped_try_tick_handler<boost_strand, RM_CREF(Handler)> wrap_try_tick(Handler&& handler)
	{
		return wrapped_try_tick_handler<boost_strand, RM_CREF(Handler)>(this, TRY_MOVE(handler));
	}

	/*!
	@brief 把被调用函数包装到tick中，调用后参数将强制被右值引用，且只能调用一次
	*/
	template <typename Handler>
	wrapped_try_tick_handler<boost_strand, RM_CREF(Handler), true> suck_wrap_try_tick(Handler&& handler)
	{
		return wrapped_try_tick_handler<boost_strand, RM_CREF(Handler), true>(this, TRY_MOVE(handler));
	}

	/*!
	@brief 复制一个依赖同一个ios的strand
	*/
	virtual shared_strand clone();

	/*!
	@brief 检查是否在所依赖ios线程中执行
	@return true 在, false 不在
	*/
	virtual bool in_this_ios();

	/*!
	@brief 依赖的ios调度器运行线程数
	*/
	size_t ios_thread_number();

	/*!
	@brief 判断是否在本strand中运行
	*/
	virtual bool running_in_this_thread();

	/*!
	@brief 当前任务队列是否为空(不算当前)
	@param checkTick 是否计算tick任务
	*/
	virtual bool empty(bool checkTick = true);

	/*!
	@brief 获取当前调度器代理
	*/
	ios_proxy& get_ios_proxy();

	/*!
	@brief 获取当前调度器
	*/
	boost::asio::io_service& get_io_service();

	/*!
	@brief 获取定时器
	*/
	actor_timer* get_timer();
private:
#ifdef ENABLE_NEXT_TICK
	void run_tick();
	bool* _pCheckDestroy;
	mem_alloc<wrap_next_tick_space> _nextTickAlloc;
	msg_queue<wrap_next_tick_base*> _nextTickQueue;
#endif //ENABLE_NEXT_TICK
protected:
#if (defined ENABLE_MFC_ACTOR || defined ENABLE_WX_ACTOR)
	virtual void _post(const std::function<void ()>& h);
#endif
	actor_timer* _timer;
	ios_proxy* _iosProxy;
	strand_type* _strand;
	std::weak_ptr<boost_strand> _weakThis;
public:
	/*!
	@brief 在一个strand中调用某个函数，直到这个函数被执行完成后才返回
	@warning 此函数有可能使整个程序陷入死锁，只能在与strand所依赖的ios无关线程中调用
	*/
	template <typename H>
	void syncInvoke(H&& h)
	{
		assert(!in_this_ios());
		boost::mutex mutex;
		boost::condition_variable con;
		boost::unique_lock<boost::mutex> ul(mutex);
		post([&]
		{
			h();
			mutex.lock();
			con.notify_one();
			mutex.unlock();
		});
		con.wait(ul);
	}

	/*!
	@brief 同上，带返回值
	*/
	template <typename R, typename H>
	R syncInvoke(H&& h)
	{
		assert(!in_this_ios());
		R r;
		boost::mutex mutex;
		boost::condition_variable con;
		boost::unique_lock<boost::mutex> ul(mutex);
		post([&]
		{
			r = h();
			mutex.lock();
			con.notify_one();
			mutex.unlock();
		});
		con.wait(ul);
		return r;
	}

	/*!
	@brief 在strand中调用某个带返回值函数，然后通过一个回调函数把返回值传出
	@param cb 传出参数的函数
	*/
	template <typename H, typename CB>
	void asyncInvoke(H&& h, CB&& cb)
	{
		post([=]
		{
			cb(h());
		});
	}

	template <typename H, typename CB>
	void asyncInvokeVoid(H&& h, CB&& cb)
	{
		post([=]
		{
			h();
			cb();
		});
	}
};

#endif