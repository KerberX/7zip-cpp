#include "stdafx.h"
#include "SevenZipExtractor.h"
#include "GUIDs.h"
#include "FileSys.h"
#include "ArchiveOpenCallback.h"
#include "ArchiveExtractCallback.h"
#include "InStreamWrapper.h"
#include "PropVariant.h"
#include "UsefulFunctions.h"


namespace SevenZip
{
	using namespace intl;

	SevenZipExtractor::SevenZipExtractor(const SevenZipLibrary& library, const TString& archivePath)
		: SevenZipArchive(library, archivePath)
	{
	}
	SevenZipExtractor::~SevenZipExtractor()
	{
	}

	bool SevenZipExtractor::ExtractArchive(const TString& destDirectory, ProgressCallback* callback)
	{
		CComPtr<IStream> fileStream = FileSys::OpenFileToRead(m_archivePath);
		if (fileStream == NULL)
		{
			return false;	//Could not open archive
		}
		return ExtractArchiveInternal(fileStream, destDirectory, callback);
	}
	bool SevenZipExtractor::ExtractArchive(const IndiencesArray& tFilesIndices, const TString& directory, ProgressCallback* callback)
	{
		CComPtr<IStream> fileStream = FileSys::OpenFileToRead(m_archivePath);
		if (fileStream == NULL)
		{
			return false;	//Could not open archive
		}
		return ExtractArchiveInternal(fileStream, directory, callback, &tFilesIndices);
	}
	bool SevenZipExtractor::ExtractToMemory(uint32_t nIndex, void* pBuffer, ProgressCallback* callback)
	{
		CComPtr<IStream> fileStream = FileSys::OpenFileToRead(m_archivePath);
		if (fileStream != NULL)
		{
			CComPtr<IInArchive> archive = UsefulFunctions::GetArchiveReader(m_library, m_compressionFormat);
			CComPtr<InStreamWrapper> inFile = new InStreamWrapper(fileStream);
			CComPtr<ArchiveOpenCallback> openCallback = new ArchiveOpenCallback();

			HRESULT hr = archive->Open(inFile, 0, openCallback);
			if (hr == S_OK)
			{
				CComPtr<ArchiveExtractCallbackMemory> extractCallback = new ArchiveExtractCallbackMemory(archive, pBuffer, callback);
				
				UInt32 tFile[] = {nIndex};
				hr = archive->Extract(tFile, 1, false, extractCallback);
				if (hr == S_OK)
				{
					if (callback)
					{
						callback->OnDone(m_archivePath);
					}
					archive->Close();
					return true;
				}
			}
		}
		return false;
	}

	bool SevenZipExtractor::ExtractArchiveInternal(const CComPtr<IStream>& archiveStream, const TString& destDirectory, ProgressCallback* callback, const IndiencesArray* pFilesIndices)
	{
		CComPtr<IInArchive> archive = UsefulFunctions::GetArchiveReader(m_library, m_compressionFormat);
		CComPtr<InStreamWrapper> inFile = new InStreamWrapper(archiveStream);
		CComPtr<ArchiveOpenCallback> openCallback = new ArchiveOpenCallback();

		HRESULT hr = archive->Open(inFile, 0, openCallback);
		if (hr != S_OK)
		{
			return false;	//Open archive error
		}

		CComPtr< ArchiveExtractCallback > extractCallback = new ArchiveExtractCallback(archive, destDirectory, callback);

		const UInt32* pIndiences = pFilesIndices ? pFilesIndices->data() : NULL;
		UInt32 nIndiencesCount = pFilesIndices ? pFilesIndices->size() : -1;
		hr = archive->Extract(pIndiences, nIndiencesCount, false, extractCallback);
		if (hr != S_OK) // returning S_FALSE also indicates error
		{
			return false;	//Extract archive error
		}

		if (callback)
		{
			callback->OnDone(m_archivePath);
		}
		archive->Close();
		return true;
	}
}
