#ifndef __WRAPPED_TRIG_HANDLER_H
#define __WRAPPED_TRIG_HANDLER_H

#ifdef _DEBUG

#include <boost/detail/interlocked.hpp>

template <typename Handler>
class wrapped_trig_handler
{
public:
	wrapped_trig_handler(const Handler& handler)
		:handler_(handler), pIsTrig(new long(0))
	{

	}

	void operator()()
	{
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
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
		if (!BOOST_INTERLOCKED_EXCHANGE(pIsTrig.get(), 1))
		{
			handler_(arg1, arg2, arg3, arg4);
		}
		else
		{
			assert(false);
		}
	}

	boost::shared_ptr<long> pIsTrig;
	Handler handler_;
};
#endif

#endif