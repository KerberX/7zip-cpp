#include "StdAfx.h"
#include "SevenString.h"

namespace
{
	int DoCompareStrings(std::wstring_view left, std::wstring_view right, bool ignoreCase)
	{
		return ::CompareStringOrdinal(left.data(), left.length(), right.data(), left.length(), FALSE);
	}
	int DoCompareStrings(std::string_view left, std::string_view right, bool ignoreCase)
	{
		using namespace SevenZip;

		// Assuming UTF8 as source string encoding
		const std::wstring leftUnicode = String::ToUnicode(left);
		const std::wstring rightUnicode = String::ToUnicode(right);
		return DoCompareStrings(leftUnicode, rightUnicode, ignoreCase);
	}
}

namespace SevenZip::String
{
	TString& MakeLower(TString& string)
	{
		::CharLowerBuff(string.data(), (DWORD)string.length());
		return string;
	}
	TString& MakeUpper(TString& string)
	{
		::CharUpperBuff(string.data(), (DWORD)string.length());
		return string;
	}

	int Compare(TStringView left, TStringView right)
	{
		return DoCompareStrings(left, right, false);
	}
	int CompareNoCase(TStringView left, TStringView right)
	{
		return DoCompareStrings(left, right, true);
	}
}

namespace SevenZip::String
{
	std::wstring ToUnicode(const char* source, size_t length, CodePage sourceCodePage)
	{
		if (source == nullptr || length == 0)
		{
			return {};
		}
		if (length == std::string::npos)
		{
			length = std::char_traits<char>::length(source);
		}

		const int lengthRequired = ::MultiByteToWideChar(static_cast<UINT>(sourceCodePage), 0, source, static_cast<int>(length), nullptr, 0);
		if (lengthRequired > 0)
		{
			std::wstring converted;
			converted.resize(static_cast<size_t>(lengthRequired));
			::MultiByteToWideChar(static_cast<UINT>(sourceCodePage), 0, source, static_cast<int>(length), converted.data(), lengthRequired);

			return converted;
		}
		return {};
	}
	std::string ToCodePage(const wchar_t* source, size_t length, CodePage targetCodePage)
	{
		if (source == nullptr || length == 0)
		{
			return {};
		}
		if (length == std::wstring::npos)
		{
			length = std::char_traits<wchar_t>::length(source);
		}

		const int lengthRequired = ::WideCharToMultiByte(static_cast<UINT>(targetCodePage), 0, source, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
		if (lengthRequired > 0)
		{
			std::string converted;
			converted.resize(static_cast<size_t>(lengthRequired));
			::WideCharToMultiByte(static_cast<UINT>(targetCodePage), 0, source, static_cast<int>(length), converted.data(), lengthRequired, nullptr, nullptr);

			return converted;
		}
		return {};
	}
}
