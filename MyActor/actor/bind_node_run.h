#ifndef __BIND_NODE_RUN_H
#define __BIND_NODE_RUN_H

#ifdef ENABLE_UV_ACTOR
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include "v8.h"
#include "node.h"
#include "node_object_wrap.h"
#include "my_actor.h"
#include "run_strand.h"
#include "generator.h"

#ifdef _MSC_VER
#pragma comment(lib, "node.lib")
#endif

struct std_buffer
{
	void* data;
	size_t byte_length;
};

template <typename Type>
struct V8CastMatch_;

template <>
struct V8CastMatch_<bool>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, bool v)
	{
		return v8::Boolean::New(isolate, v);
	}
};

template <>
struct V8CastMatch_<int>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, int v)
	{
		return v8::Number::New(isolate, v);
	}
};

template <>
struct V8CastMatch_<unsigned>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, unsigned v)
	{
		return v8::Number::New(isolate, v);
	}
};

template <>
struct V8CastMatch_<long long>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, long long v)
	{
		return v8::Number::New(isolate, (double)v);
	}
};

template <>
struct V8CastMatch_<unsigned long long>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, unsigned long long v)
	{
		return v8::Number::New(isolate, (double)v);
	}
};

template <>
struct V8CastMatch_<double>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, double v)
	{
		return v8::Number::New(isolate, v);
	}
};

template <>
struct V8CastMatch_<float>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, float v)
	{
		return v8::Number::New(isolate, v);
	}
};

template <>
struct V8CastMatch_<char>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, char v)
	{
		char ts[2] = { v, 0 };
		return v8::String::NewFromUtf8(isolate, ts);
	}
};

template <>
struct V8CastMatch_<wchar_t>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, wchar_t v)
	{
		uint16_t ts[2] = { (uint16_t)v, 0 };
		return v8::String::NewFromTwoByte(isolate, ts);
	}
};

template <size_t N, typename T>
struct V8CastMatch_<T[N]>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, T(&v)[N])
	{
		v8::Local<v8::Array> res = v8::Array::New(isolate, N);
		for (size_t i = 0; i < N; i++)
		{
			res->Set(i, V8CastMatch_<RM_CREF(T)>::cast(isolate, v[i]));
		}
		return res;
	}
};

template <size_t N>
struct V8CastMatch_<char[N]>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const char(&v)[N])
	{
		return v8::String::NewFromUtf8(isolate, v);
	}
};

template <size_t N>
struct V8CastMatch_<const char[N]>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const char(&v)[N])
	{
		return v8::String::NewFromUtf8(isolate, v);
	}
};

template <>
struct V8CastMatch_<char*>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const char* v)
	{
		return v8::String::NewFromUtf8(isolate, v);
	}
};

template <>
struct V8CastMatch_<const char*>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const char* v)
	{
		return v8::String::NewFromUtf8(isolate, v);
	}
};

template <size_t N>
struct V8CastMatch_<wchar_t[N]>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const wchar_t(&v)[N])
	{
		return v8::String::NewFromTwoByte(isolate, (uint16_t*)v);
	}
};

template <size_t N>
struct V8CastMatch_<const wchar_t[N]>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const wchar_t(&v)[N])
	{
		return v8::String::NewFromTwoByte(isolate, (uint16_t*)v);
	}
};

template <>
struct V8CastMatch_<wchar_t*>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const wchar_t* v)
	{
		return v8::String::NewFromTwoByte(isolate, (uint16_t*)v);
	}
};

template <>
struct V8CastMatch_<const wchar_t*>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const wchar_t* v)
	{
		return v8::String::NewFromTwoByte(isolate, (uint16_t*)v);
	}
};

template <>
struct V8CastMatch_<std::string>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const std::string& v)
	{
		return v8::String::NewFromUtf8(isolate, v.c_str());
	}
};

template <>
struct V8CastMatch_<std_buffer>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, const std_buffer& v)
	{
		return v8::ArrayBuffer::New(isolate, v.data, v.byte_length);
	}
};

template <typename T>
struct V8CastMatch_<std::vector<T>>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, std::vector<T>& v)
	{
		v8::Local<v8::Array> res = v8::Array::New(isolate, v.size());
		for (size_t i = 0; i < v.size(); i++)
		{
			res->Set(i, V8CastMatch_<RM_CREF(T)>::cast(isolate, v[i]));
		}
		return res;
	}
};

template <typename T>
struct V8CastMatch_<std::list<T>>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, std::list<T>& v)
	{
		v8::Local<v8::Array> res = v8::Array::New(isolate, v.size());
		int i = 0;
		for (auto& ele : v)
		{
			res->Set(i++, V8CastMatch_<RM_CREF(T)>::cast(isolate, ele));
		}
		return res;
	}
};

