#ifndef __GENERATOR_H
#define __GENERATOR_H

#include "my_actor.h"

//在generator函数体内，获取当前generator对象
#define co_self __co_self
//作为generator函数体首个参数
#define co_generator generator& co_self
//在generator上下文初始化中(co_end_context_init)，设置当前不捕获外部变量
#define co_context_no_capture __no_context_capture
struct __co_context_no_capture{};

//开始定义generator函数体上下文，类似于局部变量
#define co_begin_context \
	static_assert(__COUNTER__+1 == __COUNTER__, ""); __co_context_no_capture const co_context_no_capture = __co_context_no_capture();\
	goto __restart; __restart:;\
	struct co_context_tag: public co_context_base {

#define _co_end_context(__ctx__) \
	co_self._lockThis();\
	DEBUG_OPERATION(co_self._ctx->__inside = true);}\
	struct co_context_tag* __ctx = static_cast<co_context_tag*>(co_self._ctx);\
	struct co_context_tag& __ctx__ = *__ctx;\
	int __coNext = 0;\
	size_t __coSwitchTempVal = 0;\
	bool __coSwitchFirstLoopSign = false;\
	bool __coSwitchDefaultSign = false;\
	bool __coSwitchPreSign = false;\
	bool __yieldSwitch = false; {

//结束generator函数体上下文定义
#define co_end_context(__ctx__) };\
	if (!co_self._ctx){	co_self._ctx = new co_context_tag();\
	_co_end_context(__ctx__)

#define _cop(__p__) decltype(__p__)& __p__
#define _co_capture0() co_context_tag()
#define _co_capture1(p1) co_context_tag(_cop(p1))
#define _co_capture2(p1,p2) co_context_tag(_cop(p1),_cop(p2))
#define _co_capture3(p1,p2,p3) co_context_tag(_cop(p1),_cop(p2),_cop(p3))
#define _co_capture4(p1,p2,p3,p4) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4))
#define _co_capture5(p1,p2,p3,p4,p5) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5))
#define _co_capture6(p1,p2,p3,p4,p5,p6) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6))
#define _co_capture7(p1,p2,p3,p4,p5,p6,p7) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7))
#define _co_capture8(p1,p2,p3,p4,p5,p6,p7,p8) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8))
#define _co_capture9(p1,p2,p3,p4,p5,p6,p7,p8,p9) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8),_cop(p9))
#define _co_capture10(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) co_context_tag(_cop(p1),_cop(p2),_cop(p3),_cop(p4),_cop(p5),_cop(p6),_cop(p7),_cop(p8),_cop(p9),_cop(p10))
#define _co_capture(...) _BOND_LR__(_co_capture, MPL_ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__)

//结束generator函数体上下文定义，带内部变量初始化
#define co_end_context_init(__ctx__, __capture__, ...) _co_capture __capture__:__VA_ARGS__{}};\
	if (!co_self._ctx){	co_self._ctx = new co_context_tag __capture__;\
	_co_end_context(__ctx__)

//在启用co_fork时，初始化fork出来的generator状态，默认可以不用，将直接拷贝
#define co_fork_context co_context_tag(co_context_tag& host)

//在generator结束时，做最后状态清理，可以不用
#define co_destroy_context ~co_context_tag()

#define co_no_context co_begin_context; co_end_context(__noctx)

#define co_context_space_size sizeof(co_context_tag)

#define co_end_context_alloc(__alloc__, __ctx__) };\
	if (!co_self._ctx){	co_self._ctx = new(__alloc__)co_context_tag();\
	_co_end_context(__ctx__)

#define co_end_context_alloc_init(__alloc__, __ctx__, __capture__, ...) _co_capture __capture__:__VA_ARGS__{}};\
	if (!co_self._ctx){	co_self._ctx = new(__alloc__)co_context_tag __capture__;\
	_co_end_context(__ctx__)

//开始generator的代码区域
#define co_begin }\
	if (!__ctx->_sharedSign.empty()){__ctx->_sharedSign=true; __ctx->_sharedSign.reset();}\
	if (!__ctx->__coNext) {__ctx->__coNext = (__COUNTER__+1)/2;}else if (-1==__ctx->__coNext) co_stop;\
	__coNext=__ctx->__coNext; __ctx->__coNext=0;\
	switch(__coNext) { case __COUNTER__/2:;

//结束generator的代码区域
#define co_end break;default:assert(false);}\
	goto __stop; __stop:;\
	delete __ctx; co_self._ctx = __ctx = NULL; return;

#define co_end_dealloc(__dealloc__) break;default:assert(false);}\
	goto __stop; __stop:;\
	__ctx->~co_context_tag(); __dealloc__(__ctx); co_self._ctx = __ctx = NULL; return;

#define _co_yield do{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);\
	DEBUG_OPERATION(__ctx->__inside = false);\
	__ctx->__coNext = (__COUNTER__+1)/2;\
	return; case __COUNTER__/2:;\
	}while (0)

//generator的yield原语
#define co_yield \
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	for (__yieldSwitch = false;;__yieldSwitch = true)\
	if (__yieldSwitch) {_co_yield; break;}\
	else

//generator异步回调接口
#define co_async \
	co_self.gen_strand()->wrap(std::bind([](generator_handle& host){\
	assert(host);\
	struct co_context_base* __ctx = host->_ctx;\
	if (!__ctx) return;\
	generator* host_ = host->_revert_this(host);\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host_->_next(); }\
	else { __ctx->__asyncSign = true; };\
	}, std::move(co_self.async_this())))

//generator可共享异步回调接口
#define co_shared_async \
	co_self.gen_strand()->wrap(std::bind([](generator_handle& host, shared_bool& sign){\
	if (sign) return;\
	struct co_context_base* __ctx = host->_ctx;\
	if (!__ctx) return;\
	if (__ctx->__asyncSign) { __ctx->__asyncSign = false; host->_next(); }\
	else { __ctx->__asyncSign = true; };\
	}, co_self.shared_this(), co_self.shared_async_sign()))

#define co_anext \
	co_self.gen_strand()->wrap(std::bind([](generator_handle& host){\
	assert(host);\
	host->_revert_this(host)->_next();\
	}, std::move(co_self.shared_this())))

//带返回值的generator异步回调接口
#define co_async_result(...) _co_async_result(std::move(co_self.async_this()), __VA_ARGS__)
#define co_anext_result(...) _co_anext_result(std::move(co_self.shared_this()), __VA_ARGS__)
//带返回值的generator可共享异步回调接口
#define co_shared_async_result(...) _co_shared_async_result(co_self.shared_this(), co_self.shared_async_sign(), __VA_ARGS__)

//挂起generator，等待调度器下次触发
#define co_tick do{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	co_self.gen_strand()->next_tick(std::bind([](generator_handle& host){\
	if(!host->_ctx)return; host->_revert_this(host)->_next(); }, std::move(co_self.shared_this()))); _co_yield;}while (0)

