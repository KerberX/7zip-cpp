// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#pragma once
#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>
#include "ProgressNotifier.h"
#include "COM.h"

namespace SevenZip::Callback
{
	class OpenArchive: public IArchiveOpenCallback, public ICryptoGetTextPassword
	{
		private:
			COM::RefCount<OpenArchive> m_RefCount;

		protected:
			ProgressNotifierDelegate m_Notifier;
			TString m_ArchivePath;

			int64_t m_BytesCompleted = 0;
			int64_t m_BytesTotal = 0;

		public:
			OpenArchive(const TString& path, ProgressNotifier* notifier = nullptr)
				:m_RefCount(*this), m_Notifier(notifier), m_ArchivePath(path)
			{
			}
			virtual ~OpenArchive() = default;

		public:
			void SetNotifier(ProgressNotifier* notifier)
			{
				m_Notifier = notifier;
			}

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