template <typename K, typename V>
struct V8CastMatch_<std::map<K, V>>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, std::map<K, V>& v)
	{
		v8::Local<v8::Object> res = v8::Object::New(isolate);
		for (auto& ele : v)
		{
			res->Set(V8CastMatch_<RM_CREF(K)>::cast(isolate, ele.first), V8CastMatch_<RM_CREF(V)>::cast(isolate, ele.second));
		}
		return res;
	}
};

template <typename K, typename V>
struct V8CastMatch_<std::multimap<K, V>>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, std::multimap<K, V>& v)
	{
		v8::Local<v8::Object> res = v8::Object::New(isolate);
		for (auto& ele : v)
		{
			res->Set(V8CastMatch_<RM_CREF(K)>::cast(isolate, ele.first), V8CastMatch_<RM_CREF(V)>::cast(isolate, ele.second));
		}
		return res;
	}
};

template <typename V>
struct V8CastMatch_<std::set<V>>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, std::set<V>& v)
	{
		v8::Local<v8::Object> res = v8::Object::New(isolate);
		int i = 0;
		for (auto& ele : v)
		{
			res->Set(i++, V8CastMatch_<RM_CREF(V)>::cast(isolate, ele));
		}
		return res;
	}
};

template <typename V>
struct V8CastMatch_<std::multiset<V>>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, std::multiset<V>& v)
	{
		v8::Local<v8::Object> res = v8::Object::New(isolate);
		int i = 0;
		for (auto& ele : v)
		{
			res->Set(i++, V8CastMatch_<RM_CREF(V)>::cast(isolate, ele));
		}
		return res;
	}
};

template <typename V>
struct V8CastMatch_<v8::Local<V>>
{
	static v8::Local<v8::Value> cast(v8::Isolate* isolate, v8::Local<V>& v)
	{
		return v;
	}
};

template <typename Arg>
v8::Local<v8::Value> std_value_to_v8(v8::Isolate* isolate, Arg&& arg)
{
	return V8CastMatch_<RM_CREF(Arg)>::cast(isolate, std::forward<Arg>(arg));
}
//////////////////////////////////////////////////////////////////////////

template <size_t N>
struct args_to_v8_local
{
	template <typename... Args>
	static void make(v8::Isolate* isolate, v8::Local<v8::Value>* dst, std::tuple<Args...>& src)
	{
		dst[N - 1] = std_value_to_v8(isolate, std::get<N - 1>(src));
		args_to_v8_local<N - 1>::make(isolate, dst, src);
	}
};

template <>
struct args_to_v8_local<0>
{
	template <typename... Args>
	static void make(v8::Isolate* isolate, v8::Local<v8::Value>* dst, std::tuple<Args...>& src) {}
};
//////////////////////////////////////////////////////////////////////////

template <typename Type>
struct v8_local_to_std
{
	template <typename... Args>
	static Type cast(v8::Local<v8::Value> src, Args&&... args)
	{
		return v8_local_to_std<RM_CREF(Type)>::cast(src, std::forward<Args>(args)...);
	}
};

template <>
struct v8_local_to_std<void>
{
	static int cast(v8::Local<v8::Value> src)
	{
		return 0;
	}
};

template <>
struct v8_local_to_std<double>
{
	static double cast(v8::Local<v8::Value> src)
	{
		assert(src->IsNumber());
		return src->NumberValue();
	}
};

template <>
struct v8_local_to_std<int>
{
	static int cast(v8::Local<v8::Value> src)
	{
		assert(src->IsNumber());
		return (int)src->NumberValue();
	}
};

template <>
struct v8_local_to_std<unsigned>
{
	static unsigned cast(v8::Local<v8::Value> src)
	{
		assert(src->IsNumber());
		return (unsigned)src->NumberValue();
	}
};

template <>
struct v8_local_to_std<bool>
{
	static bool cast(v8::Local<v8::Value> src)
	{
		assert(src->IsBoolean());
		return src->BooleanValue();
	}
};

template <>
struct v8_local_to_std<std::string>
{
	static std::string cast(v8::Local<v8::Value> src)
	{
		assert(src->IsString());
		return std::string(*(v8::String::Utf8Value)src->ToString());
	}
};

template <>
struct v8_local_to_std<std_buffer>
{
	static std_buffer cast(v8::Local<v8::Value> src)
	{
		assert(src->IsArrayBuffer());
		v8::Local<v8::ArrayBuffer> v = v8::Local<v8::ArrayBuffer>::Cast(src);
		return std_buffer{ v->GetContents().Data(), v->GetContents().ByteLength() };
	}
};

