#pragma once


#include <exception>
#include "SevenString.h"


namespace SevenZip
{
	TString StrFmt(const TCHAR* format, ...);
	TString GetWinErrMsg(const TString& contextMessage, DWORD lastError);
	TString GetCOMErrMsg(const TString& contextMessage, HRESULT lastError);

	class SevenZipException
	{
		protected:
			TString m_Message;

		public:
			SevenZipException();
			SevenZipException(const TString& message);
			virtual ~SevenZipException();

		public:
			const TString& GetMessage() const;
	};
}
