#ifndef __GENERATOR_H
#define __GENERATOR_H

#include "my_actor.h"

#define co_generator generator& gen

#define co_begin_context static_assert(__COUNTER__+1 == __COUNTER__, ""); int no_capture = 0; struct co_context_tag: public co_context_base {

#define co_end_context(__ctx__) };\
	if (!gen._ctx){\
	gen._ctx = new co_context_tag();\
	DEBUG_OPERATION(gen._ctx->__inside = true);}\
	struct co_context_base* __ctx = gen._ctx;\
	struct co_context_tag& __ctx__ = *(struct co_context_tag*)gen._ctx;\
	bool __yieldSign = false; {

#define _co_capture1() co_context_tag()
#define _co_capture2(__p1__) co_context_tag(decltype(__p1__)& __p1__)
#define _co_capture3(__p1__, __p2__) co_context_tag(decltype(__p1__)& __p1__, decltype(__p2__)& __p2__)
#define _co_capture4(__p1__, __p2__, __p3__) co_context_tag(decltype(__p1__)& __p1__, decltype(__p2__)& __p2__, decltype(__p3__)& __p3__)
#define _co_capture5(__p1__, __p2__, __p3__, __p4__) co_context_tag(decltype(__p1__)& __p1__, decltype(__p2__)& __p2__, decltype(__p3__)& __p3__, decltype(__p4__)& __p4__)
#define _co_capture6(__p1__, __p2__, __p3__, __p4__, __p5__) co_context_tag(decltype(__p1__)& __p1__, decltype(__p2__)& __p2__, decltype(__p3__)& __p3__,\
	decltype(__p4__)& __p4__, decltype(__p5__)& __p5__)
#define _co_capture7(__p1__, __p2__, __p3__, __p4__, __p5__, __p6__) co_context_tag(decltype(__p1__)& __p1__, decltype(__p2__)& __p2__, decltype(__p3__)& __p3__,\
	decltype(__p4__)& __p4__, decltype(__p5__)& __p5__, decltype(__p6__)& __p6__)
#define _co_capture(...) _BOND_LR__(_co_capture, _PP_NARG(__pl__, __VA_ARGS__))(__VA_ARGS__)

#define co_end_context_init(__ctx__, __capture__, ...) _co_capture __capture__:__VA_ARGS__{}};\
	if (!gen._ctx){\
	gen._ctx = new co_context_tag __capture__; \
	DEBUG_OPERATION(gen._ctx->__inside = true);}\
	struct co_context_base* __ctx = gen._ctx;\
	struct co_context_tag& __ctx__ = *(struct co_context_tag*)gen._ctx;\
	bool __yieldSign = false;{

#define co_begin }\
	if (!__ctx->__coNext) {__ctx->__coNext = (__COUNTER__+1)/2;}\
	switch(__ctx->__coNext) { case __COUNTER__/2:;

#define co_end } delete gen._ctx; gen._ctx = NULL; return;

#define _co_yield do{\
	assert(__ctx->__inside);\
	DEBUG_OPERATION(__ctx->__inside = false);\
	__ctx->__coNext = (__COUNTER__+1)/2;\
	return; case __COUNTER__/2:__ctx->__coNext=0;\
	if(!__ctx->_sharedSign.empty()){__ctx->_sharedSign=true;__ctx->_sharedSign.reset();}\
	}while (0)

#define co_yield \
	assert(__ctx->__inside);\
	for (__yieldSign = false;;__yieldSign = true)\
	if (__yieldSign) {_co_yield; break;}\
	else

#define co_async \
	gen.curr_strand()->wrap(__co_async_wrap(__ctx, std::bind([](generator_handle& host){\
	struct co_context_base* __ctx = host->_ctx;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->next(); }\
	else { __ctx->__asyncSign = true; };\
	}, gen.shared_from_this())))

#define co_shared_async \
	gen.curr_strand()->wrap(__co_async_wrap(__ctx, std::bind([](generator_handle& host, shared_bool& sign){\
	if (sign) return;\
	struct co_context_base* __ctx = host->_ctx;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->next(); }\
	else { __ctx->__asyncSign = true; };\
	}, gen.shared_from_this(), __make_co_shared_sign(__ctx))))

#define co_async_result(...) _co_async_result(gen.shared_from_this(), __VA_ARGS__)
#define co_shared_async_result(...) _co_shared_async_result(gen.shared_from_this(), __make_co_shared_sign(__ctx), __VA_ARGS__)

#define co_tick do{\
	assert(__ctx->__inside);\
	gen.curr_strand()->next_tick(std::bind([](generator_handle& host){host->next(); }, gen.shared_from_this())); _co_yield;}while(0)

#define _co_await do{\
	assert(__ctx->__inside); if (!__ctx->__asyncSign) { __ctx->__asyncSign = true; _co_yield; }}while (0)

#define co_await \
	assert(__ctx->__inside);\
	for (__yieldSign = false;;__yieldSign = true)\
	if (__yieldSign) {_co_await; break;}\
	else

#define co_next(__host__) do{\
	generator* __gen = __host__.get();\
	__gen->curr_strand()->distribute(std::bind([](generator_handle& host){\
	assert(!host->_ctx->__asyncSign);\
	host->next(); }, std::move(__host__))); }while(0)

#define co_tick_next(__host__) do{\
	generator* __gen = __host__.get();\
	__gen->curr_strand()->next_tick(std::bind([](generator_handle& host){\
	assert(!host->_ctx->__asyncSign);\
	host->next(); }, std::move(__host__))); }while (0)