template <typename T>
struct v8_local_to_std<std::vector<T>>
{
	static std::vector<T> cast(v8::Local<v8::Value> src)
	{
		assert(src->IsArray());
		v8::Local<v8::Array> v = v8::Local<v8::Array>::Cast(src);
		std::vector<T> res(v->Length());
		for (size_t i = 0; i < v->Length(); i++)
		{
			res[i] = v8_local_to_std<RM_CREF(T)>::cast(v->Get(i));
		}
		return res;
	}
};

template <typename T>
struct v8_local_to_std<std::list<T>>
{
	static std::list<T> cast(v8::Local<v8::Value> src)
	{
		assert(src->IsArray());
		v8::Local<v8::Array> v = v8::Local<v8::Array>::Cast(src);
		std::list<T> res;
		for (size_t i = 0; i < v->Length(); i++)
		{
			res.push_back(v8_local_to_std<RM_CREF(T)>::cast(v->Get(i)));
		}
		return res;
	}
};

template <typename T>
struct v8_local_to_std<std::map<int, T>>
{
	static std::map<int, T> cast(v8::Local<v8::Value> src, const int* index, size_t length)
	{
		assert(src->IsObject());
		v8::Local<v8::Object> v = v8::Local<v8::Object>::Cast(src);
		std::map<int, T> res;
		for (size_t i = 0; i < length; i++)
		{
			v8::Local<v8::Value> t = v->Get(index[i]);
			if (!t->IsUndefined())
			{
				res.insert(std::make_pair(index[i], v8_local_to_std<RM_CREF(T)>::cast(t)));
			}
		}
		return res;
	}

	template <size_t N>
	static std::map<int, T> cast(v8::Local<v8::Value> src, const int(&index)[N])
	{
		return cast(src, index, N);
	}

	template <typename... Args>
	static std::map<int, T> cast(v8::Local<v8::Value> src, int first, Args&&... args)
	{
		int index[1 + sizeof...(Args)] = { first, args... };
		return cast(src, index, 1 + sizeof...(Args));
	}
};


template <typename T>
struct v8_local_to_std<std::map<std::string, T>>
{
	static std::map<std::string, T> cast(v8::Local<v8::Value> src, const std::string* index, size_t length)
	{
		assert(src->IsObject());
		v8::Local<v8::Object> v = v8::Local<v8::Object>::Cast(src);
		std::map<std::string, T> res;
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		for (size_t i = 0; i < length; i++)
		{
			v8::Local<v8::Value> t = v->Get(std_value_to_v8(isolate, index[i]));
			if (!t->IsUndefined())
			{
				res.insert(std::make_pair(index[i], v8_local_to_std<RM_CREF(T)>::cast(t)));
			}
		}
		return res;
	}

	static std::map<std::string, T> cast(v8::Local<v8::Value> src, const char* const* index, size_t length)
	{
		assert(src->IsObject());
		v8::Local<v8::Object> v = v8::Local<v8::Object>::Cast(src);
		std::map<std::string, T> res;
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		for (size_t i = 0; i < length; i++)
		{
			v8::Local<v8::Value> t = v->Get(std_value_to_v8(isolate, index[i]));
			if (!t->IsUndefined())
			{
				res.insert(std::make_pair(index[i], v8_local_to_std<RM_CREF(T)>::cast(t)));
			}
		}
		return res;
	}

	template <size_t N>
	static std::map<std::string, T> cast(v8::Local<v8::Value> src, const std::string(&index)[N])
	{
		return cast(src, index, N);
	}

	template <size_t N>
	static std::map<std::string, T> cast(v8::Local<v8::Value> src, const char*(&index)[N])
	{
		return cast(src, index, N);
	}

	template <size_t N>
	static std::map<std::string, T> cast(v8::Local<v8::Value> src, char*(&index)[N])
	{
		return cast(src, index, N);
	}

	template <typename... Args>
	static std::map<std::string, T> cast(v8::Local<v8::Value> src, const char* first, Args&&... args)
	{
		const char* index[1 + sizeof...(Args)] = { first, args... };
		return cast(src, index, 1 + sizeof...(Args));
	}

	template <typename... Args>
	static std::map<std::string, T> cast(v8::Local<v8::Value> src, const std::string& first, Args&&... args)
	{
		const char* index[1 + sizeof...(Args)] = { first.c_str(), args.c_str()... };
		return cast(src, index, 1 + sizeof...(Args));
	}
};

//////////////////////////////////////////////////////////////////////////

#define NODE_CAST_LOCAL_VALUE (v8::Local<v8::Value>)

#define node_set_method(__exports__, __method__) NODE_SET_METHOD(__exports__, #__method__, __method__)

