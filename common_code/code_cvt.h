#ifndef __CODE_CVT_H
#define __CODE_CVT_H

#include <string>

using namespace std;

string wchar_to_utf8(const wstring& wc);
string wchar_to_utf8(const wchar_t* wc);

wstring utf8_to_wchar(const string& u8);
wstring utf8_to_wchar(const char* u8);

string gbk_to_utf8(const string& gb);
string gbk_to_utf8(const char* gb);

string utf8_to_gbk(const string& u8);
string utf8_to_gbk(const char* u8);

string wchar_to_gbk(const wstring& wc);
string wchar_to_gbk(const wchar_t* wc, size_t length = -1);

wstring gbk_to_wchar(const string& gb);
wstring gbk_to_wchar(const char* gb, size_t length = -1);

#endif