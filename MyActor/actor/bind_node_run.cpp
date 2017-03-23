#include "bind_node_run.h"

#ifdef ENABLE_UV_ACTOR

cpp_call_js::cpp_call_js()
{
}

cpp_call_js::cpp_call_js(v8::Isolate* isolate, const shared_uv_strand& uvStrand, v8::Local<v8::Value> callback, v8::Local<v8::Value> recv)
:_uvStrand(uvStrand), _callback(isolate, v8::Local<v8::Function>::Cast(callback)), _recv(isolate, recv)
{
	assert(_uvStrand->in_this_ios());
	assert(callback->IsFunction());
}

cpp_call_js::cpp_call_js(v8::Isolate* isolate, const shared_uv_strand& uvStrand, const v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>& callback, v8::Local<v8::Value> recv)
:_uvStrand(uvStrand), _callback(callback), _recv(isolate, recv)
{
	assert(_uvStrand->in_this_ios());
	assert(!callback.IsEmpty());
}

cpp_call_js::cpp_call_js(v8::Isolate* isolate, const shared_uv_strand& uvStrand, v8::Local<v8::Value> object, const char* funcName)
:cpp_call_js(isolate, uvStrand, object->ToObject()->Get(v8::String::NewFromUtf8(isolate, funcName)), object) {}

cpp_call_js::~cpp_call_js()
{
	reset();
}

void cpp_call_js::reset()
{
	_uvStrand.reset();
	_callback.Reset();
}

bool cpp_call_js::empty() const
{
	return !_uvStrand;
}
//////////////////////////////////////////////////////////////////////////

js_steady_timer* js_steady_timer::_jsTimer = NULL;

void js_steady_timer::init(v8::Handle<v8::Object> exports)
{
	NODE_SET_METHOD(exports, "set_timeout", js_steady_timer::set_timeout);
	NODE_SET_METHOD(exports, "set_deadline", js_steady_timer::set_deadline);
	NODE_SET_METHOD(exports, "set_interval", js_steady_timer::set_interval);
	NODE_SET_METHOD(exports, "clear_timer", js_steady_timer::clear_timer);
}

void js_steady_timer::install(v8::Isolate* isolate, const shared_uv_strand& strand)
{
	_jsTimer = new js_steady_timer();
	_jsTimer->_strand = strand.get();
}

void js_steady_timer::uninstall()
{
	delete _jsTimer;
	_jsTimer = NULL;
}

js_steady_timer::timer_handle* js_steady_timer::timer_handle::make(v8::Local<v8::Object>& v8Obj)
{
	js_steady_timer::timer_handle* timerHandle = new js_steady_timer::timer_handle();
	timerHandle->Wrap(v8Obj);
	return timerHandle;
}

void js_steady_timer::set_timeout(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	assert(2 == args.Length());
	uv_strand::hold_stack uh(_jsTimer->_strand);
	typedef v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>> v8_handle_type;
	typedef v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> v8_callback_type;
	v8::Local<v8::ObjectTemplate> v8timerHandleTemp = v8::ObjectTemplate::New();
	v8timerHandleTemp->SetInternalFieldCount(1);
	v8::Local<v8::Object> v8timerHandle = v8timerHandleTemp->NewInstance();
	js_steady_timer::timer_handle* timerHandle = js_steady_timer::timer_handle::make(v8timerHandle);
	int ms = v8_local_to_std<int>::cast(args[0]);
	_jsTimer->_strand->over_timer()->timeout(ms, timerHandle->_handler, std::bind([](v8_handle_type& v8Handle, v8_callback_type& callback)
	{
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		v8::HandleScope scope(isolate);
		v8::Local<v8::Function>::New(isolate, callback)->Call((v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), 0, NULL);
	}, v8_handle_type(args.GetIsolate(), v8timerHandle), v8_callback_type(args.GetIsolate(), v8::Local<v8::Function>::Cast(args[1]))));
	args.GetReturnValue().Set(v8timerHandle);
}

void js_steady_timer::set_deadline(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	assert(2 == args.Length());
	uv_strand::hold_stack uh(_jsTimer->_strand);
	typedef v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>> v8_handle_type;
	typedef v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> v8_callback_type;
	v8::Local<v8::ObjectTemplate> v8timerHandleTemp = v8::ObjectTemplate::New();
	v8timerHandleTemp->SetInternalFieldCount(1);
	v8::Local<v8::Object> v8timerHandle = v8timerHandleTemp->NewInstance();
	js_steady_timer::timer_handle* timerHandle = js_steady_timer::timer_handle::make(v8timerHandle);
	int ms = v8_local_to_std<int>::cast(args[0]);
	_jsTimer->_strand->over_timer()->deadline((long long)ms * 1000, timerHandle->_handler, std::bind([](v8_handle_type& v8Handle, v8_callback_type& callback)
	{
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		v8::HandleScope scope(isolate);
		v8::Local<v8::Function>::New(isolate, callback)->Call((v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), 0, NULL);
	}, v8_handle_type(args.GetIsolate(), v8timerHandle), v8_callback_type(args.GetIsolate(), v8::Local<v8::Function>::Cast(args[1]))));
	args.GetReturnValue().Set(v8timerHandle);
}

void js_steady_timer::set_interval(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	assert(2 == args.Length());
	uv_strand::hold_stack uh(_jsTimer->_strand);
	typedef v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>> v8_handle_type;
	typedef v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> v8_callback_type;
	v8::Local<v8::ObjectTemplate> v8timerHandleTemp = v8::ObjectTemplate::New();
	v8timerHandleTemp->SetInternalFieldCount(1);
	v8::Local<v8::Object> v8timerHandle = v8timerHandleTemp->NewInstance();
	js_steady_timer::timer_handle* timerHandle = js_steady_timer::timer_handle::make(v8timerHandle);
	int ms = v8_local_to_std<int>::cast(args[0]);
	_jsTimer->_strand->over_timer()->interval(ms, timerHandle->_handler, std::bind([](v8_handle_type& v8Handle, v8_callback_type& callback)
	{
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		v8::HandleScope scope(isolate);
		v8::Local<v8::Function>::New(isolate, callback)->Call((v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), 0, NULL);
	}, v8_handle_type(args.GetIsolate(), v8timerHandle), v8_callback_type(args.GetIsolate(), v8::Local<v8::Function>::Cast(args[1]))));
	args.GetReturnValue().Set(v8timerHandle);
}

void js_steady_timer::clear_timer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	assert(1 == args.Length());
	uv_strand::hold_stack uh(_jsTimer->_strand);
	_jsTimer->_strand->over_timer()->cancel(node::ObjectWrap::Unwrap<js_steady_timer::timer_handle>(v8::Local<v8::Object>::Cast(args[0]))->_handler);
}
//////////////////////////////////////////////////////////////////////////

v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> js_global::_global;
cpp_call_js js_global::global;

void js_global::install(v8::Isolate* isolate, const shared_uv_strand& strand)
{
	if (!_global.IsEmpty())
	{
		global = cpp_call_js(isolate, strand, _global);
	}
}

void js_global::uninstall()
{
	global.reset();
	_global.Reset();
}

#endif