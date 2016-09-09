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

bool generator::next()
{
	assert(_handler);
	assert(!_ctx || !_ctx->__inside);
	DEBUG_OPERATION(if (_ctx) _ctx->__inside = true);
	CHECK_EXCEPTION(_handler, *this);
	if (_ctx && !_ctx->__coNext)
	{
		delete _ctx;
		_ctx = NULL;
	}
	if (!_ctx && _notify)
	{
		CHECK_EXCEPTION(_notify);
	}
	return !_ctx;
}

generator_handle generator::shared_from_this()
{
	return _weakThis.lock();
}

const shared_strand& generator::curr_strand()
{
	return _strand;
}