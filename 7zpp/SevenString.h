#pragma once
#include <tchar.h>
#include <string>
#include <string_view>
#include <vector>

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
	using TStringVector = std::vector<TString>;
}

namespace SevenZip::String
{
	namespace Internal
	{
		template<class TChar>
		std::basic_string<TChar> DoFormat(const TChar* formatString, ...)
		{
			TString buffer;

			va_list argptr;
			va_start(argptr, formatString);

			int count = 0;
			if constexpr(std::is_same_v<TChar, wchar_t>)
			{
				count = _vscwprintf(formatString, argptr);
			}
			else if constexpr(std::is_same_v<TChar, char>)
			{
				count = _vscprintf(formatString, argptr);
			}
			else
			{
				static_assert(false, "function 'Format' is unavailable for this char type");
			}

			if (count > 0)
			{
				// Resize to exact required length, the string will take care of null terminator
				buffer.resize((size_t)count);

				// And tell vs[n][w]printf that we allocated buffer with space for that null terminator
				// because it expects length with it, otherwise it won't print last character.
				const size_t effectiveSize = buffer.size() + 1;
				if constexpr(std::is_same_v<TChar, wchar_t>)
				{
					count = vswprintf(buffer.data(), effectiveSize, formatString, argptr);
				}
				else if constexpr(std::is_same_v<TChar, char>)
				{
					count = vsnprintf(buffer.data(), effectiveSize, formatString, argptr);
				}
			}
			va_end(argptr);
			return buffer;
		}
	}

	template<class T>
	TString ToString(T value)
	{
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

		if constexpr(std::is_same_v<TChar, wchar_t>)
		{
			return std::to_wstring(std::move(value));
		}
		else
		{
			return std::to_string(std::move(value));
		}
	}

	template<class... Args>
	TString Format(const TChar* formatString, Args&&... arg)
	{
		return Internal::DoFormat(formatString, std::forward<Args>(arg)...);
	}

	TString& MakeLower(TString& string);
	TString& MakeUpper(TString& string);
	inline TString ToLower(TStringView string)
	{
		TString temp(string);
		MakeLower(temp);
		return temp;
	}
	inline TString ToUpper(TStringView string)
	{
		TString temp(string);
		MakeUpper(temp);
		return temp;
	}

	int Compare(TStringView left, TStringView right);
	int CompareNoCase(TStringView left, TStringView right);
}

namespace SevenZip::String
{
	enum class CodePage
	{
		ANSI = CP_ACP,
		OEM = CP_OEMCP,
		MAC = CP_MACCP,
		ThreadANSI = CP_THREAD_ACP,
		Symbol = CP_SYMBOL,
		UTF7 = CP_UTF7,
		UTF8 = CP_UTF8
	};

	std::wstring ToUnicode(const char* source, size_t length, CodePage sourceCodePage = CodePage::UTF8);
	inline std::wstring ToUnicode(std::string_view source, CodePage sourceCodePage = CodePage::UTF8)
	{
		return ToUnicode(source.data(), source.length(), sourceCodePage);
	}

	std::string ToCodePage(const wchar_t* source, size_t length, CodePage targetCodePage);
	inline std::string ToCodePage(std::wstring_view source, CodePage targetCodePage)
	{
		return ToCodePage(source.data(), source.length(), targetCodePage);
	}

	inline std::string ToUTF8(const wchar_t* source, size_t length)
	{
		return ToCodePage(source, length, CodePage::UTF8);
	}
	inline std::string ToUTF8(std::wstring_view source)
	{
		return ToCodePage(source, CodePage::UTF8);
	}
}
