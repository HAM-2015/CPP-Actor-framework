#include "generator.h"

const shared_bool& __make_co_shared_sign(struct co_context_base* __ctx)
{
	if (__ctx->_sharedSign.empty())
	{
		__ctx->_sharedSign = shared_bool::new_(false);
	}
	return __ctx->_sharedSign;
}

generator::generator()
: _ctx(NULL) {}

bool generator::_next()
{
	assert(_strand->running_in_this_thread());
	assert(_handler);
	assert(!_ctx || !_ctx->__inside);
	DEBUG_OPERATION(if (_ctx) _ctx->__inside = true);
	CHECK_EXCEPTION(_handler, *this);
	assert(!_ctx || _ctx->__coNext);
	if (!_ctx)
	{
		clear_function(_handler);
		if (_notify)
		{
			CHECK_EXCEPTION(_notify);
			clear_function(_notify);
		}
	}
	return !_ctx;
}

void generator::stop()
{
	if (_strand->running_in_this_thread())
	{
		if (_ctx)
		{
			if (!_ctx->__lockStop)
			{
				bool inside = 0 == _ctx->__coNext;
				assert(inside == _ctx->__inside);
				_ctx->__coNext = -1;
				if (!inside)
				{
					_next();
				}
			}
			else
			{
				_ctx->__readyQuit = true;
			}
		}
		else
		{
			clear_function(_handler);
			clear_function(_notify);
		}
	} 
	else
	{
		_strand->post(std::bind([](generator_handle& gen)
		{
			if (gen->_ctx)
			{
				gen->_ctx->__coNext = -1;
				gen->_next();
			}
			else
			{
				clear_function(gen->_handler);
				clear_function(gen->_notify);
			}
		}, shared_from_this()));
	}
}

generator_handle generator::shared_from_this()
{
	return _weakThis.lock();
}

const shared_strand& generator::gen_strand()
{
	return _strand;
}

generator_handle generator::_begin_fork()
{
	return generator::create(_strand, _handler);
}