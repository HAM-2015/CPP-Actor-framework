#ifndef __MSG_PIPE_H
#define __MSG_PIPE_H

#include "boost_coroutine.h"
#include <boost/atomic/atomic.hpp>
#include <boost/thread/shared_mutex.hpp>

/*!
@brief 协程间通信管道
*/
template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class msg_pipe
{
private:
	template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
	struct pipe_type
	{
		typedef typename boost::function<void (T0, T1, T2, T3)> writer_type;
		typedef typename param_list<msg_param<T0, T1, T2, T3> > reader_type;
		typedef typename coro_msg_handle<T0, T1, T2, T3> reader_handle;
	};

	template <typename T0, typename T1, typename T2>
	struct pipe_type<T0, T1, T2, void>
	{
		typedef typename boost::function<void (T0, T1, T2)> writer_type;
		typedef typename param_list<msg_param<T0, T1, T2> > reader_type;
		typedef typename coro_msg_handle<T0, T1, T2> reader_handle;
	};

	template <typename T0, typename T1>
	struct pipe_type<T0, T1, void, void>
	{
		typedef typename boost::function<void (T0, T1)> writer_type;
		typedef typename param_list<msg_param<T0, T1> > reader_type;
		typedef typename coro_msg_handle<T0, T1> reader_handle;
	};

	template <typename T0>
	struct pipe_type<T0, void, void, void>
	{
		typedef typename boost::function<void (T0)> writer_type;
		typedef typename param_list<msg_param<T0> > reader_type;
		typedef typename coro_msg_handle<T0> reader_handle;
	};

	template <>
	struct pipe_type<void, void, void, void>
	{
		typedef typename boost::function<void ()> writer_type;
		typedef typename coro_msg_handle<> reader_type;
		typedef typename coro_msg_handle<> reader_handle;
	};
public:
	typedef typename pipe_type<T0, T1, T2, T3>::writer_type writer_type;
	typedef typename pipe_type<T0, T1, T2, T3>::reader_type reader_type;
	typedef typename boost::function<size_t (boost_coro*, reader_type&)> regist_reader;
	__yield_interrupt typedef typename boost::function<writer_type (boost_coro*, int timeout)> get_writer;
private:
	template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
	struct temp_buffer
	{
		struct ps 
		{
			ps(const T0& p0, const T1& p1, const T2& p2, const T3& p3)
				:_p0(p0), _p1(p1), _p2(p2), _p3(p3) {}

			T0 _p0;
			T1 _p1;
			T2 _p2;
			T3 _p3;
		};

		void push(const T0& p0, const T1& p1, const T2& p2, const T3& p3)
		{
			boost::shared_ptr<ps> t(new ps(p0, p1, p2, p3));
			boost::lock_guard<boost::mutex> lg(_mutex);
			_tempBuff.push_back(t);
		}

		void pop(const writer_type& wt)
		{
			while (!_tempBuff.empty())
			{
				auto t = _tempBuff.front();
				_tempBuff.pop_front();
				wt(t->_p0, t->_p1, t->_p2, t->_p3);
			}
		}

		writer_type temp_writer(boost::shared_ptr<temp_buffer>& st)
		{
			return boost::bind(&temp_buffer::push, st, _1, _2, _3, _4);
		}

		list<boost::shared_ptr<ps> > _tempBuff;
		boost::mutex _mutex;
	};

	template <typename T0, typename T1, typename T2>
	struct temp_buffer<T0, T1, T2, void>
	{
		struct ps 
		{
			ps(const T0& p0, const T1& p1, const T2& p2)
				:_p0(p0), _p1(p1), _p2(p2) {}

			T0 _p0;
			T1 _p1;
			T2 _p2;
		};

		void push(const T0& p0, const T1& p1, const T2& p2)
		{
			boost::shared_ptr<ps> t(new ps(p0, p1, p2));
			boost::lock_guard<boost::mutex> lg(_mutex);
			_tempBuff.push_back(t);
		}

		void pop(const writer_type& wt)
		{
			while (!_tempBuff.empty())
			{
				auto t = _tempBuff.front();
				_tempBuff.pop_front();
				wt(t->_p0, t->_p1, t->_p2);
			}
		}

		writer_type temp_writer(boost::shared_ptr<temp_buffer>& st)
		{
			return boost::bind(&temp_buffer::push, st, _1, _2, _3);
		}

		list<boost::shared_ptr<ps> > _tempBuff;
		boost::mutex _mutex;
	};

	template <typename T0, typename T1>
	struct temp_buffer<T0, T1, void, void>
	{
		struct ps 
		{
			ps(const T0& p0, const T1& p1)
				:_p0(p0), _p1(p1) {}

			T0 _p0;
			T1 _p1;
		};

		void push(const T0& p0, const T1& p1)
		{
			boost::shared_ptr<ps> t(new ps(p0, p1));
			boost::lock_guard<boost::mutex> lg(_mutex);
			_tempBuff.push_back(t);
		}

		void pop(const writer_type& wt)
		{
			while (!_tempBuff.empty())
			{
				auto t = _tempBuff.front();
				_tempBuff.pop_front();
				wt(t->_p0, t->_p1);
			}
		}

		writer_type temp_writer(boost::shared_ptr<temp_buffer>& st)
		{
			return boost::bind(&temp_buffer::push, st, _1, _2);
		}

		list<boost::shared_ptr<ps> > _tempBuff;
		boost::mutex _mutex;
	};

	template <typename T0>
	struct temp_buffer<T0, void, void, void>
	{
		struct ps 
		{
			ps(const T0& p0)
				:_p0(p0) {}

			T0 _p0;
		};

		void push(const T0& p0)
		{
			boost::shared_ptr<ps> t(new ps(p0));
			boost::lock_guard<boost::mutex> lg(_mutex);
			_tempBuff.push_back(t);
		}

		void pop(const writer_type& wt)
		{
			while (!_tempBuff.empty())
			{
				auto t = _tempBuff.front();
				_tempBuff.pop_front();
				wt(t->_p0);
			}
		}

		writer_type temp_writer(boost::shared_ptr<temp_buffer>& st)
		{
			return boost::bind(&temp_buffer::push, st, _1);
		}

		list<boost::shared_ptr<ps> > _tempBuff;
		boost::mutex _mutex;
	};

	template <>
	struct temp_buffer<void, void, void, void>
	{
		temp_buffer()
			:_count(0)
		{

		}

		void push()
		{
			_count++;
		}

		void pop(const writer_type& wt)
		{
			while (_count)
			{
				_count--;
				wt();
			}
		}

		writer_type temp_writer(boost::shared_ptr<temp_buffer>& st)
		{
			return boost::bind(&temp_buffer::push, st);
		}

		boost::atomic<size_t> _count;
	};

	struct wrapped_param 
	{
		size_t _count;
		writer_type _handler;
		boost::shared_mutex _mutex;
	};

	template <typename Handler>
	class wrapped_invoke
	{
	public:
		wrapped_invoke(const Handler& handler)
			: _param(new wrapped_param)
		{
			_param->_count = 0;
			_param->_handler = handler;
		}
	public:
		void operator()()
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler();
		}

		void operator()() const
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler();
		}

		template <typename Arg1>
		void operator()(const Arg1& arg1)
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler(arg1);
		}

		template <typename Arg1>
		void operator()(const Arg1& arg1) const
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler(arg1);
		}

		template <typename Arg1, typename Arg2>
		void operator()(const Arg1& arg1, const Arg2& arg2)
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler(arg1, arg2);
		}

		template <typename Arg1, typename Arg2>
		void operator()(const Arg1& arg1, const Arg2& arg2) const
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler(arg1, arg2);
		}

		template <typename Arg1, typename Arg2, typename Arg3>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler(arg1, arg2, arg3);
		}

		template <typename Arg1, typename Arg2, typename Arg3>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler(arg1, arg2, arg3);
		}

		template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
			const Arg4& arg4)
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler(arg1, arg2, arg3, arg4);
		}

		template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
			const Arg4& arg4) const
		{
			boost::shared_lock<boost::shared_mutex> sl(_param->_mutex);
			_param->_handler(arg1, arg2, arg3, arg4);
		}
	public:
		boost::shared_ptr<wrapped_param> _param;
	};

	typedef typename pipe_type<T0, T1, T2, T3>::reader_handle reader_handle;
