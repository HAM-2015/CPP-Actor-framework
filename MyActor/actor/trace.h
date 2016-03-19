#ifndef __TRACE_H
#define __TRACE_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <iostream> 
#include "try_move.h"

template <typename T>
struct TraceMatch_ 
{
	template <typename TP>
	static void trace(std::wostream& out, TP&& s)
	{
		out << s;
	}
};

template <>
struct TraceMatch_<bool> 
{
	static void trace(std::wostream& out, bool b)
	{
		out << (b ? "true" : "false");
	}
};

template <size_t N, typename T>
struct TraceMatch_<T[N]>
{
	static void trace(std::wostream& out, const T(&s)[N])
	{
		out << "[";
		for (int i = 0; i < (int)N - 1; i++)
		{
			TraceMatch_<T>::trace(out, s[i]);
			out << ", ";
		}
		if (N)
		{
			TraceMatch_<T>::trace(out, s[N - 1]);
		}
		out << "]";
	}
};

template <size_t N>
struct TraceMatch_<char[N]>
{
	static void trace(std::wostream& out, const char(&s)[N])
	{
		out << s;
	}
};

template <size_t N>
struct TraceMatch_<wchar_t[N]>
{
	static void trace(std::wostream& out, const wchar_t(&s)[N])
	{
		out << s;
	}
};

template <>
struct TraceMatch_<std::string>
{
	static void trace(std::wostream& out, const std::string& s)
	{
		out << s.c_str();
	}
};

template <>
struct TraceMatch_<char*>
{
	static void trace(std::wostream& out, const char* s)
	{
		out << s;
	}
};

template <>
struct TraceMatch_<wchar_t*>
{
	static void trace(std::wostream& out, const wchar_t* s)
	{
		out << s;
	}
};

template <typename T>
struct TraceMatch_<std::vector<T>>
{
	static void trace(std::wostream& out, const std::vector<T>& s)
	{
		out << "[";
		for (int i = 0; i < (int)s.size() - 1; i++)
		{
			TraceMatch_<RM_CREF(T)>::trace(out, s[i]);
			out << ", ";
		}
		if (!s.empty())
		{
			TraceMatch_<RM_CREF(T)>::trace(out, s.back());
		}
		out << "]";
	}
};

template <typename T>
struct TraceMatch_<std::list<T>>
{
	static void trace(std::wostream& out, const std::list<T>& s)
	{
		out << "{";
		int l = (int)s.size();
		auto it = s.begin();
		for (int i = 0; i < l - 1; i++, it++)
		{
			TraceMatch_<RM_CREF(T)>::trace(out, *it);
			out << "->";
		}
		if (l)
		{
			TraceMatch_<RM_CREF(T)>::trace(out, *it);
		}
		out << "}";
	}
};

template <typename K, typename V>
struct TraceMatch_<std::map<K, V>>
{
	static void trace(std::wostream& out, const std::map<K, V>& s)
	{
		out << "{";
		int l = (int)s.size();
		auto it = s.begin();
		for (int i = 0; i < l - 1; i++, it++)
		{
			TraceMatch_<RM_CREF(K)>::trace(out, it->first);
			out << ": ";
			TraceMatch_<RM_CREF(V)>::trace(out, it->second);
			out << ", ";
		}
		if (l)
		{
			TraceMatch_<RM_CREF(K)>::trace(out, it->first);
			out << ": ";
			TraceMatch_<RM_CREF(V)>::trace(out, it->second);
		}
		out << "}";
	}
};

template <typename T>
struct TraceMatch_<std::set<T>>
{
	static void trace(std::wostream& out, const std::set<T>& s)
	{
		out << "{";
		int l = (int)s.size();
		auto it = s.begin();
		for (int i = 0; i < l - 1; i++, it++)
		{
			TraceMatch_<RM_CREF(T)>::trace(out, *it);
			out << ", ";
		}
		if (l)
		{
			TraceMatch_<RM_CREF(T)>::trace(out, *it);
		}
		out << "}";
	}
};