#define _co_await do{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	assert(__ctx->__awaitSign || __ctx->__sharedAwaitSign);\
	DEBUG_OPERATION(__ctx->__awaitSign = __ctx->__sharedAwaitSign = false);\
	if (!__ctx->__asyncSign) { __ctx->__asyncSign = true; _co_yield; }}while (0)

//generator await原语，与co_async之类使用
#define co_await \
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	for (__yieldSwitch = false;;__yieldSwitch = true)\
	if (__yieldSwitch) {_co_await; break;}\
	else

#define co_next_(__host__) do{\
	__host__->_revert_this(__host__)->_co_next();\
	}while (0)

#define co_next(__host__) do{\
	generator_handle __host = __host__;\
	__host->_revert_this(__host)->_co_next();\
	}while (0)

#define co_tick_next_(__host__) do{\
	__host__->_revert_this(__host__)->_co_tick_next();\
	}while (0)

#define co_tick_next(__host__) do{\
	generator_handle __host = __host__;\
	__host->_revert_this(__host)->_co_tick_next();\
	}while (0)

#define co_async_next_(__host__) do{\
	__host__->_revert_this(__host__)->_co_async_next();\
	}while (0)

#define co_async_next(__host__) do{\
	generator_handle __host = __host__;\
	__host->_revert_this(__host)->_co_async_next();\
	}while (0)