private:
	struct pipe_param
	{
		pipe_param()
			:_hasWriter(false) {_regCount = 0;}
		bool _hasWriter;
		writer_type _writer;
		list<boost::function<void ()> > _getList;
		boost::mutex _mutex;
		boost::function<void (writer_type)> _registWriter;
		boost::atomic<size_t> _regCount;
	};

	struct pipe_lock 
	{
		~pipe_lock()
		{
			auto tc = _proxyCoro.lock();
			if (tc)
			{
				tc->notify_force_quit();//没有去获取reader，协程没必要等待了，可以强制退出
			}
		}

		boost::weak_ptr<boost_coro> _proxyCoro;
	};
private:
	msg_pipe()
	{

	}
public:
	/*!
	@brief 创建一个协程消息管道
	@param writer 返回写管道函数
	@return 返回注册读取数据的函数，只能注册一个
	*/
	static regist_reader make(__out writer_type& writer)
	{
		boost::shared_ptr<temp_buffer<T0, T1, T2, T3> > tempBuff(new temp_buffer<T0, T1, T2, T3>());
		wrapped_invoke<writer_type> wrapWriter(tempBuff->temp_writer(tempBuff));
		writer = wrapWriter;
		return boost::bind(&msg_pipe::registReader, wrapWriter._param, (boost::weak_ptr<temp_buffer<T0, T1, T2, T3> >)tempBuff, _1, _2);
	}
	
	/*!
	@brief 协程内创建一个消息管道
	@param coro 调用该函数的协程
	@param strand 管道依赖的strand
	@param getWriterFunc 返回获取写管道的函数，只有在调用regist_reader返回后才能从该函数中得到结果
	@return 返回注册读取数据的函数，只能调用一次
	*/
	__yield_interrupt static regist_reader make(boost_coro* coro, shared_strand strand, __out get_writer& getWriterFunc)
	{
		boost::shared_ptr<pipe_param> pipeParam(new pipe_param);
		async_trig_handle<> ath;
		child_coro_handle tempCoro = coro->create_child_coro(strand, boost::bind(&msg_pipe::pipeCoro1, _1, pipeParam, 
			coro->begin_trig(ath)));
		coro->child_coro_run(tempCoro);
		coro->wait_trig(ath);
		boost::shared_ptr<pipe_lock> pipeLock(new pipe_lock);
		pipeLock->_proxyCoro = coro->child_coro_peel(tempCoro);
		getWriterFunc = boost::bind(&msg_pipe::getWriter, pipeParam, _1, _2);
		return boost::bind(&msg_pipe::registReader1, pipeLock, pipeParam, _1, _2);
	}

	__yield_interrupt static regist_reader make(boost_coro* coro, __out get_writer& getWriterFunc)
	{
		return make(coro, coro->this_strand(), getWriterFunc);
	}
	
	/*!
	@brief 同上，协程依赖的ios无关线程中创建一个消息管道
	*/
	static regist_reader outside_make(shared_strand strand, __out get_writer& getWriterFunc)
	{
		assert(!strand->in_this_ios());
		boost::shared_ptr<pipe_param> pipeParam(new pipe_param);
		boost::mutex mutex;
		boost::condition_variable conVar;
		coro_handle tempCoro = boost_coro::outside_create(strand, boost::bind(&msg_pipe::pipeCoro2, _1, pipeParam, 
			&mutex, &conVar));
		{
			boost::unique_lock<boost::mutex> ul(mutex);
			tempCoro->notify_start_run();
			conVar.wait(ul);
		}
		boost::shared_ptr<pipe_lock> pipeLock(new pipe_lock);
		pipeLock->_proxyCoro = tempCoro;
		getWriterFunc = boost::bind(&msg_pipe::getWriter, pipeParam, _1, _2);
		return boost::bind(&msg_pipe::registReader1, pipeLock, pipeParam, _1, _2);
	}
