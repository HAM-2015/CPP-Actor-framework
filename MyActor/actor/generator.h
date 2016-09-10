#ifndef __GENERATOR_H
#define __GENERATOR_H

#include "my_actor.h"

#define co_generator generator& gen

#define co_begin_context \
	static_assert(__COUNTER__+1 == __COUNTER__, ""); int const no_capture = 0;\
	if (false) {__restart: goto __break1; goto __restart; __break1:;}\
	struct co_context_tag: public co_context_base {

#define _co_end_context(__ctx__) \
	DEBUG_OPERATION(gen._ctx->__inside = true);}\
	struct co_context_tag* __ctx = static_cast<co_context_tag*>(gen._ctx);\
	struct co_context_tag& __ctx__ = *__ctx;\
	bool __yieldSwitch = false; {

#define co_end_context(__ctx__) };\
	if (!gen._ctx){	gen._ctx = new co_context_tag();\
	_co_end_context(__ctx__)

#define _cop(__p__) decltype(__p__)& __p__
#define _co_capture1() co_context_tag()
#define _co_capture2(p1) co_context_tag(_cop(p1))
#define _co_capture3(p1,p2) co_context_tag(_cop(p1),_cop(p2))
#define _co_capture4(p1,p2,p3) co_context_tag(_cop(p1),_cop(p2),_cop(p3))
#define _co_capture5(p1,p2,p3,p4) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4))
#define _co_capture6(p1,p2,p3,p4,p5) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5))
#define _co_capture7(p1,p2,p3,p4,p5,p6) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6))
#define _co_capture8(p1,p2,p3,p4,p5,p6,p7) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7))
#define _co_capture9(p1,p2,p3,p4,p5,p6,p7,p8) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8))
#define _co_capture10(p1,p2,p3,p4,p5,p6,p7,p8,p9) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8),_cop(p9))
#define _co_capture11(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8),_cop(p9),_cop(p10))
#define _co_capture(...) _BOND_LR__(_co_capture, _PP_NARG(__pl__, __VA_ARGS__))(__VA_ARGS__)

#define co_end_context_init(__ctx__, __capture__, ...) _co_capture __capture__:__VA_ARGS__{}};\
	if (!gen._ctx){	gen._ctx = new co_context_tag __capture__; \
	_co_end_context(__ctx__)

#define co_fork_context co_context_tag(co_context_tag& curr)

#define co_destroy_context ~co_context_tag()

#define co_no_context co_begin_context; co_end_context(__noctx)

#define co_context_space_size sizeof(co_context_tag)

#define co_end_context_alloc(__alloc__, __ctx__) };\
	if (!gen._ctx){	gen._ctx = new(__alloc__)co_context_tag(); \
	_co_end_context(__ctx__)

#define co_end_context_alloc_init(__alloc__, __ctx__, __capture__, ...) _co_capture __capture__:__VA_ARGS__{}};\
	if (!gen._ctx){	gen._ctx = new(__alloc__)co_context_tag __capture__; \
	_co_end_context(__ctx__)

#define co_begin }\
	if (!__ctx->__coNext) {__ctx->__coNext = (__COUNTER__+1)/2;}else if (-1==__ctx->__coNext) co_stop;\
	try {switch(__ctx->__coNext) { case __COUNTER__/2:;

#define co_end }\
	}catch(generator::stop_exception&){}\
	if (false) {__stop: goto __break2; goto __stop; __break2:;}\
	delete __ctx; gen._ctx = __ctx = NULL; return;

#define co_end_dealloc(__dealloc__) }\
	}catch(generator::stop_exception&){}\
	if (false) {__stop: goto __break2; goto __stop; __break2:;}\
	__ctx->~co_context_tag(); __dealloc__(__ctx); gen._ctx = __ctx = NULL; return;

#define _co_yield do{\
	assert(__ctx->__inside);\
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);\
	DEBUG_OPERATION(__ctx->__inside = false);\
	__ctx->__coNext = (__COUNTER__+1)/2;\
	return; case __COUNTER__/2:__ctx->__coNext=0;\
	if(!__ctx->_sharedSign.empty()){__ctx->_sharedSign=true;__ctx->_sharedSign.reset();}\
	}while (0)

#define co_yield \
	assert(__ctx->__inside);\
	for (__yieldSwitch = false;;__yieldSwitch = true)\
	if (__yieldSwitch) {_co_yield; break;}\
	else

#define co_async \
	gen.curr_strand()->wrap(__co_async_wrap(__ctx, std::bind([](generator_handle& host){\
	struct co_context_base* __ctx = host->_ctx;\
	if (!__ctx) return;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->next(); }\
	else { __ctx->__asyncSign = true; };\
	}, gen.shared_from_this())))

#define co_shared_async \
	gen.curr_strand()->wrap(__co_shared_async_wrap(__ctx, std::bind([](generator_handle& host, shared_bool& sign){\
	if (sign) return;\
	struct co_context_base* __ctx = host->_ctx;\
	if (!__ctx) return;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->next(); }\
	else { __ctx->__asyncSign = true; };\
	}, gen.shared_from_this(), __make_co_shared_sign(__ctx))))

#define co_async_result(...) _co_async_result(gen.shared_from_this(), __VA_ARGS__)
#define co_shared_async_result(...) _co_shared_async_result(gen.shared_from_this(), __make_co_shared_sign(__ctx), __VA_ARGS__)

#define co_tick do{\
	assert(__ctx->__inside);\
	gen.curr_strand()->next_tick(std::bind([](generator_handle& host){host->next(); }, gen.shared_from_this())); _co_yield;}while(0)

