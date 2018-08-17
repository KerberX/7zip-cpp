#include "stdafx.h"
#include "SevenZipCompressor.h"
#include "GUIDs.h"
#include "FileSys.h"
#include "ArchiveUpdateCallback.h"
#include "OutStreamWrapper.h"
#include "PropVariant.h"
#include "UsefulFunctions.h"


namespace SevenZip
{
	using namespace intl;

	const TString AllFilesPattern = _T("*");

	SevenZipCompressor::SevenZipCompressor(const SevenZipLibrary& library, const TString& archivePath)
		:SevenZipArchive(library, archivePath)
	{
	}
	SevenZipCompressor::~SevenZipCompressor()
	{
	}

	bool SevenZipCompressor::CompressDirectory(const TString& directory, ProgressCallback* callback, bool includeSubdirs)
	{
		return FindAndCompressFiles(directory, AllFilesPattern, FileSys::GetPath(directory), includeSubdirs, callback);
	}
	bool SevenZipCompressor::CompressFiles(const TString& directory, const TString& searchFilter, ProgressCallback* callback, bool includeSubdirs)
	{
		return FindAndCompressFiles(directory, searchFilter, directory, includeSubdirs, callback);
	}
	bool SevenZipCompressor::CompressAllFiles(const TString& directory, ProgressCallback* callback, bool includeSubdirs)
	{
		return FindAndCompressFiles(directory, AllFilesPattern, directory, includeSubdirs, callback);
	}
	bool SevenZipCompressor::CompressFile(const TString& filePath, ProgressCallback* callback)
	{
		FilePathInfoVector files = FileSys::GetFile(filePath);
		if (!files.empty())
		{
			m_outputPath = filePath;
			return CompressFilesToArchive(TString(), files, callback);
		}
		return false;
	}

	CComPtr<IStream> SevenZipCompressor::OpenArchiveStream()
	{
		CComPtr< IStream > fileStream = FileSys::OpenFileToWrite(m_ArchiveFilePath);
		if (fileStream == NULL)
		{
			return NULL;
			//throw SevenZipException( StrFmt( _T( "Could not create archive \"%s\"" ), m_archivePath.c_str() ) );
		}
		return fileStream;
	}
	bool SevenZipCompressor::FindAndCompressFiles(const TString& directory, const TString& searchPattern, const TString& pathPrefix, bool recursion, ProgressCallback* callback)
	{
		if (!FileSys::DirectoryExists(directory))
		{
			return false;	//Directory does not exist
		}

		if (FileSys::IsDirectoryEmptyRecursive(directory))
		{
			return false;	//Directory \"%s\" is empty" ), directory.c_str()
		}

		m_outputPath = directory;

		FilePathInfoVector files = FileSys::GetFilesInDirectory(directory, searchPattern, recursion);
		return CompressFilesToArchive(pathPrefix, files, callback);
	}
	bool SevenZipCompressor::CompressFilesToArchive(const TString& pathPrefix, const FilePathInfoVector& filePaths, ProgressCallback* progressCallback)
	{
		CComPtr<IOutArchive> archiver = Utility::GetArchiveWriter(m_LibraryInstance, m_Property_CompressionFormat);
		SetCompressionProperties(archiver);

		//Set full outputFilePath including ending
		m_outputPath += Utility::EndingFromCompressionFormat(m_Property_CompressionFormat);

		CComPtr<OutStreamWrapper> outFile = new OutStreamWrapper(OpenArchiveStream());
		CComPtr<ArchiveUpdateCallback> updateCallback = new ArchiveUpdateCallback(pathPrefix, filePaths, m_outputPath, progressCallback);

		HRESULT hr = archiver->UpdateItems(outFile, (UInt32)filePaths.size(), updateCallback);

		if (progressCallback)
		{
			progressCallback->OnDone(m_outputPath);	//Todo: give full path support
		}

		if (hr != S_OK) // returning S_FALSE also indicates error
		{
			return false;
		}
		return true;
	}
	bool SevenZipCompressor::SetCompressionProperties(IUnknown* pArchive)
	{
		std::string sMethod = FormatMethodString();
		const wchar_t* tNames[] = {L"x", L"s", L"mt", L"m0"};
		CPropVariant tValues[] =
		{
			(UInt32)m_Property_CompressionLevel,
			GetProperty_Solid(),
			GetProperty_MultiThreaded(),
			sMethod.c_str()
		};

		CComPtr<ISetProperties> tSetter;
		pArchive->QueryInterface(IID_ISetProperties, reinterpret_cast<void**>(&tSetter));
		if (tSetter == NULL)
		{
			return false;	//Archive does not support setting compression properties
		}

		const size_t nPropertiesCount = ARRAYSIZE(tValues);
		HRESULT hr = tSetter->SetProperties(tNames, tValues, nPropertiesCount);
		if (hr != S_OK)
		{
			return false;
		}
		return true;
	}
}
