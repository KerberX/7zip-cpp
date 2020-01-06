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
#include "ProgressNotifier.h"
#include "InStreamWrapper.h"
#include "OutStreamWrapper.h"
#include "VariantProperty.h"
#pragma warning(disable: 4101)

namespace
{
	constexpr auto AllFilesPattern = _T("*");

	const char* GetMethodString(SevenZip::CompressionMethod method)
	{
		using namespace SevenZip;

		switch (method)
		{
			case CompressionMethod::LZMA:
			{
				return "LZMA";
			}
			case CompressionMethod::BZIP2:
			{
				return "BZip2";
			}
			case CompressionMethod::PPMD:
			{
				return "PPMd";
			}
		};

		// LZMA2 is the default
		return "LZMA2";
	}
	std::string FormatMethodString(int dictionarySize, SevenZip::CompressionMethod method)
	{
		using namespace SevenZip;

		const int64_t dictSizeMB = 20 + dictionarySize;

		std::string result(256, '\000');
		switch (method)
		{
			case CompressionMethod::LZMA:
			case CompressionMethod::LZMA2:
			{
				sprintf_s(result.data(), result.size(), "%s:d=%lld", GetMethodString(method), dictSizeMB);
				break;
			}
			case CompressionMethod::BZIP2:
			{
				result = "BZip2:d=900000b";
				break;
			}
			case CompressionMethod::PPMD:
			{
				sprintf_s(result.data(), result.size(), "%s:mem=%lld", GetMethodString(method), dictSizeMB);
				break;
			}
		};
		return result;
	}
	bool SetCompressionProperties(IUnknown& archive, bool multithreaded, bool solidArchive, int compressionLevel, int dictionarySize, SevenZip::CompressionMethod method)
	{
		using namespace SevenZip;

		std::string methodString = FormatMethodString(dictionarySize, method);
		constexpr wchar_t* names[] = {L"x", L"s", L"mt", L"m"};
		VariantProperty values[] =
		{
			static_cast<UInt32>(compressionLevel),
			solidArchive,
			multithreaded,
			methodString.c_str()
		};

		CComPtr<ISetProperties> propertiesSet;
		archive.QueryInterface(SevenZip::IID_ISetProperties, reinterpret_cast<void**>(&propertiesSet));
		if (!propertiesSet)
		{
			// Archive does not support setting compression properties
			return false;
		}

		return SUCCEEDED(propertiesSet->SetProperties(names, values, ARRAYSIZE(values)));
	}
}

namespace SevenZip
{
	void Archive::InvalidateCache()
	{
		m_IsLoaded = false;
	}
	bool Archive::InitCompressionFormat()
	{
		m_OverrideCompressionFormat = false;
		m_Property_CompressionFormat = Utility::GetCompressionFormat(*m_Library, m_ArchivePath, m_Notifier);

		return m_Property_CompressionFormat != CompressionFormat::Unknown;
	}
	bool Archive::InitItems()
	{
		return Utility::GetArchiveItems(*m_Library, m_ArchivePath, m_Property_CompressionFormat, m_Items, m_Notifier);
	}
	bool Archive::InitMetadata()
	{
		if (!m_IsLoaded)
		{
			const bool detectedFormat = !m_OverrideCompressionFormat ? InitCompressionFormat() : true;
			m_IsLoaded = detectedFormat && InitItems();
		}
		return m_IsLoaded;
	}

