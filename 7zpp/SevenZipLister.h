#pragma once

#include "SevenZipLibrary.h"
#include "SevenZipArchive.h"
#include "CompressionFormat.h"
#include "ArchiveListCallback.h"


namespace SevenZip
{
	class SevenZipLister: public SevenZipArchive
	{
		public:

		SevenZipLister(const SevenZipLibrary& library, const TString& archivePath);
		virtual ~SevenZipLister();

		virtual bool ListArchive(ListingNotifier* callback);

		private:
		bool ListArchive(const CComPtr< IStream >& archiveStream, ListingNotifier* callback);
	};
}
