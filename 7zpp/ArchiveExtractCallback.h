// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#pragma once
#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>
#include "SevenZipArchive.h"
#include "ProgressNotifier.h"
#include "FileInfo.h"
#include "COM.h"
#include <optional>

namespace SevenZip::Callback
{
	class Extractor: public IArchiveExtractCallback, public ICryptoGetTextPassword
	{
		private:
			COM::RefCount<Extractor> m_RefCount;

		protected:
			CComPtr<IInArchive> m_Archive;
			ProgressNotifierDelegate m_Notifier;

		protected:
			std::optional<FileInfo> GetFileInfo(FileIndex fileIndex) const;

		public:
			Extractor(ProgressNotifier* notifier = nullptr)
				:m_RefCount(*this), m_Notifier(notifier)
			{
			}
			virtual ~Extractor() = default;

		public:
			void SetArchive(const CComPtr<IInArchive>& archive)
			{
				m_Archive = archive;
			}
			void SetNotifier(ProgressNotifier* notifier)
			{
				m_Notifier = notifier;
			}

		public:
			STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) override;
			STDMETHOD_(ULONG, AddRef)() override
			{
				return m_RefCount.AddRef();
			}
			STDMETHOD_(ULONG, Release)() override
			{
				return m_RefCount.Release();
			}

			// IProgress
			STDMETHOD(SetTotal)(UInt64 size) override;
			STDMETHOD(SetCompleted)(const UInt64* completeValue) override;

			// ICryptoGetTextPassword
			STDMETHOD(CryptoGetTextPassword)(BSTR* password) override;
	};
}

namespace SevenZip::Callback
{
	class FileExtractor: public Extractor
	{
		protected:
			TString m_Directory;
			TString m_TargetPath;
			std::optional<FileInfo> m_FileInfo;

			int64_t m_BytesCompleted = 0;
			size_t m_ItemCount = 0;

		public:
			FileExtractor() = default;
			FileExtractor(TStringView directory, ProgressNotifier* notifier = nullptr)
				:Extractor(notifier), m_Directory(directory)
			{
			}

		public:
			// IArchiveExtractCallback
			STDMETHOD(GetStream)(UInt32 fileIndex, ISequentialOutStream** outStream, Int32 askExtractMode) override;
			STDMETHOD(PrepareOperation)(Int32 askExtractMode) override;
			STDMETHOD(SetOperationResult)(Int32 resultEOperationResult) override;

		public:
			virtual HRESULT GetTargetPath(uint32_t fileIndex, const FileInfo& fileInfo, TString& targetPath) const
			{
				return S_FALSE;
			}
	};
}