#define co_shared_async_next(__host__, __sign__) do{\
	__host__->_co_shared_async_next(__sign__);\
	}while (0)

#define co_invoke_(__handler__) do{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	generator::create(co_self.gen_strand(), __handler__, co_async)->run(); _co_await;\
	}while (0)

//递归调用另一个generator，直到执行完毕后接着下一行
#define co_invoke(...) co_invoke_(std::bind(__VA_ARGS__))
//sleep，毫秒
#define co_sleep(__timer__, __ms__) do{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	(__timer__)->timeout(__ms__, std::bind([](generator_handle& host){\
	if(!host->_ctx)return; host->_revert_this(host)->_next(); }, std::move(co_self.shared_this()))); _co_yield; }while (0)

//开始运行一个generator
#define co_go(__strand__) CoGo_(__strand__)-
//fork出一个generator，新的generator将在该语句下一行开始执行
#define co_fork do{{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);\
	generator_handle newGen = co_self._begin_fork();\
	newGen->_ctx = new co_context_tag(*__ctx);\
	newGen->_lockThis();\
	newGen->_ctx->clear();\
	newGen->_ctx->__lockStop = __ctx->__lockStop;\
	newGen->_ctx->__coNext = (__COUNTER__+1)/2;\
	newGen->run();\
	}if (0) case __COUNTER__/2:break;\
	}while (0)

#define co_fork_(__new__, ...) do{{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);\
	__new__ = co_self._begin_fork(__VA_ARGS__);\
	__new__->_lockThis();\
	__new__->_ctx = new co_context_tag(*__ctx);\
	__new__->_ctx->clear();\
	__new__->_ctx->__lockStop = __ctx->__lockStop;\
	__new__->_ctx->__coNext = (__COUNTER__+1)/2;\
	__new__->run();\
	}if (0) case __COUNTER__/2:break;\
	}while (0)

#define co_fork_alloc(__alloc__) do{{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);\
	generator_handle newGen = co_self._begin_fork();\
	newGen->_lockThis();\
	newGen->_ctx = new(__alloc__)co_context_tag(*__ctx);\
	newGen->_ctx->clear();\
	newGen->_ctx->__lockStop = __ctx->__lockStop;\
	newGen->_ctx->__coNext = (__COUNTER__+1)/2;\
	newGen->run();\
	}if (0) case __COUNTER__/2:break;\
	}while (0)

#define co_fork_alloc_(__alloc__, __new__, ...) do{{\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__inside);\
	assert(!__ctx->__awaitSign && !__ctx->__sharedAwaitSign);\
	__new__ = co_self._begin_fork(__VA_ARGS__);\
	__new__->_lockThis();\
	__new__->_ctx = new(__alloc__)co_context_tag(*__ctx);\
	__new__->_ctx->clear();\
	__new__->_ctx->__lockStop = __ctx->__lockStop;\
	__new__->_ctx->__coNext = (__COUNTER__+1)/2;\
	__new__->run();\
	}if (0) case __COUNTER__/2:break;\
	}while (0)
//拷贝出一个和当前一样的generator
#define co_clone do{\
	if(-1==__ctx->__coNext) co_stop;\
	co_self._begin_fork()->run();\
	}while (0)

#define co_clone_(__new__, ...) do{\
	if(-1==__ctx->__coNext) co_stop;\
	__new__=co_self._begin_fork(__VA_ARGS__); __new__->run();\
	}while (0)
