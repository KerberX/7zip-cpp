#include "stdafx.h"
#include "SevenZipArchive.h"
#include "SevenZipLibrary.h"
#include <atlbase.h>
#include "FileInfo.h"
#include "Utility.h"
#include "GUIDs.h"
#include "FileSys.h"
#include "ArchiveOpenCallback.h"
#include "ArchiveExtractCallback.h"
#include "ArchiveUpdateCallback.h"
#include "ListingNotifier.h"
#include "ProgressNotifier.h"
#include "InStreamWrapper.h"
#include "OutStreamWrapper.h"
#include "PropVariant.h"
#pragma warning(disable: 4101)

namespace SevenZip
{
	// Static
	static const TString AllFilesPattern = _T("*");

	TString SevenZipArchive::GetExtensionFromFormat(CompressionFormatEnum nFormat)
	{
		return Utility::EndingFromCompressionFormat(nFormat);
	}

	//////////////////////////////////////////////////////////////////////////
	// Metadata
	void SevenZipArchive::InvalidateChace()
	{
		m_IsMetaDataReaded = false;
	}

	bool SevenZipArchive::DetectCompressionFormatHelper(CompressionFormatEnum& format)
	{
		m_OverrideCompressionFormat = false;
		return Utility::DetectCompressionFormat(m_LibraryInstance, m_ArchiveFilePath, format, m_Notifier);
	}
	bool SevenZipArchive::ReadInArchiveMetadataHelper()
	{
		bool bDetectedCompressionFormat = true;
		if (!m_OverrideCompressionFormat)
		{
			bDetectedCompressionFormat = DetectCompressionFormatHelper(m_Property_CompressionFormat);
		}
		bool bGotItemNumberNamesAndOrigSizes = InitItemsNames();

		return (bDetectedCompressionFormat && bGotItemNumberNamesAndOrigSizes);
	}
	bool SevenZipArchive::InitNumberOfItems()
	{
		return Utility::GetNumberOfItems(m_LibraryInstance, m_ArchiveFilePath, m_Property_CompressionFormat, m_ItemCount, m_Notifier);
	}
	bool SevenZipArchive::InitItemsNames()
	{
		return Utility::GetItemsNames(m_LibraryInstance, m_ArchiveFilePath, m_Property_CompressionFormat, m_ItemCount, m_ItemNames, m_ItemOriginalSizes, m_Notifier);
	}

	// Extraction
	bool SevenZipArchive::ExtractToDisk(const TString& destDirectory, const IndexesVector* filesIndices, const TStringVector& finalPaths, ProgressNotifier* notifier) const
	{
		CComPtr<IStream> fileStream = FileSys::OpenFileToRead(m_ArchiveFilePath);
		if (fileStream != NULL)
		{
			CComPtr<IInArchive> archive = Utility::GetArchiveReader(m_LibraryInstance, m_Property_CompressionFormat);
			CComPtr<InStreamWrapper> inFile = new InStreamWrapper(fileStream);
			CComPtr<ArchiveOpenCallback> openCallback = new ArchiveOpenCallback();

			HRESULT hr = archive->Open(inFile, 0, openCallback);
			if (hr == S_OK)
			{
				CComPtr<ArchiveExtractCallback> extractCallback = new ArchiveExtractCallback(archive, destDirectory, notifier);
				extractCallback->SetFinalPath(finalPaths);

				const uint32_t* indiencesData = filesIndices ? filesIndices->data() : NULL;
				uint32_t indiencesCount = filesIndices ? (uint32_t)filesIndices->size() : -1;
				hr = archive->Extract(indiencesData, indiencesCount, false, extractCallback);
				
				// Returning S_FALSE also indicates error
				if (hr == S_OK)
				{
					archive->Close();
					if (notifier)
					{
						notifier->OnDone();
					}
					return true;
				}
			}
		}
		return false;
	}
	IndexesVector SevenZipArchive::RemoveInvalidIndexes(const IndexesVector& sourceArray) const
	{
		IndexesVector newArray = sourceArray;
		newArray.erase(std::remove(newArray.begin(), newArray.end(), (IndexesVector::value_type)-1), newArray.end());
		return newArray;
	}

