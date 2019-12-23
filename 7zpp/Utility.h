#pragma once
#include "SevenZipLibrary.h"
#include "CompressionFormat.h"
#include <7zip/Archive/IArchive.h>
#include <7zTypes.h>
#include "GUIDs.h"
#include "FileSystem.h"
#include "ArchiveOpenCallback.h"
#include "InStreamWrapper.h"

namespace SevenZip
{
	class ProgressNotifier;
}

namespace SevenZip::Utility
{
	TString& MakeLower(TString& string);
	TString& MakeUpper(TString& string);
	inline TString ToLower(const TString& string)
	{
		TString s = string;
		return MakeLower(s);
	}
	inline TString ToUpper(const TString& string)
	{
		TString s = string;
		return MakeUpper(s);
	}

	std::optional<GUID> GetCompressionGUID(const CompressionFormatEnum& format);

	CComPtr<IInArchive> GetArchiveReader(const Library& library, const CompressionFormatEnum& format);
	CComPtr<IOutArchive> GetArchiveWriter(const Library& library, const CompressionFormatEnum& format);

	bool DetectCompressionFormat(const Library& library, const TString& archivePath, CompressionFormatEnum& archiveCompressionFormat, ProgressNotifier* notifier = nullptr);
	bool GetNumberOfItems(const Library& library, const TString& archivePath, CompressionFormatEnum& format, size_t& itemCount, ProgressNotifier* notifier = nullptr);
	bool GetItemsNames(const Library& library, const TString& archivePath, CompressionFormatEnum& format, size_t& itemCount, std::vector<TString>& itemnames, std::vector<size_t>& origsizes, ProgressNotifier* notifier = nullptr);

	TString ExtensionFromCompressionFormat(const CompressionFormatEnum& format);
	CompressionFormatEnum CompressionFormatFromExtension(const TString& extWithDot);
}
