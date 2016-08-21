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