//重启当前generator，从头开始
#define co_restart do{\
	if(-1==__ctx->__coNext) co_stop;\
	delete __ctx; co_self._ctx = __ctx = NULL; goto __restart;\
	}while (0)

#define co_restart_dealloc(__dealloc__) do{\
	if(-1==__ctx->__coNext) co_stop;\
	__ctx->~co_context_tag(); __dealloc__(__ctx); co_self._ctx = __ctx = NULL; goto __restart;\
	}while (0)
//结束当前generator的运行
#define co_stop do{goto __stop;} while(0)
//锁定外部generator的stop操作
#define co_lock_stop do{\
	assert(__ctx->__inside);\
	if(-1==__ctx->__coNext) co_stop;\
	assert(__ctx->__lockStop<255);\
	__ctx->__lockStop++;\
	}while (0)
//解锁外部generator的stop操作
#define co_unlock_stop do{\
	assert(__ctx->__inside);\
	assert(__ctx->__lockStop);\
	assert(-1!=__ctx->__coNext);\
	if (0==--__ctx->__lockStop && __ctx->__readyQuit) co_stop;\
	}while (0)
//当前generator调度器
#define co_strand co_self.gen_strand()

//发送一个任务到另一个strand中执行，执行完成后接着下一行
#define co_begin_send(__strand__) {if (co_self._co_send(__strand__, [&]{
#define co_end_send }))_co_yield;}
//发送一个任务到另一个strand中执行，执行完成后接着下一行
#define co_begin_async_send(__strand__) {co_self._co_async_send(__strand__, [&]{
#define co_end_async_send });_co_yield;}

#define _case(id, p) case p:goto __co_case_##id##_##p;
#define _case_default(id) default:goto __co_case_##id##_default;
#define _switch_case1(id,p1) _case(id,p1)_case_default(id)
#define _switch_case2(id,p1,p2) _case(id,p1)_case(id,p2)_case_default(id)
#define _switch_case3(id,p1,p2,p3) _case(id,p1)_case(id,p2)_case(id,p3)_case_default(id)
#define _switch_case4(id,p1,p2,p3,p4) _case(id,p1)_case(id,p2)_case(id,p3)_case(id,p4)_case_default(id)
#define _switch_case5(id,p1,p2,p3,p4,p5) _case(id,p1)_case(id,p2)_case(id,p3)_case(id,p4)_case(id,p5)_case_default(id)
#define _switch_case6(id,p1,p2,p3,p4,p5,p6) _case(id,p1)_case(id,p2)_case(id,p3)_case(id,p4)_case(id,p5)_case(id,p6)_case_default(id)
#define _switch_case7(id,p1,p2,p3,p4,p5,p6,p7) _case(id,p1)_case(id,p2)_case(id,p3)_case(id,p4)_case(id,p5)_case(id,p6)_case(id,p7)_case_default(id)
#define _switch_case8(id,p1,p2,p3,p4,p5,p6,p7,p8) _case(id,p1)_case(id,p2)_case(id,p3)_case(id,p4)_case(id,p5)_case(id,p6)_case(id,p7)_case(id,p8)_case_default(id)
#define _switch_case9(id,p1,p2,p3,p4,p5,p6,p7,p8,p9) _case(id,p1)_case(id,p2)_case(id,p3)_case(id,p4)_case(id,p5)_case(id,p6)_case(id,p7)_case(id,p8)_case(id,p9)_case_default(id)
#define _switch_case10(id,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) _case(id,p1)_case(id,p2)_case(id,p3)_case(id,p4)_case(id,p5)_case(id,p6)_case(id,p7)_case(id,p8)_case(id,p9)_case(id,p10)_case_default(id)
#define _switch_case(id, ...) _BOND_LR__(_switch_case, MPL_ARGS_SIZE(__VA_ARGS__))(id, __VA_ARGS__)
#define co_define_int_name(__name__, __val__) enum{__name__=__val__};
//因为generator内部无法在switch-case里面co_yield，提供该宏间接实现switch效果
#define co_begin_switch_ex(id, __val__, ...) do{switch(__val__) {_switch_case(id, __VA_ARGS__)}
#define co_switch_case_ex(id, p) __co_case_##id##_##p:
#define co_switch_default_ex(id) co_switch_case_ex(id, default)
#define co_end_switch_ex }while (0)