#define co_async_next(__host__) do{\
	generator* __gen = __host__.get();\
	__gen->curr_strand()->distribute(std::bind([](generator_handle& host){\
	struct co_context_base* __ctx = host->_ctx;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->next(); }\
	else { __ctx->__asyncSign = true; };\
	}, std::move(__host__))); }while (0)

#define co_shared_async_next(__host__, __sign__) do{\
	generator* __gen = __host__.get(); \
	__gen->curr_strand()->distribute(std::bind([](generator_handle& host, shared_bool& sign){\
	if (sign) return;\
	struct co_context_base* __ctx = host->_ctx;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->next(); }\
	else { __ctx->__asyncSign = true; }; \
	}, std::move(__host__), std::move(__sign__))); }while (0)

#define co_invoke_(__handler__) do{\
	assert(__ctx->__inside);\
	gen.curr_strand()->try_tick(std::bind([](generator_handle& gen){gen->next(); }, generator::create(gen.curr_strand(), __handler__, co_async))); _co_await; }while (0)

#define co_invoke(...) co_invoke_(std::bind(__VA_ARGS__))

#define co_sleep(__timer__, __ms__) do{\
	assert(__ctx->__inside); \
	(__timer__)->timeout(__ms__, std::bind([](generator_handle& host){host->next(); }, gen.shared_from_this())); _co_yield; } while (0)

#define co_run(__gen__) do{generator* __gen = __gen__.get(); __gen->curr_strand()->try_tick(std::bind([](generator_handle& host){host->next(); }, std::move(__gen__)));}while(0)

#define co_go(__strand__) CoGo_(__strand__)-

struct co_context_base
{
	virtual ~co_context_base(){}
	shared_bool _sharedSign;
	int __coNext = 0;
	bool __asyncSign = false;
#if (_DEBUG || DEBUG)
	bool __inside = false;
#endif
};

template <typename Handler>
Handler&& __co_async_wrap(struct co_context_base* __ctx, Handler&& handler)
{
	assert(__ctx->__inside);
	return (Handler&&)handler;
}

const shared_bool& __make_co_shared_sign(struct co_context_base* __ctx);

class generator;
typedef std::shared_ptr<generator> generator_handle;

class generator
{
	FRIEND_SHARED_PTR(generator);
private:
	generator();
	~generator(){}
public:
	template <typename SharedStrand, typename Handler>
	static generator_handle create(SharedStrand&& strand, Handler&& handler)
	{
		generator_handle res(new generator());
		res->_weakThis = res;
		res->_strand = std::forward<SharedStrand>(strand);
		res->_handler = std::forward<Handler>(handler);
		return res;
	}

	template <typename SharedStrand, typename Handler, typename Notify>
	static generator_handle create(SharedStrand&& strand, Handler&& handler, Notify&& notify)
	{
		generator_handle res(new generator());
		res->_weakThis = res;
		res->_strand = std::forward<SharedStrand>(strand);
		res->_handler = std::forward<Handler>(handler);
		res->_notify = std::forward<Notify>(notify);
		return res;
	}

	bool next();
	generator_handle shared_from_this();
	const shared_strand& curr_strand();

	co_context_base* _ctx;
private:
	std::weak_ptr<generator> _weakThis;
	std::function<void(generator&)> _handler;
	std::function<void()> _notify;
	shared_strand _strand;
	NONE_COPY(generator);
};

struct CoGo_
{
	template <typename SharedStrand>
	CoGo_(SharedStrand&& strand)
	:_strand(std::forward<SharedStrand>(strand)) {}

	template <typename Handler>
	void operator-(Handler&& handler)
	{
		boost_strand* st = _strand.get();
		st->try_tick(std::bind([](generator_handle& gen){gen->next(); }, generator::create(std::move(_strand), std::forward<Handler>(handler))));
	}

	shared_strand _strand;
};

template <typename... _Types>
struct CoAsyncResult_
{
	CoAsyncResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		co_async_next(_gen);
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAsyncResult_&) = delete;
	RVALUE_CONSTRUCT2(CoAsyncResult_, _gen, _result);
	LVALUE_CONSTRUCT2(CoAsyncResult_, _gen, _result);
};

template <typename... _Types>
struct CoShardAsyncResult_
{
	CoShardAsyncResult_(generator_handle&& gen, const shared_bool& sign, _Types&... result)
	:_gen(std::move(gen)), _sign(sign), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		co_shared_async_next(_gen, _sign);
	}

	generator_handle _gen;
	shared_bool _sign;
	std::tuple<_Types&...> _result;
	void operator=(const CoShardAsyncResult_&) = delete;
	RVALUE_CONSTRUCT3(CoShardAsyncResult_, _gen, _sign, _result);
	LVALUE_CONSTRUCT3(CoShardAsyncResult_, _gen, _sign, _result);
};

template <typename... Args>
CoAsyncResult_<Args...> _co_async_result(generator_handle&& gen, Args&... result)
{
	assert(gen->_ctx->__inside);
	return CoAsyncResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoShardAsyncResult_<Args...> _co_shared_async_result(generator_handle&& gen, const shared_bool& sign, Args&... result)
{
	assert(gen->_ctx->__inside);
	return CoShardAsyncResult_<Args...>(std::move(gen), sign, result...);
}

#endif