#define BEGIN_NODE_CLASS_INIT(__class_name__)\
	static void Init(v8::Handle<v8::Object> ___exports){ \
	const char* const ___className = #__class_name__; \
	v8::Isolate* const isolate = v8::Isolate::GetCurrent(); \
	v8::HandleScope ___scope(isolate); \
	v8::Local<v8::FunctionTemplate> ___funcTemp = v8::FunctionTemplate::New(isolate, New); \
	___funcTemp->SetClassName(v8::String::NewFromUtf8(isolate, ___className)); \
	___funcTemp->InstanceTemplate()->SetInternalFieldCount(1);

#define node_class_method(__method_name__) NODE_SET_PROTOTYPE_METHOD(___funcTemp, #__method_name__, __method_name__);

#define END_NODE_CLASS_INIT ___exports->Set(v8::String::NewFromUtf8(isolate, ___className), ___funcTemp->GetFunction());}

//////////////////////////////////////////////////////////////////////////

#define BEGIN_NODE_CLASS_NEW(__class_name__)\
	static void New(const v8::FunctionCallbackInfo<v8::Value>& args){ \
	if (args.IsConstructCall()){ \
	v8::Isolate* const isolate = v8::Isolate::GetCurrent(); \
	v8::HandleScope ___scope(isolate); \
	auto ___funcHandler = [&]()->__class_name__*{

#define END_NODE_CLASS_NEW }; \
	auto* const ___obj = ___funcHandler(); \
	assert(___obj); \
	___obj->Wrap(args.This()); \
	args.GetReturnValue().Set(args.This());}}

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct CheckClassFuncResult
{
	template <typename Handler>
	static void invoke(const v8::FunctionCallbackInfo<v8::Value>& args, Handler&& handler)
	{
		args.GetReturnValue().Set(handler());
	}
};

template <>
struct CheckClassFuncResult<void>
{
	template <typename Handler>
	static void invoke(const v8::FunctionCallbackInfo<v8::Value>& args, Handler&& handler)
	{
		handler();
	}
};

#define BEGIN_NODE_CLASS_METHOD(__class_name__, __method_name__) \
	static void __method_name__(const v8::FunctionCallbackInfo<v8::Value>& args){ \
	__class_name__* const this_ = node::ObjectWrap::Unwrap<__class_name__>(args.Holder()); \
	assert(this_); \
	v8::Isolate* const isolate = v8::Isolate::GetCurrent(); \
	v8::HandleScope ___scope(isolate); \
	auto ___funcHandler = [&]{

#define END_NODE_CLASS_METHOD };\
	CheckClassFuncResult<decltype(___funcHandler())>::invoke(args, ___funcHandler);}

//////////////////////////////////////////////////////////////////////////

template <typename Handler>
class NodeJsCallCpp_ : public node::ObjectWrap
{
private:
	template <typename H>
	NodeJsCallCpp_(H&& h):_handler(std::forward<H>(h)) {}
	~NodeJsCallCpp_() {}
public:
	template <typename H>
	static v8::Local<v8::Value> make(v8::Isolate* isolate, H&& h)
	{
		v8::Local<v8::ObjectTemplate> objTemp = v8::ObjectTemplate::New(isolate);
		objTemp->SetInternalFieldCount(1);
		v8::Local<v8::Object> nodeObj = objTemp->NewInstance();
		(new NodeJsCallCpp_(std::forward<H>(h)))->Wrap(nodeObj);
		return nodeObj;
	}

	void invoke(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		_handler(args);
	}
private:
	Handler _handler;
	NONE_COPY(NodeJsCallCpp_);
};

template <typename Handler>
class NodeJsCallCppOnce_;

#if (_DEBUG || DEBUG)
struct CheckNodeJsCallCppOnceDestroy_ : public node::ObjectWrap
{
	template <typename Handler> friend class NodeJsCallCppOnce_;
	CheckNodeJsCallCppOnceDestroy_() :destroyed(false){}
	~CheckNodeJsCallCppOnceDestroy_(){ assert(destroyed); }
	bool destroyed;
};
#endif

template <typename Handler>
class NodeJsCallCppOnce_
{
private:
	template <typename H>
	NodeJsCallCppOnce_(H&& h) :_handler(std::forward<H>(h)) {}
	~NodeJsCallCppOnce_() { DEBUG_OPERATION(_checkDestroy->destroyed = true); }
public:
	template <typename H>
	static v8::Local<v8::Value> make(v8::Isolate* isolate, H&& h)
	{
		v8::Local<v8::ObjectTemplate> objTemp = v8::ObjectTemplate::New(isolate);
		NodeJsCallCppOnce_* newObj = new NodeJsCallCppOnce_(std::forward<H>(h));
#if (_DEBUG || DEBUG)
		objTemp->SetInternalFieldCount(2);
		v8::Local<v8::Object> nodeObj = objTemp->NewInstance();
		newObj->_checkDestroy = new CheckNodeJsCallCppOnceDestroy_;
		newObj->_checkDestroy->Wrap(nodeObj);
		nodeObj->SetInternalField(1, v8::External::New(isolate, newObj));
#else
		objTemp->SetInternalFieldCount(1);
		v8::Local<v8::Object> nodeObj = objTemp->NewInstance();
		nodeObj->SetInternalField(0, v8::External::New(isolate, newObj));
#endif
		return nodeObj;
	}

	void invoke(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		_handler(args);
		delete this;
	}
private:
#if (_DEBUG || DEBUG)
	CheckNodeJsCallCppOnceDestroy_* _checkDestroy;
#endif
	Handler _handler;
	NONE_COPY(NodeJsCallCppOnce_);
};

/*!
@brief 构造一个v8对象，用于nodejs端调用C++端函数（可以不调用或多次调用）
*/
template <typename CastHandler>
static v8::Local<v8::Function> make_js_call_cpp(v8::Isolate* isolate, CastHandler&& castHandler)
{
	typedef NodeJsCallCpp_<RM_CREF(CastHandler)> CastHandler_;
	return v8::Function::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		(node::ObjectWrap::Unwrap<CastHandler_>(args.Data()->ToObject()))->invoke(args);
	}, CastHandler_::make(isolate, std::forward<CastHandler>(castHandler)));
}

/*!
@brief 构造一个v8对象，用于nodejs端调用C++端函数（必须调用，且只能调用一次）
*/
template <typename CastHandler>
static v8::Local<v8::Function> make_js_call_cpp_once(v8::Isolate* isolate, CastHandler&& castHandler)
{
	typedef NodeJsCallCppOnce_<RM_CREF(CastHandler)> CastHandler_;
	return v8::Function::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		v8::Local<v8::Object> nodeObj = args.Data()->ToObject();
#if (_DEBUG || DEBUG)
		assert(!(node::ObjectWrap::Unwrap<CheckNodeJsCallCppOnceDestroy_>(nodeObj))->destroyed);
		static_cast<CastHandler_*>(v8::Local<v8::External>::Cast(nodeObj->GetInternalField(1))->Value())->invoke(args);
#else
		static_cast<CastHandler_*>(v8::Local<v8::External>::Cast(nodeObj->GetInternalField(0))->Value())->invoke(args);
#endif
	}, CastHandler_::make(isolate, std::forward<CastHandler>(castHandler)));
}

