#ifndef __WRAPPED_NO_PARAMS_HANDLER_H
#define __WRAPPED_NO_PARAMS_HANDLER_H

template <typename Handler = void>
class wrapped_no_params_handler
{
public:
	wrapped_no_params_handler(const Handler& handler)
		: _handler(handler)
	{
	}

	void operator()()
	{
		_handler();
	}

	void operator()() const
	{
		_handler();
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1)
	{
		_handler();
	}

	template <typename Arg1>
	void operator()(const Arg1& arg1) const
	{
		_handler();
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2)
	{
		_handler();
	}

	template <typename Arg1, typename Arg2>
	void operator()(const Arg1& arg1, const Arg2& arg2) const
	{
		_handler();
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
	{
		_handler();
	}

	template <typename Arg1, typename Arg2, typename Arg3>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
	{
		_handler();
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4)
	{
		_handler();
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4) const
	{
		_handler();
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5)
	{
		_handler();
	}

	template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
		typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5) const
	{
		_handler();
	}

	Handler _handler;
};

template <typename H>
wrapped_no_params_handler<H> wrap_no_params(const H& h)
{
	return wrapped_no_params_handler<H>(h);
}

#endif