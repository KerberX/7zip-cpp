#pragma once
#include <exception>
#include "SevenString.h"

namespace SevenZip
{
	TString FormatString(const TCHAR* format, ...);
	TString GetWinErrMsg(const TString& contextMessage, DWORD lastError);
	TString GetCOMErrMsg(const TString& contextMessage, HRESULT lastError);

	class Exception: public std::exception
	{
		public:
			static TString FormatString(const TCHAR* format, ...);
			static TString GetWinErrMsg(const TString& contextMessage, DWORD lastError);
			static TString GetCOMErrMsg(const TString& contextMessage, HRESULT lastError);

		protected:
			TString m_Message;

		public:
			Exception() = default;
			Exception(const TString& message)
				:m_Message(message)
			{
			}

		public:
			const TString& GetMessage() const
			{
				return m_Message;
			}
	};
}
