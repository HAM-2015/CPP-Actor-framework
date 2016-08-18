#include "bind_node_run.h"

#ifdef ENABLE_UV_ACTOR

cpp_call_js::cpp_call_js(v8::Isolate* isolate, const shared_uv_strand& uvStrand, v8::Local<v8::Value> callback, v8::Local<v8::Value> recv)
:_uvStrand(uvStrand), _callback(isolate, v8::Local<v8::Function>::Cast(callback)), _recv(isolate, recv)
{
	assert(_uvStrand->in_this_ios());
	assert(callback->IsFunction());
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

#endif