#define _co_await do{\
	assert(__ctx->__inside);\
	assert(__ctx->__awaitSign || __ctx->__sharedAwaitSign);\
	DEBUG_OPERATION(__ctx->__awaitSign = __ctx->__sharedAwaitSign = false); \
	if (!__ctx->__asyncSign) { __ctx->__asyncSign = true; _co_yield; }}while (0)

#define co_await \
	assert(__ctx->__inside);\
	for (__yieldSwitch = false;;__yieldSwitch = true)\
	if (__yieldSwitch) {_co_await; break;}\
	else

#define co_next(__host__) do{\
	generator_handle& __host = __host__;\
	generator* __gen = __host.get(); \
	__gen->curr_strand()->distribute(std::bind([](generator_handle& host){\
	if (!host->__ctx) return;\
	assert(!host->_ctx->__asyncSign);\
	host->next(); }, std::move(__host))); }while(0)

#define co_tick_next(__host__) do{\
	generator_handle& __host = __host__;\
	generator* __gen = __host.get(); \
	__gen->curr_strand()->next_tick(std::bind([](generator_handle& host){\
	if (!host->__ctx) return;\
	assert(!host->_ctx->__asyncSign);\
	host->next(); }, std::move(__host))); }while (0)

#define co_async_next(__host__) do{\
	generator_handle& __host = __host__;\
	generator* __gen = __host.get(); \
	__gen->curr_strand()->distribute(std::bind([](generator_handle& host){\
	struct co_context_base* __ctx = host->_ctx;\
	if (!__ctx) return;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->next(); }\
	else { __ctx->__asyncSign = true; };\
	}, std::move(__host))); }while (0)

#define co_shared_async_next(__host__, __sign__) do{\
	generator_handle& __host = __host__;\
	generator* __gen = __host.get(); \
	__gen->curr_strand()->distribute(std::bind([](generator_handle& host, shared_bool& sign){\
	if (sign) return;\
	struct co_context_base* __ctx = host->_ctx;\
	if (!__ctx) return;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->next(); }\
	else { __ctx->__asyncSign = true; }; \
	}, std::move(__host), std::move(__sign__))); }while (0)

#define co_invoke_(__handler__) do{\
	assert(__ctx->__inside);\
	gen.curr_strand()->try_tick(std::bind([](generator_handle& gen){gen->next(); }, generator::create(gen.curr_strand(), __handler__, co_async))); _co_await; }while (0)

#define co_invoke(...) co_invoke_(std::bind(__VA_ARGS__))

#define co_sleep(__timer__, __ms__) do{\
	assert(__ctx->__inside); \
	(__timer__)->timeout(__ms__, std::bind([](generator_handle& host){host->next(); }, gen.shared_from_this())); _co_yield; } while (0)

