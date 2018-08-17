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
			virtual bool ExtractArchive(const TString& directory, ProgressNotifier* callback);
			virtual bool ExtractArchive(const IndiencesArray& tFilesIndices, const TString& directory, ProgressNotifier* callback);
			virtual bool ExtractToMemory(uint32_t nIndex, void* pBuffer, ProgressNotifier* callback);

		private:
			bool ExtractArchiveInternal(const CComPtr<IStream>& archiveStream, const TString& directory, ProgressNotifier* callback, const IndiencesArray* pFilesIndices = NULL);
	};
}
