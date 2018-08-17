#pragma once

#include "SevenZipLibrary.h"
#include <atlbase.h>
#include "FileInfo.h"
#include "CompressionFormat.h"
#include "CompressionLevel.h"

namespace SevenZip
{
	class SevenZipArchive
	{
	public:
		SevenZipArchive(const SevenZipLibrary& library, const TString& archivePath);
		virtual ~SevenZipArchive();

		virtual bool ReadInArchiveMetadata();

		virtual void SetProperty_CompressionFormat(const CompressionFormatEnum& format);
		virtual CompressionFormatEnum GetProperty_CompressionFormat();

		virtual void SetProperty_CompressionLevel(const CompressionLevelEnum& level);
		virtual CompressionLevelEnum GetProperty_CompressionLevel();

		virtual bool DetectCompressionFormat();

		virtual size_t GetNumberOfItems();
		virtual const std::vector<TString>& GetItemsNames();
		virtual const std::vector<size_t>&  GetOrigSizes();

	protected:
		bool m_IsMetaDataReaded = false;
		bool m_OverrideCompressionFormat = false;
		const SevenZipLibrary& m_LibraryInstance;
		TString m_ArchiveFilePath;
		CompressionFormatEnum m_Property_CompressionFormat;
		CompressionLevelEnum m_Property_CompressionLevel;
		size_t m_ItemCount = 0;
		std::vector<TString> m_ItemNames;
		std::vector<size_t> m_ItemOriginalSizes;

	private:
		bool pri_GetNumberOfItems();
		bool pri_GetItemsNames();
		bool pri_DetectCompressionFormat(CompressionFormatEnum & format);
		bool pri_DetectCompressionFormat();
	};
}