//因为generator内部无法在switch-case里面co_yield，提供该宏间接实现switch效果（不支持嵌套）
#define co_begin_switch(__val__) for(__coSwitchPreSign=false,__coSwitchDefaultSign=false,__coSwitchFirstLoopSign=true,__coSwitchTempVal=(size_t)__val__;\
	__coSwitchFirstLoopSign || (!__coSwitchPreSign && __coSwitchDefaultSign);__coSwitchFirstLoopSign=false){if(0){
#define co_switch_case_(__val__) __coSwitchPreSign=true;}if (__coSwitchPreSign || __coSwitchTempVal==(size_t)(__val__)){
#define co_switch_case(__val__) __coSwitchPreSign=true;}if (__coSwitchPreSign || (__coSwitchFirstLoopSign&&__coSwitchTempVal==(size_t)(__val__))){
#define co_switch_default __coSwitchPreSign=true;}if(!__coSwitchDefaultSign && !__coSwitchPreSign){__coSwitchDefaultSign=true;}else{
#define co_end_switch __coSwitchPreSign=true;}}

struct co_context_base
{
#ifdef _MSC_VER
	virtual ~co_context_base(){}//FIXME VC下启用co_destroy_context时，不加这个会有编译错误
#endif
	void clear()
	{
		_sharedSign.reset();
		__coNext = 0;
		__readyQuit = false;
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
	unsigned char __lockStop = 0;
	bool __readyQuit = false;
	bool __asyncSign = false;
#if (_DEBUG || DEBUG)
	bool __inside = false;
	bool __awaitSign = false;
	bool __sharedAwaitSign = false;
	bool __yieldSign = false;
#endif
};

class generator;
typedef std::shared_ptr<generator> generator_handle;

class generator
{
	friend my_actor;
	FRIEND_SHARED_PTR(generator);
private:
	generator();
	~generator();
public:
	template <typename SharedStrand, typename Handler>
	static generator_handle create(SharedStrand&& strand, Handler&& handler)
	{
		mem_alloc_base* genObjRefCountAlloc_ = _genObjRefCountAlloc;
		generator_handle res(new(_genObjAlloc->allocate())generator(), [genObjRefCountAlloc_](generator* p)
		{
			p->~generator();
			genObjRefCountAlloc_->deallocate(p);
		});
		res->_weakThis = res;
		res->_strand = std::forward<SharedStrand>(strand);
		res->_handler = std::forward<Handler>(handler);
		return res;
	}

	template <typename SharedStrand, typename Handler, typename Notify>
	static generator_handle create(SharedStrand&& strand, Handler&& handler, Notify&& notify)
	{
		mem_alloc_base* genObjRefCountAlloc_ = _genObjRefCountAlloc;
		generator_handle res(new(_genObjAlloc->allocate())generator(), [genObjRefCountAlloc_](generator* p)
		{
			p->~generator();
			genObjRefCountAlloc_->deallocate(p);
		});
		res->_weakThis = res;
		res->_strand = std::forward<SharedStrand>(strand);
		res->_handler = std::forward<Handler>(handler);
		res->_notify = std::forward<Notify>(notify);
		return res;
	}

	void run();
	void stop();
	const shared_strand& gen_strand();
	generator_handle& shared_this();
	generator_handle& async_this();
	const shared_bool& shared_async_sign();
public:
	bool _next();
	void _lockThis();
	generator* _revert_this(generator_handle&);
	generator_handle _begin_fork(std::function<void()> notify = std::function<void()>());
	void _co_next();
	void _co_tick_next();
	void _co_async_next();
	void _co_shared_async_next(shared_bool& sign);

	template <typename Handler>
	bool _co_send(const shared_strand& strand, Handler&& handler)
	{
		if (gen_strand() != strand)
		{
			strand->post(std::bind([](generator_handle& gen, Handler& handler)
			{
				CHECK_EXCEPTION(handler);
				generator* const self = gen.get();
				self->gen_strand()->post(std::bind([](generator_handle& gen)
				{
					gen->_revert_this(gen)->_next();
				}, std::move(gen)));
			}, std::move(shared_this()), std::forward<Handler>(handler)));
			return true;
		}
		CHECK_EXCEPTION(handler);
		return false;
	}

	template <typename Handler>
	void _co_async_send(const shared_strand& strand, Handler&& handler)
	{
		strand->try_tick(std::bind([](generator_handle& gen, Handler& handler)
		{
			CHECK_EXCEPTION(handler);
			generator* const self = gen.get();
			self->gen_strand()->distribute(std::bind([](generator_handle& gen)
			{
				gen->_revert_this(gen)->_next();
			}, std::move(gen)));
		}, std::move(shared_this()), std::forward<Handler>(handler)));
	}

	co_context_base* _ctx;
private:
	static void install();
	static void uninstall();
private:
	std::weak_ptr<generator> _weakThis;
	std::shared_ptr<generator> _sharedThis;
	std::function<void(generator&)> _handler;
	std::function<void()> _notify;
	shared_strand _strand;
	DEBUG_OPERATION(bool _isRun);
	NONE_COPY(generator);
	static mem_alloc_mt<generator>* _genObjAlloc;
	static mem_alloc_base* _genObjRefCountAlloc;
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
		res->run();
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
		assert(_gen);
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		_gen->_revert_this(_gen)->_co_async_next();
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
	CoShardAsyncResult_(generator_handle& gen, const shared_bool& sign, _Types&... result)
	:_gen(gen), _sign(sign), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		_gen->_co_shared_async_next(_sign);
	}

	generator_handle _gen;
	shared_bool _sign;
	std::tuple<_Types&...> _result;
	void operator=(const CoShardAsyncResult_&) = delete;
	RVALUE_CONSTRUCT3(CoShardAsyncResult_, _gen, _sign, _result);
	LVALUE_CONSTRUCT3(CoShardAsyncResult_, _gen, _sign, _result);
};

template <typename... _Types>
struct CoAnextResult_
{
	CoAnextResult_(generator_handle&& gen, _Types&... result)
	:_gen(std::move(gen)), _result(result...) {}

	template <typename... Args>
	void operator()(Args&&... args)
	{
		assert(_gen);
		_result = std::tuple<Args&&...>(std::forward<Args>(args)...);
		_gen->_revert_this(_gen)->_next();
	}

	generator_handle _gen;
	std::tuple<_Types&...> _result;
	void operator=(const CoAnextResult_&) = delete;
	RVALUE_CONSTRUCT2(CoAnextResult_, _gen, _result);
	LVALUE_CONSTRUCT2(CoAnextResult_, _gen, _result);
};

template <typename... Args>
CoAsyncResult_<Args...> _co_async_result(generator_handle&& gen, Args&... result)
{
	return CoAsyncResult_<Args...>(std::move(gen), result...);
}

template <typename... Args>
CoShardAsyncResult_<Args...> _co_shared_async_result(generator_handle& gen, const shared_bool& sign, Args&... result)
{
	return CoShardAsyncResult_<Args...>(gen, sign, result...);
}

template <typename... Args>
CoAnextResult_<Args...> _co_anext_result(generator_handle&& gen, Args&... result)
{
	return CoAnextResult_<Args...>(std::move(gen), result...);
}

#endif