	bool Archive::DoExtract(const CComPtr<Callback::Extractor>& extractor, const FileIndexView* files) const
	{
		if (files && files->empty())
		{
			return false;
		}

		if (auto fileStream = FileSystem::OpenFileToRead(m_ArchivePath))
		{
			auto archive = Utility::GetArchiveReader(*m_Library, m_Property_CompressionFormat);
			auto inFile = CreateObject<InStreamWrapper>(fileStream, m_Notifier);
			auto openCallback = CreateObject<Callback::OpenArchive>(m_ArchivePath, m_Notifier);

			if (SUCCEEDED(archive->Open(inFile, nullptr, openCallback)))
			{
				CallAtExit atExit([&]()
				{
					archive->Close();
				});
				extractor->SetArchive(archive);
				extractor->SetNotifier(m_Notifier);

				HRESULT result = E_INVALIDARG;
				if (files)
				{
					auto IsInvalidIndex = [this](FileIndex index)
					{
						return index == InvalidFileIndex || index >= m_Items.size();
					};

					// Process only specified files
					if (files->size() == 1)
					{
						// No need to sort single index
						if (IsInvalidIndex(*files->data()))
						{
							return false;
						}
						result = archive->Extract(files->data(), static_cast<UInt32>(files->size()), false, extractor);
					}
					else
					{
						// IInArchive::Extract requires sorted array
						FileIndexVector temp = files->CopyToVector();
						std::sort(temp.begin(), temp.end());

						// Remove invalid items
						temp.erase(std::remove_if(temp.begin(), temp.end(), IsInvalidIndex), temp.end());

						if (!temp.empty())
						{
							result = archive->Extract(temp.data(), static_cast<UInt32>(temp.size()), false, extractor);
						}
					}
				}
				else
				{
					// Process all files, the callback will decide which files are needed
					result = archive->Extract(nullptr, std::numeric_limits<UInt32>::max(), false, extractor);
				}

				return SUCCEEDED(result);
			}
		}
		return false;
	}
	bool Archive::DoCompress(const TString& pathPrefix, const FilePathInfo::Vector& filePaths, const TStringVector& inArchiveFilePaths)
	{
		auto archiveWriter = Utility::GetArchiveWriter(*m_Library, m_Property_CompressionFormat);
		SetCompressionProperties(*archiveWriter, m_Property_MultiThreaded, m_Property_Solid, m_Property_CompressionLevel, m_Property_DictionarySize, m_Property_CompressionMethod);

		auto outFile = CreateObject<OutStreamWrapper_IStream>(FileSystem::OpenFileToWrite(m_ArchivePath));
		auto updateCallback = CreateObject<Callback::UpdateArchiveBase>(pathPrefix, filePaths, inArchiveFilePaths, m_ArchivePath, m_Notifier);
		updateCallback->SetExistingItemsCount(m_Items.size());

		return SUCCEEDED(archiveWriter->UpdateItems(outFile, static_cast<UInt32>(filePaths.size()), updateCallback));
	}
	bool Archive::FindAndCompressFiles(const TString& directory, const TString& searchPattern, const TString& pathPrefix, bool recursion)
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
		return DoCompress(pathPrefix, files, relativePaths);
	}

	bool Archive::Load(TStringView filePath)
	{
		// Clear metadata
		*this = std::move(Archive(*m_Library, m_Notifier));

		// Load new archive
		m_ArchivePath = filePath;
		m_IsLoaded = InitMetadata();

		return m_IsLoaded;
	}
	
	// Extraction
	bool Archive::Extract(const CComPtr<Callback::Extractor>& extractor) const
	{
		return DoExtract(extractor, nullptr);
	}
	bool Archive::Extract(const CComPtr<Callback::Extractor>& extractor, FileIndexView files) const
	{
		return DoExtract(extractor, &files);
	}

	bool Archive::ExtractToDirectory(const TString& directory) const
	{
		auto extractor = CreateObject<Callback::FileExtractor>(directory, m_Notifier);
		return DoExtract(extractor.Detach(), nullptr);
	}
	bool Archive::ExtractToDirectory(const TString& directory, FileIndexView files) const
	{
		auto extractor = CreateObject<Callback::FileExtractor>(directory, m_Notifier);
		return DoExtract(extractor.Detach(), &files);
	}

	// Compression
	bool Archive::CompressDirectory(const TString& directory, bool isRecursive)
	{
		return FindAndCompressFiles(directory, AllFilesPattern, FileSystem::GetPath(directory), isRecursive);
	}
	bool Archive::CompressFiles(const TString& directory, const TString& searchFilter, bool recursive)
	{
		return FindAndCompressFiles(directory, searchFilter, directory, recursive);
	}
	bool Archive::CompressSpecifiedFiles(const TStringVector& sourceFiles, const TStringVector& archivePaths)
	{
		FilePathInfo::Vector files;
		files.resize(sourceFiles.size());
		for (size_t i = 0; i < sourceFiles.size(); i++)
		{
			FilePathInfo::Vector infoArray = FileSystem::GetFile(sourceFiles[i]);
			if (!infoArray.empty())
			{
				files[i] = std::move(infoArray.front());
			}
		}

		return DoCompress(TString(), files, archivePaths);
	}
	bool Archive::CompressFile(const TString& filePath)
	{
		FilePathInfo::Vector files = FileSystem::GetFile(filePath);
		if (!files.empty())
		{
			return DoCompress(FileSystem::GetPath(filePath), files, {});
		}
		return false;
	}
	bool Archive::CompressFile(const TString& filePath, const TString& archivePath)
	{
		FilePathInfo::Vector files = FileSystem::GetFile(filePath);
		if (!files.empty())
		{
			return DoCompress(FileSystem::GetPath(filePath), files, {archivePath});
		}
		return false;
	}

	Archive& Archive::operator=(Archive&& other)
	{
		Archive nullObject(*m_Library);

		ExchangeAndReset(m_Library, other.m_Library, nullptr);
		ExchangeAndReset(m_Notifier, other.m_Notifier, nullObject.m_Notifier);
		m_ArchivePath = std::move(other.m_ArchivePath);

		// Metadata
		m_Items = std::move(nullObject.m_Items);
		ExchangeAndReset(m_IsLoaded, other.m_IsLoaded, nullObject.m_IsLoaded);
		ExchangeAndReset(m_OverrideCompressionFormat, other.m_OverrideCompressionFormat, nullObject.m_OverrideCompressionFormat);

		// Properties
		ExchangeAndReset(m_Property_CompressionFormat, other.m_Property_CompressionFormat, nullObject.m_Property_CompressionFormat);
		ExchangeAndReset(m_Property_CompressionMethod, other.m_Property_CompressionMethod, nullObject.m_Property_CompressionMethod);
		ExchangeAndReset(m_Property_CompressionLevel, other.m_Property_CompressionLevel, nullObject.m_Property_CompressionLevel);
		ExchangeAndReset(m_Property_DictionarySize, other.m_Property_DictionarySize, nullObject.m_Property_DictionarySize);
		ExchangeAndReset(m_Property_MultiThreaded, other.m_Property_MultiThreaded, nullObject.m_Property_MultiThreaded);
		ExchangeAndReset(m_Property_Solid, other.m_Property_Solid, nullObject.m_Property_Solid);

		return *this;
	}
}
