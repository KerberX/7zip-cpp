#pragma once
#include <7zip/Archive/IArchive.h>
#include <7zTypes.h>
#include "Common.h"
#include "GUIDs.h"
#include "FileInfo.h"
#include "ArchiveOpenCallback.h"
#include "InStreamWrapper.h"
#include <optional>

namespace SevenZip
{
	class Library;
	class ProgressNotifier;
}

namespace SevenZip::Utility
{
	std::optional<GUID> GetCompressionGUID(CompressionFormat format);

	CComPtr<IInArchive> GetArchiveReader(const Library& library, CompressionFormat format);
	CComPtr<IOutArchive> GetArchiveWriter(const Library& library, CompressionFormat format);

	CompressionFormat GetCompressionFormat(const Library& library, const TString& archivePath, ProgressNotifier* notifier = nullptr);
	bool GetNumberOfItems(const Library& library, const TString& archivePath, CompressionFormat format, size_t& itemCount, ProgressNotifier* notifier = nullptr);
	
	std::optional<FileInfo> GetArchiveItem(const CComPtr<IInArchive>& archive, FileIndex fileIndex);
	bool GetArchiveItems(const Library& library, const TString& archivePath, CompressionFormat format, FileInfo::Vector& items, ProgressNotifier* notifier = nullptr);

	TString ExtensionFromCompressionFormat(CompressionFormat format);
	CompressionFormat CompressionFormatFromExtension(const TString& extWithDot);
	TString GetCompressionFormatName(CompressionFormat format);
}
