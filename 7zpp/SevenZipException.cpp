#include "stdafx.h"
#include "SevenZipException.h"

namespace SevenZip
{
	TString Exception::FormatString(const TCHAR* format, ...)
	{
		va_list	args;
		va_start(args, format);

		const int count = _vsctprintf(format, args) + 1;
		TString result(count, _T('\0'));
		_vsntprintf_s(result.data(), result.size(), _TRUNCATE, format, args);
		va_end(args);

		return result;
	}
	TString Exception::GetWinErrMsg(const TString& contextMessage, DWORD lastError)
	{
		// TODO: use FormatMessage to get the appropriate message from the 
		return FormatString(_T("%s: GetLastError = %lu"), contextMessage.c_str(), lastError);
	}
	TString Exception::GetCOMErrMsg(const TString& contextMessage, HRESULT lastError)
	{
		// TODO: use FormatMessage to get the appropriate message from the 
		return FormatString(_T("%s: HRESULT = 0x%08X"), contextMessage.c_str(), lastError);
	}
}