//////////////////////////////////////////////////////////////////////////

/*!
@brief nodejs初始化的时候注册，让cpp可以以全局方式调用js
*/
#define NODE_SET_GLOBAL \
	NODE_SET_METHOD(exports, "set_global", [](const v8::FunctionCallbackInfo<v8::Value>& args){\
	js_global::_global = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>(args.GetIsolate(), v8::Local<v8::Function>::Cast(args[0])); });

//////////////////////////////////////////////////////////////////////////

/*!
@brief 构造一个C++调用nodejs端方法的对象，必须在Actor内部使用
*/
class cpp_call_js
{
public:
	struct result_undefined {};
public:
	cpp_call_js();
	cpp_call_js(v8::Isolate* isolate, const shared_uv_strand& uvStrand, v8::Local<v8::Value> callback, v8::Local<v8::Value> recv = v8::Local<v8::Value>());
	cpp_call_js(v8::Isolate* isolate, const shared_uv_strand& uvStrand, const v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>& callback, v8::Local<v8::Value> recv = v8::Local<v8::Value>());
	cpp_call_js(v8::Isolate* isolate, const shared_uv_strand& uvStrand, v8::Local<v8::Value> object, const char* funcName);
	~cpp_call_js();
public:
	void reset();
	bool empty() const;

