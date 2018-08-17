#pragma once
#include "SevenZipLibrary.h"
#include "CompressionFormat.h"
#include <7zip/Archive/IArchive.h>
#include <7zTypes.h>
#include "GUIDs.h"
#include "FileSys.h"
#include "ArchiveOpenCallback.h"
#include "InStreamWrapper.h"

namespace SevenZip
{
	using namespace intl;
	class ProgressNotifier;

	class Utility
	{
		public:
			static TString& ToLower(TString& string);
			static TString& ToUpper(TString& string);
			static TString ToLower(const TString& string)
			{
				TString s = string;
				return ToLower(s);
			}
			static TString ToUpper(const TString& string)
			{
				TString s = string;
				return ToUpper(s);
			}

			static const GUID* GetCompressionGUID(const CompressionFormatEnum& format);
			
			static CComPtr<IInArchive> GetArchiveReader(const SevenZipLibrary& library, const CompressionFormatEnum& format);
			static CComPtr<IOutArchive> GetArchiveWriter(const SevenZipLibrary& library, const CompressionFormatEnum& format);
			
			static bool DetectCompressionFormat(const SevenZipLibrary& library, const TString& archivePath, CompressionFormatEnum & archiveCompressionFormat, ProgressNotifier* notifier = NULL);
			static bool GetNumberOfItems(const SevenZipLibrary& library, const TString& archivePath, CompressionFormatEnum& format, size_t& itemCount, ProgressNotifier* notifier = NULL);
			static bool GetItemsNames(const SevenZipLibrary& library, const TString& archivePath, CompressionFormatEnum& format, size_t& itemCount, std::vector<TString>& itemnames, std::vector<size_t>& origsizes, ProgressNotifier* notifier = NULL);
			
			static TString EndingFromCompressionFormat(const CompressionFormatEnum& format);
			static CompressionFormatEnum CompressionFormatFromEnding(const TString& extWithDot);
	};
}
