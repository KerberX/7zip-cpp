#pragma once
#include <tchar.h>
#include <string>

namespace SevenZip
{
	#ifdef _UNICODE
	typedef std::wstring TString;
	typedef std::wstring_view TStringView;
	#else
	typedef std::string TString;
	typedef std::string_view TStringView;
	#endif
	typedef TString::value_type TCharType;
}