	template <typename Handler, typename... Args>
	void invoke(my_actor* host, Handler&& resultHandler, Args&&... args) const
	{
		assert(!empty());
		typedef std::tuple<Args&...> tuple_type;
		tuple_type tupleArgs(args...);
		host->send(_uvStrand, [&]()
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[sizeof...(Args) ? sizeof...(Args) : 1];
			args_to_v8_local<sizeof...(Args)>::make(isolate, params, tupleArgs);
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			resultHandler(isolate, v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), sizeof...(Args), params));
		});
	}

	template <typename ResultHandler, typename CastHandler>
	void invoke_cast(my_actor* host, ResultHandler&& resultHandler, CastHandler&& castHandler) const
	{
		assert(!empty());
		host->send(_uvStrand, [&]()
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[16];
			castHandler(isolate, params);
			size_t paramNum = 0;
			while (!params[paramNum].IsEmpty() && paramNum < sizeof(params) / sizeof(v8::Local<v8::Value>))
			{
				paramNum++;
			}
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			resultHandler(isolate, v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), paramNum, params));
		});
	}

	template <typename R = void, typename... Args>
	R invoke_result(my_actor* host, Args&&... args) const
	{
		stack_obj<R> result;
		bool isUndefined = false;
		invoke(host, [&](v8::Isolate* isolate, v8::Local<v8::Value> res)
		{
			isUndefined = res->IsUndefined();
			if (!isUndefined)
			{
				result.create(v8_local_to_std<R>::cast(res));
			}
		}, std::forward<Args>(args)...);
		if (!std::is_same<R, void>::value && isUndefined)
		{
			throw result_undefined();
		}
		return stack_obj_move::move(result);
	}

	template <typename R = void, typename CastHandler>
	R invoke_result_cast(my_actor* host, CastHandler&& castHandler) const
	{
		stack_obj<R> result;
		bool isUndefined = false;
		invoke_cast(host, [&](v8::Isolate* isolate, v8::Local<v8::Value> res)
		{
			isUndefined = res->IsUndefined();
			if (!isUndefined)
			{
				result.create(v8_local_to_std<R>::cast(res));
			}
		}, std::forward<CastHandler>(castHandler));
		if (!std::is_same<R, void>::value && isUndefined)
		{
			throw result_undefined();
		}
		return stack_obj_move::move(result);
	}

	template <typename... Args>
	void operator()(my_actor* host, Args&&... args) const
	{
		invoke_result<void>(host, std::forward<Args>(args)...);
	}
	
	//////////////////////////////////////////////////////////////////////////

	template <typename Handler, typename... Args>
	void async_wait(my_actor* host, Handler&& resultHandler, Args&&... args) const
	{
		assert(!empty());
		typedef std::tuple<Args&...> tuple_type;
		tuple_type tupleArgs(args...);
		my_actor::quit_guard qg(host);
		host->trig([&](trig_once_notifer<>&& ntf)
		{
			_uvStrand->post(std::bind([&](trig_once_notifer<>& ntf)
			{
				v8::Isolate* isolate = v8::Isolate::GetCurrent();
				v8::HandleScope scope(isolate);
				v8::Local<v8::Value> params[sizeof...(Args) + 1];
				args_to_v8_local<sizeof...(Args)>::make(isolate, params, tupleArgs);
				params[sizeof...(Args)] = make_js_call_cpp_once(isolate, std::bind([&](const v8::FunctionCallbackInfo<v8::Value>& args, trig_once_notifer<>& ntf)
				{
					resultHandler(args);
					ntf();
				}, __1, std::move(ntf)));
				v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
				v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), sizeof...(Args)+1, params);
			}, std::move(ntf)));
		});
	}

	template <typename ResultHandler, typename CastHandler>
	void async_wait_cast(my_actor* host, ResultHandler&& resultHandler, CastHandler&& castHandler) const
	{
		assert(!empty());
		my_actor::quit_guard qg(host);
		host->trig([&](trig_once_notifer<>&& ntf)
		{
			_uvStrand->post(std::bind([&](trig_once_notifer<>& ntf)
			{
				v8::Isolate* isolate = v8::Isolate::GetCurrent();
				v8::HandleScope scope(isolate);
				v8::Local<v8::Value> params[16];
				castHandler(isolate, params);
				size_t paramNum = 0;
				while (!params[paramNum].IsEmpty() && paramNum < sizeof(params) / sizeof(v8::Local<v8::Value>) - 1)
				{
					paramNum++;
				}
				params[paramNum] = make_js_call_cpp_once(isolate, std::bind([&](const v8::FunctionCallbackInfo<v8::Value>& args, trig_once_notifer<>& ntf)
				{
					resultHandler(args);
					ntf();
				}, __1, std::move(ntf)));
				v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
				v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), paramNum + 1, params);
			}, std::move(ntf)));
		});
	}

	template <typename R = void, typename... Args>
	R async_wait_result(my_actor* host, Args&&... args) const
	{
		stack_obj<R> result;
		bool isUndefined = false;
		async_wait(host, [&](const v8::FunctionCallbackInfo<v8::Value>& args)
		{
			assert(args.Length());
			v8::Local<v8::Value> res = args[0];
			isUndefined = res->IsUndefined();
			if (!isUndefined)
			{
				result.create(v8_local_to_std<R>::cast(res));
			}
		}, std::forward<Args>(args)...);
		if (!std::is_same<R, void>::value && isUndefined)
		{
			throw result_undefined();
		}
		return stack_obj_move::move(result);
	}

	template <typename R = void, typename CastHandler>
	R async_wait_result_cast(my_actor* host, CastHandler&& castHandler) const
	{
		stack_obj<R> result;
		bool isUndefined = false;
		async_wait_cast(host, [&](const v8::FunctionCallbackInfo<v8::Value>& args)
		{
			assert(args.Length());
			v8::Local<v8::Value> res = args[0];
			isUndefined = res->IsUndefined();
			if (!isUndefined)
			{
				result.create(v8_local_to_std<R>::cast(res));
			}
		}, std::forward<CastHandler>(castHandler));
		if (!std::is_same<R, void>::value && isUndefined)
		{
			throw result_undefined();
		}
		return stack_obj_move::move(result);
	}

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler, typename... Args>
	void co_invoke(co_generator, Handler&& resultHandler, Args&&... args) const
	{
		typedef std::tuple<RM_CREF(Args)...> tuple_type;
		_uvStrand->try_tick(std::bind([this](generator_handle& host, Handler& resultHandler, tuple_type& tupleArgs)
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[sizeof...(Args) ? sizeof...(Args) : 1];
			args_to_v8_local<sizeof...(Args)>::make(isolate, params, tupleArgs);
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			resultHandler(isolate, v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), sizeof...(Args), params));
			host->_revert_this(host)->_co_async_next();
		}, std::move(co_self.async_this()), std::forward<Handler>(resultHandler), tuple_type(std::forward<Args>(args)...)));
	}

	template <typename ResultHandler, typename CastHandler>
	void co_invoke_cast(co_generator, ResultHandler&& resultHandler, CastHandler&& castHandler) const
	{
		_uvStrand->try_tick(std::bind([this](generator_handle& host, ResultHandler& resultHandler, CastHandler& castHandler)
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[16];
			castHandler(isolate, params);
			size_t paramNum = 0;
			while (!params[paramNum].IsEmpty() && paramNum < sizeof(params) / sizeof(v8::Local<v8::Value>))
			{
				paramNum++;
			}
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			resultHandler(isolate, v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), paramNum, params));
			host->_revert_this(host)->_co_async_next();
		}, std::move(co_self.async_this()), std::forward<ResultHandler>(resultHandler), std::forward<CastHandler>(resultHandler)));
	}

	template <typename R = void, typename ResultHandler, typename... Args>
	void co_invoke_result(ResultHandler&& resultHandler, Args&&... args) const
	{
		typedef std::tuple<RM_CREF(Args)...> tuple_type;
		_uvStrand->try_tick(std::bind([this](ResultHandler& resultHandler, tuple_type& tupleArgs)
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[sizeof...(Args) ? sizeof...(Args) : 1];
			args_to_v8_local<sizeof...(Args)>::make(isolate, params, tupleArgs);
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			v8::Local<v8::Value> res = v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), sizeof...(Args), params);
			if (!res->IsUndefined())
			{
				resultHandler(co_async_state::co_async_ok, v8_local_to_std<R>::cast(res));
			}
			else
			{
				resultHandler(co_async_state::co_async_undefined);
			}
		}, std::forward<ResultHandler>(resultHandler), tuple_type(std::forward<Args>(args)...)));
	}

	template <typename R = void, typename ResultHandler, typename CastHandler>
	void co_invoke_result_cast(ResultHandler&& resultHandler, CastHandler&& castHandler) const
	{
		_uvStrand->try_tick(std::bind([this](ResultHandler& resultHandler, CastHandler& castHandler)
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[16];
			castHandler(isolate, params);
			size_t paramNum = 0;
			while (!params[paramNum].IsEmpty() && paramNum < sizeof(params) / sizeof(v8::Local<v8::Value>))
			{
				paramNum++;
			}
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			v8::Local<v8::Value> res = v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), paramNum, params);
			if (!res->IsUndefined())
			{
				resultHandler(co_async_state::co_async_ok, v8_local_to_std<R>::cast(res));
			}
			else
			{
				resultHandler(co_async_state::co_async_undefined);
			}
		}, std::forward<ResultHandler>(resultHandler), std::forward<CastHandler>(castHandler)));
	}

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler, typename... Args>
	void co_async_wait(co_generator, Handler&& resultHandler, Args&&... args) const
	{
		typedef RM_CREF(Handler) Handler_;
		typedef std::tuple<RM_CREF(Args)...> tuple_type;
		_uvStrand->try_tick(std::bind([this](generator_handle& host, Handler_& resultHandler, tuple_type& tupleArgs)
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[sizeof...(Args)+1];
			args_to_v8_local<sizeof...(Args)>::make(isolate, params, tupleArgs);
			params[sizeof...(Args)] = make_js_call_cpp_once(isolate, std::bind([&](generator_handle& host, Handler_& resultHandler, const v8::FunctionCallbackInfo<v8::Value>& args)
			{
				resultHandler(args);
				host->_revert_this(host)->_co_async_next();
			}, std::move(host), std::move(resultHandler), __1));
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), sizeof...(Args)+1, params);
		}, std::move(co_self.async_this()), std::forward<Handler>(resultHandler), tuple_type(std::forward<Args>(args)...)));
	}

	template <typename ResultHandler, typename CastHandler>
	void co_async_wait_cast(co_generator, ResultHandler&& resultHandler, CastHandler&& castHandler) const
	{
		typedef RM_CREF(ResultHandler) ResultHandler_;
		_uvStrand->try_tick(std::bind([this](generator_handle& host, ResultHandler_& resultHandler, CastHandler& castHandler)
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[16];
			castHandler(isolate, params);
			size_t paramNum = 0;
			while (!params[paramNum].IsEmpty() && paramNum < sizeof(params) / sizeof(v8::Local<v8::Value>) - 1)
			{
				paramNum++;
			}
			params[paramNum] = make_js_call_cpp_once(isolate, std::bind([&](generator_handle& host, ResultHandler_& resultHandler, const v8::FunctionCallbackInfo<v8::Value>& args)
			{
				resultHandler(args);
				host->_revert_this(host)->_co_async_next();
			}, std::move(host), std::move(resultHandler), __1));
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), paramNum + 1, params);
		}, std::move(co_self.async_this()), std::forward<ResultHandler>(resultHandler), std::forward<CastHandler>(castHandler)));
	}

	template <typename R = void, typename ResultHandler, typename... Args>
	void co_async_wait_result(ResultHandler&& resultHandler, Args&&... args) const
	{
		typedef RM_CREF(ResultHandler) ResultHandler_;
		typedef std::tuple<RM_CREF(Args)...> tuple_type;
		_uvStrand->try_tick(std::bind([this](ResultHandler_& resultHandler, tuple_type& tupleArgs)
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[sizeof...(Args)+1];
			args_to_v8_local<sizeof...(Args)>::make(isolate, params, tupleArgs);
			params[sizeof...(Args)] = make_js_call_cpp_once(isolate, std::bind([&](ResultHandler_& resultHandler, const v8::FunctionCallbackInfo<v8::Value>& args)
			{
				assert(args.Length());
				v8::Local<v8::Value> res = args[0];
				if (!res->IsUndefined())
				{
					resultHandler(co_async_state::co_async_ok, v8_local_to_std<R>::cast(res));
				}
				else
				{
					resultHandler(co_async_state::co_async_undefined);
				}
			}, std::move(resultHandler), __1));
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), sizeof...(Args)+1, params);
		}, std::forward<ResultHandler>(resultHandler), tuple_type(std::forward<Args>(args)...)));
	}

	template <typename R = void, typename ResultHandler, typename CastHandler>
	void co_async_wait_result_cast(ResultHandler&& resultHandler, CastHandler&& castHandler) const
	{
		typedef RM_CREF(ResultHandler) ResultHandler_;
		_uvStrand->try_tick(std::bind([this](ResultHandler_& resultHandler, CastHandler& castHandler)
		{
			v8::Isolate* isolate = v8::Isolate::GetCurrent();
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> params[16];
			castHandler(isolate, params);
			size_t paramNum = 0;
			while (!params[paramNum].IsEmpty() && paramNum < sizeof(params) / sizeof(v8::Local<v8::Value>) - 1)
			{
				paramNum++;
			}
			params[paramNum] = make_js_call_cpp_once(isolate, std::bind([&](ResultHandler_& resultHandler, const v8::FunctionCallbackInfo<v8::Value>& args)
			{
				assert(args.Length());
				v8::Local<v8::Value> res = args[0];
				if (!res->IsUndefined())
				{
					resultHandler(co_async_state::co_async_ok, v8_local_to_std<R>::cast(res));
				}
				else
				{
					resultHandler(co_async_state::co_async_undefined);
				}
			}, std::move(resultHandler), __1));
			v8::Local<v8::Value> recv = v8::Local<v8::Value>::New(isolate, _recv);
			v8::Local<v8::Function>::New(isolate, _callback)->Call(!recv.IsEmpty() ? recv : (v8::Local<v8::Value>)isolate->GetCurrentContext()->Global(), paramNum + 1, params);
		}, std::forward<ResultHandler>(resultHandler), std::forward<CastHandler>(castHandler)));
	}
private:
	shared_uv_strand _uvStrand;
	v8::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>> _recv;
	v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> _callback;
	RVALUE_COPY_CONSTRUCTION3(cpp_call_js, _uvStrand, _recv, _callback);
};
//////////////////////////////////////////////////////////////////////////

struct js_global
{
	static void install(v8::Isolate* isolate, const shared_uv_strand& strand);
	static void uninstall();
	static v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> _global;
	static cpp_call_js global;
};

#endif

#endif