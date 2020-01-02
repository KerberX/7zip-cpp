// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#pragma once
#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>
#include "SevenZipArchive.h"
#include "COM.h"
#include <optional>

namespace SevenZip
{
	class ProgressNotifier;
	class OutStream;
}

namespace SevenZip::Callback
{
	class Extractor: public IArchiveExtractCallback, public ICryptoGetTextPassword
	{
		private:
			COM::RefCount<Extractor> m_RefCount;

		protected:
			CComPtr<IInArchive> m_Archive;
			ProgressNotifier* m_Notifier = nullptr;

			// Attributes
			TString m_RelativePath;
			std::optional<uint32_t> m_Attributes;
			std::optional<FILETIME> m_ModificationTime;
			std::optional<int64_t> m_FileSize;
			std::optional<int64_t> m_CompressedFileSize;
			bool m_IsDirectory = false;

		protected:
			TString GetPropertyFilePath(uint32_t fileIndex) const;
			std::optional<int64_t> GetPropertyFileSize(uint32_t fileIndex) const;
			std::optional<int64_t> GetPropertyCompressedFileSize(uint32_t fileIndex) const;
			std::optional<uint32_t> GetPropertyAttributes(uint32_t fileIndex) const;
			std::optional<FILETIME> GetPropertyModifiedTime(uint32_t fileIndex) const;
			bool GetPropertyIsDirectory(uint32_t fileIndex) const;
			HRESULT RetrieveAllProperties(uint32_t index, int32_t askExtractMode);

			void EmitDoneCallback();
			void EmitFileDoneCallback(const TString& path);

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
			int64_t m_BytesCompleted = 0;
			size_t m_FilesCount = 0;

		public:
			FileExtractor() = default;
			FileExtractor(const TString& directory, ProgressNotifier* notifier = nullptr)
				:Extractor(notifier), m_Directory(directory)
			{
			}

		public:
			// IArchiveExtractCallback
			STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode) override;
			STDMETHOD(PrepareOperation)(Int32 askExtractMode) override;
			STDMETHOD(SetOperationResult)(Int32 resultEOperationResult) override;

		public:
			virtual HRESULT GetTargetPath(const UInt32 index, const TString& ralativePath, TString& targetPath) const
			{
				return S_OK;
			}
	};
}

namespace SevenZip::Callback
{
	class StreamExtractor: public Extractor
	{
		public:
			StreamExtractor(ProgressNotifier* notifier = nullptr)
				:Extractor(notifier)
			{
			}

		public:
			// IProgress
			STDMETHOD(SetTotal)(UInt64 size) override;
			STDMETHOD(SetCompleted)(const UInt64* completeValue) override;

			// IArchiveExtractCallback
			STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode) override;
			STDMETHOD(PrepareOperation)(Int32 askExtractMode)
			{
				return S_OK;
			}
			STDMETHOD(SetOperationResult)(Int32 resultEOperationResult) override
			{
				EmitFileDoneCallback(m_RelativePath);
				return S_OK;
			}

		public:
			virtual CComPtr<OutStream> CreateStream(uint32_t fileIndex, const TString& ralativePath) = 0;
			virtual HRESULT OnDirectory(uint32_t fileIndex, const TString& ralativePath)
			{
				return S_FALSE;
			}
	};
}