	// Compression
	const char* SevenZipArchive::GetMethodString() const
	{
		switch (m_Property_CompressionMethod)
		{
			case LZMA:
			{
				return "LZMA";
			}
			case BZIP2:
			{
				return "BZip2";
			}
			case PPMD:
			{
				return "PPMd";
			}
		};

		// LZMA2 is the default
		return "LZMA2";
		return "Copy";
	}
	std::string SevenZipArchive::FormatMethodString() const
	{
		int64_t nDictSizeMB = 20 + m_Property_DictionarySize;

		std::string sOut(256, '\000');
		switch (m_Property_CompressionMethod)
		{
			case LZMA:
			case LZMA2:
			{
				sprintf_s(sOut.data(), sOut.size(), "%s:d=%lld", GetMethodString(), nDictSizeMB);
				break;
			}
			case BZIP2:
			{
				sOut = "BZip2:d=900000b";
				break;
			}
			case PPMD:
			{
				sprintf_s(sOut.data(), sOut.size(), "%s:mem=%lld", GetMethodString(), nDictSizeMB);
				break;
			}
		};
		return sOut;
	}
	bool SevenZipArchive::SetCompressionProperties(IUnknown* pArchive)
	{
		std::string method = FormatMethodString();
		const wchar_t* names[] = {L"x", L"s", L"mt", L"m"};
		CPropVariant values[] =
		{
			(UInt32)GetProperty_CompressionLevel(),
			GetProperty_Solid(),
			//"100f10m",
			GetProperty_MultiThreaded(),
			method.c_str()
		};

		CComPtr<ISetProperties> setter;
		pArchive->QueryInterface(IID_ISetProperties, reinterpret_cast<void**>(&setter));
		if (setter == NULL)
		{
			return false; // Archive does not support setting compression properties
		}

		const size_t propertiesCount = ARRAYSIZE(values);
		HRESULT hr = setter->SetProperties(names, values, propertiesCount);
		if (hr != S_OK)
		{
			return false;
		}
		return true;
	}
	bool SevenZipArchive::FindAndCompressFiles(const TString& directory, const TString& searchPattern, const TString& pathPrefix, bool recursion, ProgressNotifier* notifier)
	{
		if (!FileSys::DirectoryExists(directory))
		{
			//Directory does not exist
			return false;
		}
		if (FileSys::IsDirectoryEmptyRecursive(directory))
		{
			//Directory \"%s\" is empty" ), directory.c_str()
			return false;
		}

		FilePathInfoVector files = FileSys::GetFilesInDirectory(directory, searchPattern.empty() ? AllFilesPattern : searchPattern, recursion);

		TStringVector relativePaths(files.size(), TString());
		for (size_t i = 0; i < files.size(); i++)
		{
			relativePaths[i] = FileSys::ExtractRelativePath(directory, files[i].FilePath);
		}
		return CompressFilesToArchive(pathPrefix, files, relativePaths, notifier);
	}
	bool SevenZipArchive::CompressFilesToArchive(const TString& pathPrefix, const FilePathInfoVector& filePaths, const TStringVector& inArchiveFilePaths, ProgressNotifier* notifier)
	{
		CComPtr<IOutArchive> archiveWriter = Utility::GetArchiveWriter(m_LibraryInstance, m_Property_CompressionFormat);
		SetCompressionProperties(archiveWriter);

		CComPtr<OutStreamWrapper> outFile = new OutStreamWrapper(FileSys::OpenFileToWrite(m_ArchiveFilePath));
		CComPtr<ArchiveUpdateCallback> updateCallback = new ArchiveUpdateCallback(pathPrefix, filePaths, inArchiveFilePaths, m_ArchiveFilePath, notifier);
		updateCallback->SetExistingItemsCount(m_ItemCount);

		HRESULT nStatus = archiveWriter->UpdateItems(outFile, (UInt32)filePaths.size(), updateCallback);
		if (nStatus != S_OK) // returning S_FALSE also indicates error
		{
			return false;
		}
		else
		{
			if (notifier)
			{
				notifier->OnDone();
			}
			return true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Constructor and destructor
	SevenZipArchive::SevenZipArchive(const SevenZipLibrary& library, const TString& archivePath, ProgressNotifier* notifier)
		:m_LibraryInstance(library),
		m_Notifier(notifier),
		m_ArchiveFilePath(archivePath),
		m_Property_CompressionFormat(CompressionFormat::Unknown),
		m_Property_CompressionLevel(CompressionLevel::None)
	{
	}
	SevenZipArchive::~SevenZipArchive()
	{
		m_ItemNames.clear();
		m_ItemOriginalSizes.clear();
	}

	// Metadata
	bool SevenZipArchive::ReadInArchiveMetadata()
	{
		m_IsMetaDataReaded = true;
		return ReadInArchiveMetadataHelper();
	}
	bool SevenZipArchive::DetectCompressionFormat()
	{
		m_OverrideCompressionFormat = false;
		return DetectCompressionFormatHelper(m_Property_CompressionFormat);
	}
	size_t SevenZipArchive::GetNumberOfItems()
	{
		if (!m_IsMetaDataReaded)
		{
			m_IsMetaDataReaded = ReadInArchiveMetadataHelper();
		}
		return m_ItemCount;
	}
	const TStringVector& SevenZipArchive::GetItemsNames()
	{
		if (!m_IsMetaDataReaded)
		{
			m_IsMetaDataReaded = ReadInArchiveMetadataHelper();
		}
		return m_ItemNames;
	}
	const std::vector<size_t>& SevenZipArchive::GetOrigSizes()
	{
		if (!m_IsMetaDataReaded)
		{
			m_IsMetaDataReaded = ReadInArchiveMetadataHelper();
		}
		return m_ItemOriginalSizes;
	}

	// Properties
	void SevenZipArchive::SetProperty_CompressionFormat(const CompressionFormatEnum& format)
	{
		m_OverrideCompressionFormat = true;
		InvalidateChace();
		m_Property_CompressionFormat = format;
	}
	CompressionFormatEnum SevenZipArchive::GetProperty_CompressionFormat()
	{
		if (!m_IsMetaDataReaded && !m_OverrideCompressionFormat)
		{
			m_IsMetaDataReaded = ReadInArchiveMetadataHelper();
		}
		return m_Property_CompressionFormat;
	}

	/* Listing */
	bool SevenZipArchive::ListArchive(ListingNotifier* callback) const
	{
		CComPtr<IStream> fileStream = FileSys::OpenFileToRead(m_ArchiveFilePath);
		if (fileStream != NULL)
		{
			CComPtr<IInArchive> archive = Utility::GetArchiveReader(m_LibraryInstance, m_Property_CompressionFormat);
			CComPtr<InStreamWrapper> inFile = new InStreamWrapper(fileStream);
			CComPtr<ArchiveOpenCallback> openCallback = new ArchiveOpenCallback();

			HRESULT hr = archive->Open(inFile, 0, openCallback);
			if (hr == S_OK)
			{
				// List command
				UInt32 numItems = 0;
				archive->GetNumberOfItems(&numItems);
				for (UInt32 i = 0; i < numItems; i++)
				{
					// Get uncompressed size of file
					CPropVariant prop;
					archive->GetProperty(i, kpidSize, &prop);

					int size = prop.intVal;

					// Get name of file
					archive->GetProperty(i, kpidPath, &prop);

					// valid string? pass back the found value and call the callback function if set
					if (prop.vt == VT_BSTR)
					{
						WCHAR* path = prop.bstrVal;
						if (callback)
						{
							callback->OnFileFound(path, size);
						}
					}
				}
				CPropVariant prop;
				archive->GetArchiveProperty(kpidPath, &prop);
				archive->Close();

				if (prop.vt == VT_BSTR)
				{
					WCHAR* path = prop.bstrVal;
					if (callback)
					{
						callback->OnListingDone(path);
					}
				}
				return true;
			}
		}
		return false;
	}

	/* Extraction */
	bool SevenZipArchive::ExtractArchive(const TString& destDirectory, ProgressNotifier* notifier) const
	{
		return ExtractToDisk(destDirectory, NULL, TStringVector(), notifier);
	}
	bool SevenZipArchive::ExtractArchive(const IndexesVector& filesIndices, const TString& destDirectory, ProgressNotifier* notifier) const
	{
		return ExtractToDisk(destDirectory, &filesIndices, TStringVector(), notifier);
	}
	bool SevenZipArchive::ExtractArchive(const IndexesVector& filesIndices, const TStringVector& finalPaths, ProgressNotifier* notifier) const
	{
		return ExtractToDisk(TString(), &filesIndices, finalPaths, notifier);
	}
	bool SevenZipArchive::ExtractToMemory(const IndexesVector& filesIndices, DataBufferMap& bufferMap, ProgressNotifier* notifier) const
	{
		CComPtr<IStream> fileStream = FileSys::OpenFileToRead(m_ArchiveFilePath);
		if (fileStream != NULL)
		{
			CComPtr<IInArchive> archive = Utility::GetArchiveReader(m_LibraryInstance, m_Property_CompressionFormat);
			CComPtr<InStreamWrapper> inFile = new InStreamWrapper(fileStream);
			CComPtr<ArchiveOpenCallback> openCallback = new ArchiveOpenCallback();

			HRESULT hr = archive->Open(inFile, 0, openCallback);
			if (hr == S_OK)
			{
				IndexesVector newIndexes = RemoveInvalidIndexes(filesIndices);
				for (uint32_t index: newIndexes)
				{
					if (index < m_ItemOriginalSizes.size())
					{
						size_t requiredSize = m_ItemOriginalSizes[index];
						bufferMap[index] = DataBuffer(requiredSize, 0);
					}
				}

				CComPtr<ArchiveExtractCallbackMemory> extractCallback = new ArchiveExtractCallbackMemory(archive, bufferMap, notifier);
				hr = archive->Extract(newIndexes.data(), (UInt32)newIndexes.size(), false, extractCallback);
				if (hr == S_OK)
				{
					archive->Close();
					if (notifier)
					{
						notifier->OnDone(m_ArchiveFilePath);
					}
					return true;
				}
			}
		}
		return false;
	}
	bool SevenZipArchive::ExtractToMemory(uint32_t index, DataBuffer& buffer, ProgressNotifier* notifier) const
	{
		DataBufferMap bufferMap;
		bufferMap.insert_or_assign(index, DataBuffer());
		bool ret = ExtractToMemory({index}, bufferMap, notifier);

		buffer = bufferMap.at(index);
		return ret;
	}

	/* Compression */
	bool SevenZipArchive::CompressDirectory(const TString& directory, bool isRecursive, ProgressNotifier* notifier)
	{
		return FindAndCompressFiles(directory, AllFilesPattern, FileSys::GetPath(directory), isRecursive, notifier);
	}
	bool SevenZipArchive::CompressFiles(const TString& directory, const TString& searchFilter, bool isRecursive, ProgressNotifier* notifier)
	{
		return FindAndCompressFiles(directory, searchFilter, directory, isRecursive, notifier);
	}
	bool SevenZipArchive::CompressSpecifiedFiles(const TStringVector& sourceFiles, const TStringVector& archivePaths, ProgressNotifier* notifier)
	{
		FilePathInfoVector files;
		files.resize(sourceFiles.size());
		for (size_t i = 0; i < sourceFiles.size(); i++)
		{
			FilePathInfoVector infoArray = FileSys::GetFile(sourceFiles[i]);
			if (!infoArray.empty())
			{
				files[i] = infoArray.front();
			}
		}

		return CompressFilesToArchive(TString(), files, archivePaths, notifier);
	}
	bool SevenZipArchive::CompressFile(const TString& filePath, ProgressNotifier* notifier)
	{
		FilePathInfoVector files = FileSys::GetFile(filePath);
		if (!files.empty())
		{
			return CompressFilesToArchive(FileSys::GetPath(filePath), files, {TString()}, notifier);
		}
		return false;
	}
	bool SevenZipArchive::CompressFile(const TString& filePath, const TString& archivePath, ProgressNotifier* notifier)
	{
		FilePathInfoVector files = FileSys::GetFile(filePath);
		if (!files.empty())
		{
			return CompressFilesToArchive(FileSys::GetPath(filePath), files, {archivePath}, notifier);
		}
		return false;
	}
}
