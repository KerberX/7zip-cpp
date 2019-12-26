#include "stdafx.h"
#include "SevenZipArchive.h"
#include "SevenZipLibrary.h"
#include <atlbase.h>
#include "FileInfo.h"
#include "Utility.h"
#include "GUIDs.h"
#include "FileSystem.h"
#include "Common.h"
#include "ArchiveOpenCallback.h"
#include "ArchiveExtractCallback.h"
#include "ArchiveUpdateCallback.h"
#include "ListingNotifier.h"
#include "ProgressNotifier.h"
#include "InStreamWrapper.h"
#include "OutStreamWrapper.h"
#include "VariantProperty.h"
#pragma warning(disable: 4101)

namespace
{
	constexpr auto AllFilesPattern = _T("*");
}

namespace SevenZip
{
	// Metadata
	void Archive::InvalidateCache()
	{
		m_IsMetaDataReaded = false;
	}

	bool Archive::DetectCompressionFormatHelper(CompressionFormatEnum& format)
	{
		m_OverrideCompressionFormat = false;
		return Utility::DetectCompressionFormat(m_Library, m_ArchiveFilePath, format, m_Notifier);
	}
	bool Archive::ReadInArchiveMetadataHelper()
	{
		bool detectedCompressionFormat = true;
		if (!m_OverrideCompressionFormat)
		{
			detectedCompressionFormat = DetectCompressionFormatHelper(m_Property_CompressionFormat);
		}
		bool gotItemNumberNamesAndOrigSizes = InitItemsNames();

		return detectedCompressionFormat && gotItemNumberNamesAndOrigSizes;
	}
	bool Archive::InitNumberOfItems()
	{
		return Utility::GetNumberOfItems(m_Library, m_ArchiveFilePath, m_Property_CompressionFormat, m_ItemCount, m_Notifier);
	}
	bool Archive::InitItemsNames()
	{
		return Utility::GetItemsNames(m_Library, m_ArchiveFilePath, m_Property_CompressionFormat, m_ItemCount, m_ItemNames, m_ItemOriginalSizes, m_Notifier);
	}

