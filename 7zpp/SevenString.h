#pragma once
#include <tchar.h>
#include <string>

namespace SevenZip
{
	#ifdef _UNICODE
		using TString = std::wstring;
		using TStringView = std::wstring_view;
	#else
		using TString = std::string;
		using TStringView = std::string_view
	#endif

	using TChar = TString::value_type;
}
