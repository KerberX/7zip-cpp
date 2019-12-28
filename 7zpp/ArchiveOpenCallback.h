// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#pragma once
#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>
#include "COM.h"

namespace SevenZip
{
	class ProgressNotifier;
}

namespace SevenZip::Callback
{
	class OpenArchive:public IArchiveOpenCallback, public ICryptoGetTextPassword
	{
		private:
			COM::RefCount<OpenArchive> m_RefCount;

			ProgressNotifier* m_Notifier = nullptr;
			int64_t m_Total = 0;

		public:
			OpenArchive(ProgressNotifier* notifier = nullptr);
			~OpenArchive();

		public:
			STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject);
			STDMETHOD_(ULONG, AddRef)() override
			{
				return m_RefCount.AddRef();
			}
			STDMETHOD_(ULONG, Release)() override
			{
				return m_RefCount.Release();
			}

			// IArchiveOpenCallback
			STDMETHOD(SetTotal)(const UInt64* files, const UInt64* bytes);
			STDMETHOD(SetCompleted)(const UInt64* files, const UInt64* bytes);

			// ICryptoGetTextPassword
			STDMETHOD(CryptoGetTextPassword)(BSTR* password);
	};
}
