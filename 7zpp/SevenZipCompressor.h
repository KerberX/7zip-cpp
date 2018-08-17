#pragma once
#include <vector>
#include <atlbase.h>
#include "SevenZipLibrary.h"
#include "SevenZipArchive.h"
#include "FileInfo.h"
#include "CompressionFormat.h"
#include "CompressionLevel.h"
#include "ProgressCallback.h"

namespace SevenZip
{
	class SevenZipCompressor: public SevenZipArchive
	{
		private:
			typedef std::vector<intl::FilePathInfo> FilePathInfoVector;

		private:
			TString m_outputPath; // The final compression result compression path. Used for tracking in callbacks

		private:
			CComPtr<IStream> OpenArchiveStream();
			bool FindAndCompressFiles(const TString& directory, const TString& searchPattern, const TString& pathPrefix, bool recursion, ProgressNotifier* callback);
			bool CompressFilesToArchive(const TString& pathPrefix, const FilePathInfoVector& filePaths, ProgressNotifier* callback);
			
			bool SetCompressionProperties(IUnknown* pArchive);

		public:
			SevenZipCompressor(const SevenZipLibrary& library, const TString& archivePath);
			virtual ~SevenZipCompressor();

			// Includes the last directory as the root in the archive, e.g. specifying "C:\Temp\MyFolder"
			// makes "MyFolder" the single root item in archive with the files within it included.
			virtual bool CompressDirectory(const TString& directory, ProgressNotifier* callback, bool includeSubdirs = true);

			// Excludes the last directory as the root in the archive, its contents are at root instead. E.g.
			// specifying "C:\Temp\MyFolder" make the files in "MyFolder" the root items in the archive.
			virtual bool CompressFiles(const TString& directory, const TString& searchFilter, ProgressNotifier* callback, bool includeSubdirs = true);
			virtual bool CompressAllFiles(const TString& directory, ProgressNotifier* callback, bool includeSubdirs = true);

			// Compress just this single file as the root item in the archive.
			virtual bool CompressFile(const TString& filePath, ProgressNotifier* callback);
	};
}
