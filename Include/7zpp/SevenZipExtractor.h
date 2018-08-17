#pragma once
#include "SevenZipLibrary.h"
#include "SevenZipArchive.h"
#include "CompressionFormat.h"
#include "ProgressCallback.h"


namespace SevenZip
{
	class SevenZipExtractor: public SevenZipArchive
	{
		public:
			typedef std::vector<uint32_t> IndiencesArray;

		public:
			SevenZipExtractor(const SevenZipLibrary& library, const TString& archivePath);
			virtual ~SevenZipExtractor();

		public:
			virtual bool ExtractArchive(const TString& directory, ProgressCallback* callback);
			virtual bool ExtractArchive(const IndiencesArray& tFilesIndices, const TString& directory, ProgressCallback* callback);
			virtual bool ExtractToMemory(uint32_t nIndex, void* pBuffer, ProgressCallback* callback);

		private:
			bool ExtractArchiveInternal(const CComPtr<IStream>& archiveStream, const TString& directory, ProgressCallback* callback, const IndiencesArray* pFilesIndices = NULL);
	};
}
