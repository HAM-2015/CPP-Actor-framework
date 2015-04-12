#ifndef __WRAPPED_TRIG_HANDLER_H
#define __WRAPPED_TRIG_HANDLER_H

#ifdef _DEBUG

#include <boost/atomic/atomic.hpp>

template <typename Handler = void>
class wrapped_trig_handler
{
public:
	wrapped_trig_handler(const Handler& handler)
		:handler_(handler), pIsTrig(new boost::atomic<bool>(false))
	{

	}

	void operator()()
	{
		if (!pIsTrig->exchange(true))
		{
			handler_();
		}
		else
		{
			assert(false);
		}
	}

	void operator()() const
	{
		if (!pIsTrig->exchange(true))
		{
			handler_();
		}
		else
		{
			assert(false);
		}
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1)
	{
		if (!pIsTrig->exchange(true))
		{
			handler_(arg1);
		}
		else
		{
			assert(false);
		}
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1) const
	{
		if (!pIsTrig->exchange(true))
		{
			handler_(arg1);
		}
		else
		{
			assert(false);
		}
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2)
	{
		if (!pIsTrig->exchange(true))
		{
			handler_(arg1, arg2);
		}
		else
		{
			assert(false);
		}
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2) const
	{
		if (!pIsTrig->exchange(true))
		{
			handler_(arg1, arg2);
		}
		else
		{
			assert(false);
		}
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
	{
		if (!pIsTrig->exchange(true))
		{
			handler_(arg1, arg2, arg3);
		}
		else
		{
			assert(false);
		}
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
	{
		if (!pIsTrig->exchange(true))
		{
			handler_(arg1, arg2, arg3);
		}
		else
		{
			assert(false);
		}
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
	{
		if (!pIsTrig->exchange(true))
		{
			handler_(arg1, arg2, arg3, arg4);
		}
		else
		{
			assert(false);
		}
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4) const
	{
		if (!pIsTrig->exchange(true))
		{
			handler_(arg1, arg2, arg3, arg4);
		}
		else
		{
			assert(false);
		}
	}

	std::shared_ptr<boost::atomic<bool> > pIsTrig;
	Handler handler_;
};

template <typename H>
static wrapped_trig_handler<H> wrap_trig(const H& h)
{
	return wrapped_trig_handler<H>(h);
}

#else

template <typename H>
static H& wrap_trig(const H& h)
{
	return (H&)h;
}

#endif

#endif