private:
	/*!
	@brief 注册消费者，不要多个线程同时注册
	@return 返回第几次注册
	*/
	static size_t registReader(boost::shared_ptr<wrapped_param>& wrappedParam, boost::weak_ptr<temp_buffer<T0, T1, T2, T3> >& tempBuff, 
		boost_coro* hostCoro, reader_type& cmh)
	{
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		size_t regCount = 0;
		{
			auto tb = tempBuff.lock();
			boost::unique_lock<boost::shared_mutex> ul(wrappedParam->_mutex);
			wrappedParam->_handler = hostCoro->make_msg_notify(cmh);
			regCount = wrappedParam->_count++;
			if (tb)
			{
				tb->pop(wrappedParam->_handler);
			}
		}
		SetThreadPriority(GetCurrentThread(), hostCoro->this_strand()->get_ios_proxy().getPriority());
		return regCount;
	}
	
	/*!
	@brief 注册消费者，只能注册一次
	*/
	static size_t registReader1(boost::shared_ptr<pipe_lock>& pipeLock, boost::shared_ptr<pipe_param>& pipeParam, boost_coro* hostCoro, reader_type& cmh)
	{
		pipeParam->_mutex.lock();
		size_t rc = pipeParam->_regCount++;
		pipeParam->_registWriter(hostCoro->make_msg_notify(cmh));
		pipeParam->_mutex.unlock();
		return rc;
	}

	/*!
	@brief 获取生产者，在另一端调用registReader完成或超时后返回
	*/
	__yield_interrupt static writer_type getWriter(boost::shared_ptr<pipe_param>& pipeParam, boost_coro* hostCoro, int timeout)
	{
		async_trig_handle<> ath;
		pipeParam->_mutex.lock();
		if (!pipeParam->_hasWriter)
		{
			pipeParam->_getList.push_back(hostCoro->begin_trig(ath));
			pipeParam->_mutex.unlock();
			if (timeout >= 0)
			{
				hostCoro->open_timer();
			}
			if (!hostCoro->wait_trig(ath, timeout))
			{
				return writer_type();
			}
		}
		else
		{
			pipeParam->_mutex.unlock();
		}
		return pipeParam->_writer;
	}
	//////////////////////////////////////////////////////////////////////////

	static void pipeCoro1(boost_coro* coro, boost::shared_ptr<pipe_param>& pipeParam, boost::function<void ()>& makeOkTrig)
	{
		async_trig_handle<writer_type> waitReaderAth;
		pipeParam->_registWriter = coro->begin_trig(waitReaderAth);
		makeOkTrig();

		pipeParam->_writer = coro->wait_trig(waitReaderAth);
		pipeParam->_mutex.lock();
		pipeParam->_hasWriter = true;
		while (!pipeParam->_getList.empty())
		{
			pipeParam->_getList.front()();
			pipeParam->_getList.pop_front();
		}
		pipeParam->_mutex.unlock();
	}

	static void pipeCoro2(boost_coro* coro, boost::shared_ptr<pipe_param>& pipeParam, boost::mutex* mutex, boost::condition_variable* conVar)
	{
		async_trig_handle<writer_type> waitReaderAth;
		pipeParam->_registWriter = coro->begin_trig(waitReaderAth);
		{
			boost::lock_guard<boost::mutex> lg(*mutex);
			conVar->notify_one();
		}

		pipeParam->_writer = coro->wait_trig(waitReaderAth);
		pipeParam->_mutex.lock();
		pipeParam->_hasWriter = true;
		while (!pipeParam->_getList.empty())
		{
			pipeParam->_getList.front()();
			pipeParam->_getList.pop_front();
		}
		pipeParam->_mutex.unlock();
	}
};

#endif