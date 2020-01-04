// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#pragma once
#include <7zip/Archive/IArchive.h>
#include <7zip/ICoder.h>
#include <7zip/IPassword.h>
#include "ProgressNotifier.h"
#include "FileInfo.h"
#include "COM.h"

namespace SevenZip::Callback
{
	class UpdateArchiveBase: public IArchiveUpdateCallback, public ICryptoGetTextPassword2, public ICompressProgressInfo
	{
		private:
			COM::RefCount<UpdateArchiveBase> m_RefCount;

		protected:
			ProgressNotifierDelegate m_Notifier;

			TString m_DirectoryPrefix;
			TString m_OutputPath;
			size_t m_ExistingItemsCount = 0;
			const std::vector<TString>& m_TargetPaths;
			const std::vector<FilePathInfo>& m_SourcePaths;

		public:
			UpdateArchiveBase(const TString& dirPrefix,
							  const std::vector<FilePathInfo>& filePaths,
							  const std::vector<TString>& inArchiveFilePaths,
							  const TString& outputFilePath,
							  ProgressNotifier* notifier = nullptr)
				:m_RefCount(*this),
				m_DirectoryPrefix(dirPrefix),
				m_SourcePaths(filePaths),
				m_TargetPaths(inArchiveFilePaths),
				m_OutputPath(outputFilePath),
				m_Notifier(notifier)
			{
			}
			virtual ~UpdateArchiveBase() = default;

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

			// IProgress
			STDMETHOD(SetTotal)(UInt64 size);
			STDMETHOD(SetCompleted)(const UInt64* completeValue);

			// IArchiveUpdateCallback
			STDMETHOD(GetUpdateItemInfo)(UInt32 index, Int32* newData, Int32* newProperties, UInt32* indexInArchive);
			STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT* value);
			STDMETHOD(GetStream)(UInt32 index, ISequentialInStream** inStream);
			STDMETHOD(SetOperationResult)(Int32 operationResult);

			// ICryptoGetTextPassword2
			STDMETHOD(CryptoGetTextPassword2)(Int32* passwordIsDefined, BSTR* password);

			// ICompressProgressInfo
			STDMETHOD(SetRatioInfo)(const UInt64* inSize, const UInt64* outSize);

		public:
			void SetExistingItemsCount(size_t existingItemsCount)
			{
				m_ExistingItemsCount = existingItemsCount;
			}
	};
}
