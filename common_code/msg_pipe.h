#ifndef __MSG_PIPE_H
#define __MSG_PIPE_H

#include "actor_framework.h"
#include <boost/atomic/atomic.hpp>
#include <boost/thread/shared_mutex.hpp>

/*!
@brief Actor间通信管道
*/
template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class msg_pipe
{
private:
	template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
	struct pipe_type
	{
		typedef typename std::function<void (T0, T1, T2, T3)> writer_type;
		typedef typename param_list<msg_param<T0, T1, T2, T3> > reader_handle;
	};

	template <typename T0, typename T1, typename T2>
	struct pipe_type<T0, T1, T2, void>
	{
		typedef typename std::function<void (T0, T1, T2)> writer_type;
		typedef typename param_list<msg_param<T0, T1, T2> > reader_handle;
	};

	template <typename T0, typename T1>
	struct pipe_type<T0, T1, void, void>
	{
		typedef typename std::function<void (T0, T1)> writer_type;
		typedef typename param_list<msg_param<T0, T1> > reader_handle;
	};

	template <typename T0>
	struct pipe_type<T0, void, void, void>
	{
		typedef typename std::function<void (T0)> writer_type;
		typedef typename param_list<msg_param<T0> > reader_handle;
	};

	template <>
	struct pipe_type<void, void, void, void>
	{
		typedef typename std::function<void ()> writer_type;
		typedef typename actor_msg_handle<> reader_handle;
	};

	typedef typename pipe_type<T0, T1, T2, T3>::reader_handle reader_handle;
public:
	typedef typename pipe_type<T0, T1, T2, T3>::writer_type writer_type;
	typedef typename std::function<size_t (my_actor*, reader_handle&)> regist_reader;
	typedef typename std::function<writer_type (int timeout)> get_writer_outside;
	__yield_interrupt typedef typename std::function<writer_type (my_actor*, int timeout)> get_writer;
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

		void pop(const writer_type& wt)
		{
			while (!_tempBuff.empty())
			{
				auto t = _tempBuff.front();
				_tempBuff.pop_front();
				wt(t->_p0, t->_p1, t->_p2, t->_p3);
			}
		}

		writer_type temp_writer(std::shared_ptr<temp_buffer>& st)
		{
			return [st](const T0& p0, const T1& p1, const T2& p2, const T3& p3)
			{
				std::shared_ptr<ps> t(new ps(p0, p1, p2, p3));
				boost::lock_guard<boost::mutex> lg(st->_mutex);
				st->_tempBuff.push_back(t);
			};
		}

		list<std::shared_ptr<ps> > _tempBuff;
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

		void pop(const writer_type& wt)
		{
			while (!_tempBuff.empty())
			{
				auto t = _tempBuff.front();
				_tempBuff.pop_front();
				wt(t->_p0, t->_p1, t->_p2);
			}
		}

		writer_type temp_writer(std::shared_ptr<temp_buffer>& st)
		{
			return [st](const T0& p0, const T1& p1, const T2& p2)
			{
				std::shared_ptr<ps> t(new ps(p0, p1, p2));
				boost::lock_guard<boost::mutex> lg(st->_mutex);
				st->_tempBuff.push_back(t);
			};
		}

		list<std::shared_ptr<ps> > _tempBuff;
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

		void pop(const writer_type& wt)
		{
			while (!_tempBuff.empty())
			{
				auto t = _tempBuff.front();
				_tempBuff.pop_front();
				wt(t->_p0, t->_p1);
			}
		}

		writer_type temp_writer(std::shared_ptr<temp_buffer>& st)
		{
			return [st](const T0& p0, const T1& p1)
			{
				std::shared_ptr<ps> t(new ps(p0, p1));
				boost::lock_guard<boost::mutex> lg(st->_mutex);
				st->_tempBuff.push_back(t);
			};
		}

		list<std::shared_ptr<ps> > _tempBuff;
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

		void pop(const writer_type& wt)
		{
			while (!_tempBuff.empty())
			{
				auto t = _tempBuff.front();
				_tempBuff.pop_front();
				wt(t->_p0);
			}
		}

		writer_type temp_writer(std::shared_ptr<temp_buffer>& st)
		{
			return [st](const T0& p0)
			{
				std::shared_ptr<ps> t(new ps(p0));
				boost::lock_guard<boost::mutex> lg(st->_mutex);
				st->_tempBuff.push_back(t);
			};
		}

		list<std::shared_ptr<ps> > _tempBuff;
		boost::mutex _mutex;
	};

	template <>
	struct temp_buffer<void, void, void, void>
	{
		temp_buffer()
			:_count(0)
		{

		}

		void pop(const writer_type& wt)
		{
			while (_count)
			{
				_count--;
				wt();
			}
		}

		writer_type temp_writer(std::shared_ptr<temp_buffer>& st)
		{
			return [st]()
			{
				st->_count++;
			};
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
		std::shared_ptr<wrapped_param> _param;
	};
private:
	struct pipe_param
	{
		pipe_param()
			:_hasWriter(false) {DEBUG_OPERATION(_regCount = 0);}
		~pipe_param()
		{

		}
		bool _hasWriter;
		writer_type _writer;
		boost::mutex _mutex;
		list<std::function<void ()> > _getList;
		DEBUG_OPERATION(boost::atomic<size_t> _regCount);
	};

	struct outsite_pipe_param
	{
		outsite_pipe_param()
			:_hasWriter(false), _waitCount(0) {DEBUG_OPERATION(_regCount = 0);}
		~outsite_pipe_param()
		{

		}
		bool _hasWriter;
		writer_type _writer;
		size_t _waitCount;
		boost::mutex _mutex;
		boost::condition_variable _conVar;
		DEBUG_OPERATION(boost::atomic<size_t> _regCount);
	};
public:
	struct io 
	{
		regist_reader reader;
		writer_type writer;
	};
private:
	msg_pipe()
	{

	}
public:
	/*!
	@brief 创建一个Actor消息管道
	@param writer 返回写管道函数
	@return 返回注册读取数据的函数
	*/
	static regist_reader make(__out writer_type& writer)
	{
		std::shared_ptr<temp_buffer<T0, T1, T2, T3> > tempBuff(new temp_buffer<T0, T1, T2, T3>());
		wrapped_invoke<writer_type> wrapWriter(tempBuff->temp_writer(tempBuff));
		writer = wrapWriter;

		std::weak_ptr<temp_buffer<T0, T1, T2, T3> > weakBuff = tempBuff;
		std::shared_ptr<wrapped_param> wrappedParam = wrapWriter._param;
		return [wrappedParam, weakBuff](my_actor* hostActor, reader_handle& rh)->size_t
		{
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
			size_t regCount = 0;
			{
				auto tb = weakBuff.lock();
				boost::unique_lock<boost::shared_mutex> ul(wrappedParam->_mutex);
				wrappedParam->_handler = hostActor->make_msg_notifer(rh);
				regCount = wrappedParam->_count++;
				if (tb)
				{
					tb->pop(wrappedParam->_handler);
				}
			}
			SetThreadPriority(GetCurrentThread(), hostActor->this_strand()->get_ios_proxy().getPriority());
			return regCount;
		};
	}

	/*!
	@brief 创建一个io
	*/
	static io make_io()
	{
		io res;
		res.reader = make(res.writer);
		return res;
	}
	
	/*!
	@brief Actor内创建一个消息管道
	@param getWriterFunc 返回获取写管道的函数，只有在调用regist_reader返回后才能从该函数中得到结果
	@return 返回注册读取数据的函数，只能注册一次
	*/
	static regist_reader make(__out get_writer& getWriterFunc)
	{
		std::shared_ptr<pipe_param> pipeParam(new pipe_param);
		getWriterFunc = [pipeParam](my_actor* hostActor, int timeout)->writer_type
		{
			async_trig_handle<> ath;
			pipeParam->_mutex.lock();
			if (!pipeParam->_hasWriter)
			{
				pipeParam->_getList.push_back(hostActor->begin_trig(ath));
				pipeParam->_mutex.unlock();
				if (timeout >= 0)
				{
					hostActor->open_timer();
				}
				if (!hostActor->timed_wait_trig(ath, timeout))
				{
					return msg_pipe<T0, T1, T2, T3>::writer_type();
				}
			}
			else
			{
				pipeParam->_mutex.unlock();
			}
			return pipeParam->_writer;
		};

		std::weak_ptr<pipe_param> weakParam = pipeParam;
		return [weakParam](my_actor* hostActor, reader_handle& rh)->size_t
		{
			std::shared_ptr<pipe_param> pipeParam = weakParam.lock();
			if (pipeParam)
			{
				assert(0 == pipeParam->_regCount++);
				pipeParam->_writer = hostActor->make_msg_notifer(rh);
				pipeParam->_mutex.lock();
				pipeParam->_hasWriter = true;
				while (!pipeParam->_getList.empty())
				{
					pipeParam->_getList.front()();
					pipeParam->_getList.pop_front();
				}
				pipeParam->_mutex.unlock();
				return 0;
			}
			return -1;
		};
	}
	
	/*!
	@brief 同上，get_writer_outside 只能在与调用 regist_reader 不同的调度器内获取 writer_type
	*/
	static regist_reader make(__out get_writer_outside& getWriterFunc)
	{
		std::shared_ptr<outsite_pipe_param> pipeParam(new outsite_pipe_param);
		getWriterFunc = [pipeParam](int timeout)->writer_type
		{
			{
				boost::unique_lock<boost::mutex> ul(pipeParam->_mutex);
				if (!pipeParam->_hasWriter)
				{
					pipeParam->_waitCount++;
					if (timeout < 0)
					{
						pipeParam->_conVar.wait(ul);
					}
					else if (!pipeParam->_conVar.timed_wait(ul, boost::posix_time::milliseconds(timeout)))
					{
						return msg_pipe<T0, T1, T2, T3>::writer_type();
					}
				}
			}
			return pipeParam->_writer;
		};

		std::weak_ptr<outsite_pipe_param> weakParam = pipeParam;
		return [weakParam](my_actor* hostActor, reader_handle& rh)->size_t
		{
			std::shared_ptr<outsite_pipe_param> pipeParam = weakParam.lock();
			if (pipeParam)
			{
				assert(0 == pipeParam->_regCount++);
				pipeParam->_writer = hostActor->make_msg_notifer(rh);
				boost::unique_lock<boost::mutex> ul(pipeParam->_mutex);
				pipeParam->_hasWriter = true;
				if (pipeParam->_waitCount)
				{
					pipeParam->_waitCount = 0;
					pipeParam->_conVar.notify_all();
				}
				return 0;
			}
			return -1;
		};
	}
private:
};

#endif