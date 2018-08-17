// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#pragma once
#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>
#include "SevenZipArchive.h"
#include "ProgressNotifier.h"
//#include "7zip/Archive/IArchive.h"

namespace SevenZip
{
	namespace intl
	{
		class ArchiveExtractCallback: public IArchiveExtractCallback, public ICryptoGetTextPassword
		{
			protected:
				long m_RefCount;
				CComPtr<IInArchive> m_ArchiveHandler;
				TString m_Directory;
				TStringVector m_FinalPaths;
				size_t m_FinalPathsCounter = 0;
				int64_t m_BytesCompleted = 0;

				TString m_RelativePath;
				TString m_AbsolutePath;
				bool m_IsDirectory;

				bool m_HasAttributes;
				UInt32 m_Attributes;

				bool m_HasModificationTime;
				FILETIME m_ModificationTime;

				bool m_HasNewFileSize;
				UInt64 m_NewFileSize;

				ProgressNotifier* m_Notifier;

			protected:
				void GetPropertyFilePath(UInt32 index);
				void GetPropertyAttributes(UInt32 index);
				void GetPropertyIsDir(UInt32 index);
				void GetPropertyModifiedTime(UInt32 index);
				void GetPropertySize(UInt32 index);

				void EmitDoneCallback();
				void EmitFileDoneCallback(const TString& path);

			protected:
				HRESULT RetrieveAllProperties(UInt32 index, Int32 askExtractMode, bool* pContinue);

			public:
				ArchiveExtractCallback(const CComPtr<IInArchive>& archiveHandler, const TString& directory, ProgressNotifier* notifier = NULL);
				virtual ~ArchiveExtractCallback();

			public:
				STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) override;
				STDMETHOD_(ULONG, AddRef)() override;
				STDMETHOD_(ULONG, Release)() override;

				// IProgress
				STDMETHOD(SetTotal)(UInt64 size) override;
				STDMETHOD(SetCompleted)(const UInt64* completeValue) override;

				// IArchiveExtractCallback
				STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode) override;
				STDMETHOD(PrepareOperation)(Int32 askExtractMode) override;
				STDMETHOD(SetOperationResult)(Int32 resultEOperationResult) override;

				// ICryptoGetTextPassword
				STDMETHOD(CryptoGetTextPassword)(BSTR* password) override;

			public:
				void SetFinalPath(const TStringVector& paths)
				{
					m_FinalPaths = paths;
				}
		};

		//////////////////////////////////////////////////////////////////////////
		class ArchiveExtractCallbackMemory: public ArchiveExtractCallback
		{
			private:
				SevenZip::DataBufferMap& m_BufferMap;

			public:
				ArchiveExtractCallbackMemory(const CComPtr<IInArchive>& archiveHandler, SevenZip::DataBufferMap& bufferMap, ProgressNotifier* notifier);
				virtual ~ArchiveExtractCallbackMemory();

			public:
				// IProgress
				STDMETHOD(SetTotal)(UInt64 size) override;
				STDMETHOD(SetCompleted)(const UInt64* completeValue) override;

				// IArchiveExtractCallback
				STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode) override;
				STDMETHOD(SetOperationResult)(Int32 resultEOperationResult) override;
		};
	}
}