template <typename First>
void _trace(std::wostream& out, First&& fst)
{
	TraceMatch_<RM_CREF(First)>::trace(out, fst);
}

template <typename First>
void _trace_space(std::wostream& out, First&& fst)
{
	TraceMatch_<RM_CREF(First)>::trace(out, fst);
}

template <typename First>
void _trace_comma(std::wostream& out, First&& fst)
{
	TraceMatch_<RM_CREF(First)>::trace(out, fst);
}

template <typename First, typename... Args>
void _trace(std::wostream& out, First&& fst, Args&&... args)
{
	static_assert(sizeof...(Args) > 0, "");
	TraceMatch_<RM_CREF(First)>::trace(out, fst);
	_trace(out, TRY_MOVE(args)...);
}

template <typename First, typename... Args>
void _trace_space(std::wostream& out, First&& fst, Args&&... args)
{
	static_assert(sizeof...(Args) > 0, "");
	TraceMatch_<RM_CREF(First)>::trace(out, fst);
	out << " ";
	_trace_space(out, TRY_MOVE(args)...);
}

template <typename First, typename... Args>
void _trace_comma(std::wostream& out, First&& fst, Args&&... args)
{
	static_assert(sizeof...(Args) > 0, "");
	TraceMatch_<RM_CREF(First)>::trace(out, fst);
	out << ", ";
	_trace_comma(out, TRY_MOVE(args)...);
}

struct TraceMutex_
{
	TraceMutex_()
	{
		_mutex->lock();
	}

	~TraceMutex_()
	{
		_mutex->unlock();
	}

	static std::mutex* _mutex;
};

typedef std::wostringstream _Tracestream;

template <typename... Args> void trace(Args&&... args) { _Tracestream oss; _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::flush; } }
template <typename... Args> void trace_line(Args&&... args) { _Tracestream oss; _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void trace_space(Args&&... args) { _Tracestream oss; _trace_space(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void trace_comma(Args&&... args) { _Tracestream oss; _trace_comma(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }

#if (_DEBUG || DEBUG)
template <typename... Args> void debug_trace(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " DEBUG:   "); _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::flush; } }
template <typename... Args> void debug_trace_line(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " DEBUG:   "); _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void debug_trace_space(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " DEBUG:   "); _trace_space(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void debug_trace_comma(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " DEBUG:   "); _trace_comma(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
#else
template <typename... Args> void debug_trace(Args&&... args) {}
template <typename... Args> void debug_trace_line(Args&&... args) {}
template <typename... Args> void debug_trace_space(Args&&... args) {}
template <typename... Args> void debug_trace_comma(Args&&... args) {}
#endif

template <typename... Args> void info_trace(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " INFO:    "); _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::flush; } }
template <typename... Args> void info_trace_line(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " INFO:    "); _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void info_trace_space(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " INFO:    "); _trace_space(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void info_trace_comma(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " INFO:    "); _trace_comma(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }

template <typename... Args> void error_trace(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " ERROR:   "); _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::flush; } }
template <typename... Args> void error_trace_line(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " ERROR:   "); _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void error_trace_space(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " ERROR:   "); _trace_space(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void error_trace_comma(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " ERROR:   "); _trace_comma(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }

template <typename... Args> void warning_trace(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " WARNING: "); _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::flush; } }
template <typename... Args> void warning_trace_line(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " WARNING: "); _trace(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void warning_trace_space(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " WARNING: "); _trace_space(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }
template <typename... Args> void warning_trace_comma(Args&&... args) { _Tracestream oss; print_time_ms(oss); _trace(oss, " WARNING: "); _trace_comma(oss, TRY_MOVE(args)...); { TraceMutex_ mt; std::wcout << oss.str() << std::endl; } }

struct trace_result
{
	template <typename... Args>
	void operator ()(Args&&... args)
	{
		trace_comma(TRY_MOVE(args)...);
	}

	void operator ()()
	{
		trace_line("empty...");
	}
};

#endif