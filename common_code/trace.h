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

#if _MSC_VER == 1600
template <typename T0>
void trace(T0&& p0)
{
	trace_match<RM_CREF(T0)>::trace(p0);
}

template <typename T0, typename T1>
void trace(T0&& p0, T1&& p1)
{
	trace(TRY_MOVE(p0));
	trace_match<RM_CREF(T1)>::trace(p1);
}

template <typename T0, typename T1, typename T2>
void trace(T0&& p0, T1&& p1, T2&& p2)
{
	trace(TRY_MOVE(p0), TRY_MOVE(p1));
	trace_match<RM_CREF(T2)>::trace(p2);
}

template <typename T0, typename T1, typename T2, typename T3>
void trace(T0&& p0, T1&& p1, T2&& p2, T3&& p3)
{
	trace(TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2));
	trace_match<RM_CREF(T3)>::trace(p3);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4>
void trace(T0&& p0, T1&& p1, T2&& p2, T3&& p3, T4&& p4)
{
	trace(TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2), TRY_MOVE(p3));
	trace_match<RM_CREF(T4)>::trace(p4);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
void trace(T0&& p0, T1&& p1, T2&& p2, T3&& p3, T4&& p4, T5&& p5)
{
	trace(TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2), TRY_MOVE(p3), TRY_MOVE(p4));
	trace_match<RM_CREF(T5)>::trace(p5);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void trace(T0&& p0, T1&& p1, T2&& p2, T3&& p3, T4&& p4, T5&& p5, T6&& p6)
{
	trace(TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2), TRY_MOVE(p3), TRY_MOVE(p4), TRY_MOVE(p5));
	trace_match<RM_CREF(T6)>::trace(p6);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
void trace(T0&& p0, T1&& p1, T2&& p2, T3&& p3, T4&& p4, T5&& p5, T6&& p6, T7&& p7)
{
	trace(TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2), TRY_MOVE(p3), TRY_MOVE(p4), TRY_MOVE(p5), TRY_MOVE(p6));
	trace_match<RM_CREF(T7)>::trace(p7);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
void trace(T0&& p0, T1&& p1, T2&& p2, T3&& p3, T4&& p4, T5&& p5, T6&& p6, T7&& p7, T8&& p8)
{
	trace(TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2), TRY_MOVE(p3), TRY_MOVE(p4), TRY_MOVE(p5), TRY_MOVE(p6), TRY_MOVE(p7));
	trace_match<RM_CREF(T8)>::trace(p8);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
void trace(T0&& p0, T1&& p1, T2&& p2, T3&& p3, T4&& p4, T5&& p5, T6&& p6, T7&& p7, T8&& p8, T9&& p9)
{
	trace(TRY_MOVE(p0), TRY_MOVE(p1), TRY_MOVE(p2), TRY_MOVE(p3), TRY_MOVE(p4), TRY_MOVE(p5), TRY_MOVE(p6), TRY_MOVE(p7), TRY_MOVE(p8));
	trace_match<RM_CREF(T9)>::trace(p9);
}

#elif _MSC_VER > 1600

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

#endif