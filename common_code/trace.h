#ifndef __TRACE_H
#define __TRACE_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iostream>
#include "try_move.h"

template <typename T>
struct trace_match 
{
	template <typename TP>
	static void trace(TP&& s)
	{
		std::cout << s;
	}
};

template <>
struct trace_match<bool> 
{
	static void trace(bool b)
	{
		std::cout << (b ? "true" : "false");
	}
};

template <>
struct trace_match<wchar_t>
{
	static void trace(wchar_t wc)
	{
		std::wcout << wc;
	}
};

template <size_t N, typename T>
struct trace_match<T[N]>
{
	static void trace(const T (&s)[N])
	{
		std::cout << "[";
		for (int i = 0; i < (int)N - 1; i++)
		{
			trace_match<T>::trace(s[i]);
			std::cout << ", ";
		}
		if (N)
		{
			trace_match<T>::trace(s[N - 1]);
		}
		std::cout << "]";
	}
};

template <size_t N>
struct trace_match<char[N]>
{
	static void trace(const char (&s)[N])
	{
		std::cout << s;
	}
};

template <size_t N>
struct trace_match<wchar_t[N]>
{
	static void trace(const wchar_t(&s)[N])
	{
		std::wcout << s;
	}
};

template <>
struct trace_match<std::wstring>
{
	static void trace(const std::wstring& s)
	{
		std::wcout << s;
	}
};

template <typename T>
struct trace_match<std::vector<T>>
{
	static void trace(const vector<T>& s)
	{
		std::cout << "[";
		for (int i = 0; i < (int)s.size() - 1; i++)
		{
			trace_match<RM_CREF(T)>::trace(s[i]);
			std::cout << ", ";
		}
		if (!s.empty())
		{
			trace_match<RM_CREF(T)>::trace(s.back());
		}
		std::cout << "]";
	}
};

template <typename T>
struct trace_match<std::list<T>>
{
	static void trace(const list<T>& s)
	{
		std::cout << "{";
		int l = (int)s.size();
		auto it = s.begin();
		for (int i = 0; i < l - 1; i++, it++)
		{
			trace_match<RM_CREF(T)>::trace(*it);
			std::cout << "->";
		}
		if (l)
		{
			trace_match<RM_CREF(T)>::trace(*it);
		}
		std::cout << "}";
	}
};

template <typename K, typename V>
struct trace_match<std::map<K, V>>
{
	static void trace(const std::map<K, V>& s)
	{
		std::cout << "{";
		int l = (int)s.size();
		auto it = s.begin();
		for (int i = 0; i < l - 1; i++, it++)
		{
			trace_match<RM_CREF(K)>::trace(it->first);
			std::cout << ": ";
			trace_match<RM_CREF(V)>::trace(it->second);
			std::cout << ", ";
		}
		if (l)
		{
			trace_match<RM_CREF(K)>::trace(it->first);
			std::cout << ": ";
			trace_match<RM_CREF(V)>::trace(it->second);
		}
		std::cout << "}";
	}
};

template <typename T>
struct trace_match<std::set<T>>
{
	static void trace(const set<T>& s)
	{
		std::cout << "{";
		int l = (int)s.size();
		auto it = s.begin();
		for (int i = 0; i < l - 1; i++, it++)
		{
			trace_match<RM_CREF(T)>::trace(*it);
			std::cout << ", ";
		}
		if (l)
		{
			trace_match<RM_CREF(T)>::trace(*it);
		}
		std::cout << "}";
	}
};

void trace()
{

}

void trace_line()
{
	std::cout << std::endl;
}

template <typename T, typename... Args>
void trace(T&& p, Args&&... args)
{
	trace_match<RM_CREF(T)>::trace(p);
	trace(TRY_MOVE(args)...);
}

template <typename T, typename... Args>
void trace_line(T&& p, Args&&... args)
{
	trace_match<RM_CREF(T)>::trace(p);
	trace(TRY_MOVE(args)...);
	std::cout << std::endl;
}

#endif