	// Extraction
	bool Archive::ExtractToDisk(const TString& destDirectory, const IndexVector* filesIndices, const TStringVector& finalPaths, ProgressNotifier* notifier) const
	{
		if (auto fileStream = FileSystem::OpenFileToRead(m_ArchiveFilePath))
		{
			auto archive = Utility::GetArchiveReader(m_Library, m_Property_CompressionFormat);
			auto inFile = CreateObject<InStreamWrapper>(fileStream);
			auto openCallback = CreateObject<Callback::OpenArchive>();

			if (SUCCEEDED(archive->Open(inFile, 0, openCallback)))
			{
				auto extractCallback = CreateObject<Callback::ExtractArchive>(archive, destDirectory, notifier);
				extractCallback->SetFinalPath(finalPaths);

				const uint32_t* indiencesData = filesIndices ? filesIndices->data() : nullptr;
				const uint32_t indiencesCount = filesIndices ? (uint32_t)filesIndices->size() : -1;

				if (SUCCEEDED(archive->Extract(indiencesData, indiencesCount, false, extractCallback)))
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
	IndexVector Archive::RemoveInvalidIndexes(const IndexVector& sourceArray) const
	{
		IndexVector newArray = sourceArray;
		newArray.erase(std::remove(newArray.begin(), newArray.end(), std::numeric_limits<IndexVector::value_type>::max()), newArray.end());
		return newArray;
	}

	// Compression
	const char* Archive::GetMethodString() const
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
	}
	std::string Archive::FormatMethodString() const
	{
		const int64_t dictSizeMB = 20 + m_Property_DictionarySize;

		std::string result(256, '\000');
		switch (m_Property_CompressionMethod)
		{
			case LZMA:
			case LZMA2:
			{
				sprintf_s(result.data(), result.size(), "%s:d=%lld", GetMethodString(), dictSizeMB);
				break;
			}
			case BZIP2:
			{
				result = "BZip2:d=900000b";
				break;
			}
			case PPMD:
			{
				sprintf_s(result.data(), result.size(), "%s:mem=%lld", GetMethodString(), dictSizeMB);
				break;
			}
		};
		return result;
	}
	bool Archive::SetCompressionProperties(IUnknown* pArchive)
	{
		std::string method = FormatMethodString();
		const wchar_t* names[] = {L"x", L"s", L"mt", L"m"};
		VariantProperty values[] =
		{
			(UInt32)GetProperty_CompressionLevel(),
			GetProperty_Solid(),
			//"100f10m",
			GetProperty_MultiThreaded(),
			method.c_str()
		};

		CComPtr<ISetProperties> setter;
		pArchive->QueryInterface(IID_ISetProperties, reinterpret_cast<void**>(&setter));
		if (setter == nullptr)
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
	bool Archive::FindAndCompressFiles(const TString& directory, const TString& searchPattern, const TString& pathPrefix, bool recursion, ProgressNotifier* notifier)
	{
		if (!FileSystem::DirectoryExists(directory) || FileSystem::IsDirectoryEmptyRecursive(directory))
		{
			return false;
		}

		FilePathInfo::Vector files = FileSystem::GetFilesInDirectory(directory, searchPattern.empty() ? AllFilesPattern : searchPattern, recursion);
		TStringVector relativePaths(files.size(), TString());
		
		for (size_t i = 0; i < files.size(); i++)
		{
			relativePaths[i] = FileSystem::ExtractRelativePath(directory, files[i].FilePath);
		}
		return CompressFilesToArchive(pathPrefix, files, relativePaths, notifier);
	}
	bool Archive::CompressFilesToArchive(const TString& pathPrefix, const FilePathInfo::Vector& filePaths, const TStringVector& inArchiveFilePaths, ProgressNotifier* notifier)
	{
		auto archiveWriter = Utility::GetArchiveWriter(m_Library, m_Property_CompressionFormat);
		SetCompressionProperties(archiveWriter);

		auto outFile = CreateObject<OutStreamWrapper>(FileSystem::OpenFileToWrite(m_ArchiveFilePath));
		auto updateCallback = CreateObject<Callback::UpdateArchive>(pathPrefix, filePaths, inArchiveFilePaths, m_ArchiveFilePath, notifier);
		updateCallback->SetExistingItemsCount(m_ItemCount);

		if (SUCCEEDED(archiveWriter->UpdateItems(outFile, (UInt32)filePaths.size(), updateCallback)))
		{
			if (notifier)
			{
				notifier->OnDone();
			}
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Constructor and destructor
	Archive::Archive(const Library& library, const TString& archivePath, ProgressNotifier* notifier)
		:m_Library(library),
		m_Notifier(notifier),
		m_ArchiveFilePath(archivePath),
		m_Property_CompressionFormat(CompressionFormat::Unknown),
		m_Property_CompressionLevel(CompressionLevel::None)
	{
	}

	// Metadata
	bool Archive::ReadInArchiveMetadata()
	{
		m_IsMetaDataReaded = true;
		return ReadInArchiveMetadataHelper();
	}
	bool Archive::DetectCompressionFormat()
	{
		m_OverrideCompressionFormat = false;
		return DetectCompressionFormatHelper(m_Property_CompressionFormat);
	}
	size_t Archive::GetNumberOfItems()
	{
		if (!m_IsMetaDataReaded)
		{
			m_IsMetaDataReaded = ReadInArchiveMetadataHelper();
		}
		return m_ItemCount;
	}
	const TStringVector& Archive::GetItemsNames()
	{
		if (!m_IsMetaDataReaded)
		{
			m_IsMetaDataReaded = ReadInArchiveMetadataHelper();
		}
		return m_ItemNames;
	}
	const std::vector<size_t>& Archive::GetOrigSizes()
	{
		if (!m_IsMetaDataReaded)
		{
			m_IsMetaDataReaded = ReadInArchiveMetadataHelper();
		}
		return m_ItemOriginalSizes;
	}

	// Properties
	void Archive::SetProperty_CompressionFormat(const CompressionFormatEnum& format)
	{
		m_OverrideCompressionFormat = true;
		InvalidateCache();
		m_Property_CompressionFormat = format;
	}
	CompressionFormatEnum Archive::GetProperty_CompressionFormat()
	{
		if (!m_IsMetaDataReaded && !m_OverrideCompressionFormat)
		{
			m_IsMetaDataReaded = ReadInArchiveMetadataHelper();
		}
		return m_Property_CompressionFormat;
	}

	// Listing
	bool Archive::ListArchive(ListingNotifier* callback) const
	{
		if (auto fileStream = FileSystem::OpenFileToRead(m_ArchiveFilePath))
		{
			auto archive = Utility::GetArchiveReader(m_Library, m_Property_CompressionFormat);
			auto inFile = CreateObject<InStreamWrapper>(fileStream);
			auto openCallback = CreateObject<Callback::OpenArchive>();

			if (SUCCEEDED(archive->Open(inFile, nullptr, openCallback)))
			{
				// List command
				UInt32 numItems = 0;
				archive->GetNumberOfItems(&numItems);
				for (UInt32 i = 0; i < numItems; i++)
				{
					// Get uncompressed size of file
					VariantProperty prop;
					archive->GetProperty(i, kpidSize, &prop);

					int size = prop.intVal;

					// Get name of file
					archive->GetProperty(i, kpidPath, &prop);

					// Valid string? Pass back the found value and call the callback function if set
					if (prop.vt == VT_BSTR)
					{
						const wchar_t* path = prop.bstrVal;
						if (callback)
						{
							callback->OnFileFound(path, size);
						}
					}
				}

				VariantProperty prop;
				archive->GetArchiveProperty(kpidPath, &prop);
				archive->Close();

				if (prop.vt == VT_BSTR)
				{
					const wchar_t* path = prop.bstrVal;
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

	// Extraction
	bool Archive::ExtractArchive(const TString& destDirectory, ProgressNotifier* notifier) const
	{
		return ExtractToDisk(destDirectory, nullptr, TStringVector(), notifier);
	}
	bool Archive::ExtractArchive(const IndexVector& filesIndices, const TString& destDirectory, ProgressNotifier* notifier) const
	{
		return ExtractToDisk(destDirectory, &filesIndices, TStringVector(), notifier);
	}
	bool Archive::ExtractArchive(const IndexVector& filesIndices, const TStringVector& finalPaths, ProgressNotifier* notifier) const
	{
		return ExtractToDisk(TString(), &filesIndices, finalPaths, notifier);
	}
	bool Archive::ExtractToMemory(const IndexVector& filesIndices, DataBufferMap& bufferMap, ProgressNotifier* notifier) const
	{
		if (auto fileStream = FileSystem::OpenFileToRead(m_ArchiveFilePath))
		{
			auto archive = Utility::GetArchiveReader(m_Library, m_Property_CompressionFormat);
			auto inFile = CreateObject<InStreamWrapper>(fileStream, notifier);
			auto openCallback = CreateObject<Callback::OpenArchive>();

			if (SUCCEEDED(archive->Open(inFile, nullptr, openCallback)))
			{
				IndexVector newIndexes = RemoveInvalidIndexes(filesIndices);
				for (uint32_t index: newIndexes)
				{
					if (index < m_ItemOriginalSizes.size())
					{
						size_t requiredSize = m_ItemOriginalSizes[index];
						bufferMap[index] = DataBuffer(requiredSize, 0);
					}
				}

				auto extractCallback = CreateObject<Callback::ExtractArchiveToBuffer>(archive, bufferMap, notifier);
				if (SUCCEEDED(archive->Extract(newIndexes.data(), (UInt32)newIndexes.size(), false, extractCallback)))
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
	bool Archive::ExtractToMemory(uint32_t index, DataBuffer& buffer, ProgressNotifier* notifier) const
	{
		DataBufferMap bufferMap;
		bufferMap.insert_or_assign(index, DataBuffer());
		bool ret = ExtractToMemory({index}, bufferMap, notifier);

		buffer = bufferMap.at(index);
		return ret;
	}

	// Compression
	bool Archive::CompressDirectory(const TString& directory, bool isRecursive, ProgressNotifier* notifier)
	{
		return FindAndCompressFiles(directory, AllFilesPattern, FileSystem::GetPath(directory), isRecursive, notifier);
	}
	bool Archive::CompressFiles(const TString& directory, const TString& searchFilter, bool isRecursive, ProgressNotifier* notifier)
	{
		return FindAndCompressFiles(directory, searchFilter, directory, isRecursive, notifier);
	}
	bool Archive::CompressSpecifiedFiles(const TStringVector& sourceFiles, const TStringVector& archivePaths, ProgressNotifier* notifier)
	{
		FilePathInfo::Vector files;
		files.resize(sourceFiles.size());
		for (size_t i = 0; i < sourceFiles.size(); i++)
		{
			FilePathInfo::Vector infoArray = FileSystem::GetFile(sourceFiles[i]);
			if (!infoArray.empty())
			{
				files[i] = infoArray.front();
			}
		}

		return CompressFilesToArchive(TString(), files, archivePaths, notifier);
	}
	bool Archive::CompressFile(const TString& filePath, ProgressNotifier* notifier)
	{
		FilePathInfo::Vector files = FileSystem::GetFile(filePath);
		if (!files.empty())
		{
			return CompressFilesToArchive(FileSystem::GetPath(filePath), files, {TString()}, notifier);
		}
		return false;
	}
	bool Archive::CompressFile(const TString& filePath, const TString& archivePath, ProgressNotifier* notifier)
	{
		FilePathInfo::Vector files = FileSystem::GetFile(filePath);
		if (!files.empty())
		{
			return CompressFilesToArchive(FileSystem::GetPath(filePath), files, {archivePath}, notifier);
		}
		return false;
	}
}