#define co_run(__gen__) do{\
	generator_handle __gen = std::move(__gen__);\
	boost_strand* __strand = __gen->curr_strand().get();\
	__strand->try_tick(std::bind([](generator_handle& host){host->next(); }, std::move(__gen)));}while(0)

#define co_go(__strand__) CoGo_(__strand__)-

#define co_fork do{{\
	assert(__ctx->__inside);\
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);\
	generator_handle newGen = gen.begin_fork();\
	newGen->_ctx = new co_context_tag(*__ctx); \
	newGen->_ctx->clear();\
	newGen->_ctx->__coNext = (__COUNTER__+1)/2;\
	co_run(newGen);\
	}if (false) case __COUNTER__/2:break;\
	}while (0)

#define co_fork_alloc(__alloc__) do{{\
	assert(__ctx->__inside);\
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);\
	generator_handle newGen = gen.begin_fork();\
	newGen->_ctx = new(__alloc__)co_context_tag(*__ctx); \
	newGen->_ctx->clear();\
	newGen->_ctx->__coNext = (__COUNTER__+1)/2;\
	co_run(newGen);\
	}if (false) case __COUNTER__/2:break;\
	}while (0)

#define co_clone do{\
	co_run(gen.begin_fork());\
	}while (0)

#define co_restart do{\
	delete gen._ctx; gen._ctx = NULL; goto __restart; \
	}while (0)

#define co_stop do{goto __stop;} while(0)

struct co_context_base
{
#ifdef _MSC_VER
	virtual ~co_context_base(){}//FIXME VC下不加这个会有编译错误
#endif
	void clear()
	{
		_sharedSign.reset();
		__coNext = 0;
		__asyncSign = false;
#if (_DEBUG || DEBUG)
		__inside = false;
		__awaitSign = false;
		__sharedAwaitSign = false;
		__yieldSign = false;
#endif
	}
	shared_bool _sharedSign;
	int __coNext = 0;
	bool __asyncSign = false;
#if (_DEBUG || DEBUG)
	bool __inside = false;
	bool __awaitSign = false;
	bool __sharedAwaitSign = false;
	bool __yieldSign = false;
#endif
};

template <typename Handler>
Handler&& __co_async_wrap(struct co_context_base* __ctx, Handler&& handler)
{
	assert(__ctx->__inside);
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);
	DEBUG_OPERATION(__ctx->__awaitSign = true);
	return (Handler&&)handler;
}

template <typename Handler>
Handler&& __co_shared_async_wrap(struct co_context_base* __ctx, Handler&& handler)
{
	assert(__ctx->__inside);
	assert(!__ctx->__awaitSign);
	DEBUG_OPERATION(__ctx->__sharedAwaitSign = true);
	return (Handler&&)handler;
}

const shared_bool& __make_co_shared_sign(struct co_context_base* __ctx);

class generator;
typedef std::shared_ptr<generator> generator_handle;

class generator
{
	FRIEND_SHARED_PTR(generator);
public:
	struct stop_exception{};
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
	void stop();
	generator_handle shared_from_this();
	const shared_strand& curr_strand();
	generator_handle begin_fork();

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
	generator_handle operator-(Handler&& handler)
	{
		generator_handle res = generator::create(std::move(_strand), std::forward<Handler>(handler));
		res->curr_strand()->try_tick(std::bind([](generator_handle& gen){gen->next(); }, res));
		return res;
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
	assert(!gen->_ctx->__awaitSign && !gen->_ctx->__sharedAwaitSign);
	DEBUG_OPERATION(gen->_ctx->__awaitSign = true);
	return CoAsyncResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoShardAsyncResult_<Args...> _co_shared_async_result(generator_handle&& gen, const shared_bool& sign, Args&... result)
{
	assert(gen->_ctx->__inside);
	assert(!gen->_ctx->__awaitSign);
	DEBUG_OPERATION(gen->_ctx->__sharedAwaitSign = true);
	return CoShardAsyncResult_<Args...>(std::move(gen), sign, result...);
}

#endif