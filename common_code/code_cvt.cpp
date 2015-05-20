#include "code_cvt.h"
#include <codecvt>

std::string wchar_to_utf8(const wstring& wc)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(wc);
}

std::string wchar_to_utf8(const wchar_t* wc)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(wc);
}

std::wstring utf8_to_wchar(const string& u8)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
	return conv.from_bytes(u8);
}

std::wstring utf8_to_wchar(const char* u8)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
	return conv.from_bytes(u8);
}

std::wstring gbk_to_wchar(const string& gb)
{
	return gbk_to_wchar(gb.c_str(), gb.size());
}

std::wstring gbk_to_wchar(const char* gb, size_t length /*= -1*/)
{
	if (-1 == length)
	{
		length = strlen(gb);
	}
	const char* data_from = gb;
	const char* data_from_end = gb + length;
	const char* data_from_next = 0;

	wstring result_str;
	result_str.resize(length);

	wchar_t* data_to = &result_str[0];
	wchar_t* data_to_end = &result_str[result_str.size()];
	wchar_t* data_to_next = 0;

	typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;
	mbstate_t in_state = 0;
	std::locale sys_locale("");
	auto result = std::use_facet<convert_facet>(sys_locale).in(
		in_state, data_from, data_from_end, data_from_next,
		data_to, data_to_end, data_to_next);
	if (convert_facet::ok == result)
	{
		size_t l = -1;
		while (result_str[++l]){}
		result_str.resize(l);
		return result_str;
	}
	return std::wstring(L"");
}

std::string wchar_to_gbk(const wstring& wc)
{
	return wchar_to_gbk(wc.c_str(), wc.size());
}

std::string wchar_to_gbk(const wchar_t* wc, size_t length /*= -1*/)
{
	if (-1 == length)
	{
		while (wc[++length]){}
	}
	const wchar_t* data_from = wc;
	const wchar_t* data_from_end = wc + length;
	const wchar_t* data_from_next = 0;

	string result_str;
	result_str.resize((length + 1) * 2);
	char* data_to = &result_str[0];
	char* data_to_end = &result_str[result_str.size()];
	char* data_to_next = 0;

	typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;
	mbstate_t out_state = 0;
	std::locale sys_locale("");
	auto result = std::use_facet<convert_facet>(sys_locale).out(
		out_state, data_from, data_from_end, data_from_next,
		data_to, data_to_end, data_to_next);
	if (result == convert_facet::ok)
	{
		size_t l = -1;
		while (result_str[++l]){}
		result_str.resize(l);
		return result_str;
	}
	return std::string("");
}

std::string gbk_to_utf8(const string& gb)
{
	return wchar_to_utf8(gbk_to_wchar(gb));
}

std::string gbk_to_utf8(const char* gb)
{
	return wchar_to_utf8(gbk_to_wchar(gb));
}

std::string utf8_to_gbk(const string& u8)
{
	return wchar_to_gbk(utf8_to_wchar(u8));
}

std::string utf8_to_gbk(const char* u8)
{
	return wchar_to_gbk(utf8_to_wchar(u8));
}
