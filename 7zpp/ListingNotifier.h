#pragma once
#include "SevenZipLibrary.h"
#include "CompressionFormat.h"

namespace SevenZip
{
	class ListingNotifier
	{
		public:
			virtual ~ListingNotifier() = default;

		public:
			// Called for each file found in the archive. Size in bytes.
			virtual void OnFileFound(const WCHAR* path, int64_t size) {}

			// Called when all the files have been listed
			virtual void OnListingDone(const WCHAR* path) {}
	};
}
