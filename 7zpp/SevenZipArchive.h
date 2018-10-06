#pragma once
#include "CompressionFormat.h"
#include "CompressionLevel.h"
#include "FileInfo.h"

namespace SevenZip
{
	class SevenZipLibrary;
	class ProgressNotifier;
	class ListingNotifier;
	typedef std::vector<TString> TStringVector;
	typedef std::vector<uint32_t> IndexesVector;
	typedef std::vector<intl::FilePathInfo> FilePathInfoVector;
	typedef std::vector<uint8_t> DataBuffer;
	typedef std::unordered_map<size_t, DataBuffer> DataBufferMap;

	enum CompressionMethod
	{
		Unknown = -1,

		LZMA,
		LZMA2,
		PPMD,
		BZIP2,
	};

	class SevenZipArchive
	{
		public:
			TString GetExtensionFromFormat(CompressionFormatEnum nFormat);

		protected:
			const SevenZipLibrary& m_LibraryInstance;
			ProgressNotifier* m_Notifier = NULL;
			TString m_ArchiveFilePath;

			// Metadata
			bool m_IsMetaDataReaded = false;
			bool m_OverrideCompressionFormat = false;
			size_t m_ItemCount = 0;
			TStringVector m_ItemNames;
			std::vector<size_t> m_ItemOriginalSizes;

			// Properties
			CompressionFormatEnum m_Property_CompressionFormat = CompressionFormatEnum::Enum::Unknown;
			int m_Property_CompressionLevel = 5;
			int m_Property_DictionarySize = 5;
			CompressionMethod m_Property_CompressionMethod = CompressionMethod::LZMA;
			bool m_Property_Solid = false;
			bool m_Property_MultiThreaded = true;

		protected:
			// Metadata
			void InvalidateChace();
			bool InitNumberOfItems();
			bool InitItemsNames();
			bool DetectCompressionFormatHelper(CompressionFormatEnum& format);
			bool ReadInArchiveMetadataHelper();

			// Extraction
			bool ExtractToDisk(const TString& directory, const IndexesVector* filesIndices = NULL, const TStringVector& finalPaths = TStringVector(), ProgressNotifier* notifier = NULL) const;
			IndexesVector RemoveInvalidIndexes(const IndexesVector& tSourceArray) const;

			// Compression
			const char* GetMethodString() const;
			std::string FormatMethodString() const;
			bool SetCompressionProperties(IUnknown* pArchive);
			bool FindAndCompressFiles(const TString& directory, const TString& searchPattern, const TString& pathPrefix, bool recursion, ProgressNotifier* notifier);
			bool CompressFilesToArchive(const TString& pathPrefix, const FilePathInfoVector& filePaths, const TStringVector& inArchiveFilePaths, ProgressNotifier* notifier);

		public:
			SevenZipArchive(const SevenZipLibrary& library, const TString& archivePath, ProgressNotifier* notifier = NULL);
			virtual ~SevenZipArchive();

		public:
			// Metadata
			virtual bool ReadInArchiveMetadata();
			virtual bool DetectCompressionFormat();
			virtual size_t GetNumberOfItems();
			virtual const TStringVector& GetItemsNames();
			virtual const std::vector<size_t>& GetOrigSizes();

			// Properties
			virtual CompressionFormatEnum GetProperty_CompressionFormat();
			virtual void SetProperty_CompressionFormat(const CompressionFormatEnum& format);
			
			virtual int GetProperty_CompressionLevel() const
			{
				return m_Property_CompressionLevel;
			}
			virtual void SetProperty_CompressionLevel(int value)
			{
				m_Property_CompressionLevel = value;
			}
			
			virtual int GetProperty_DictionarySize() const
			{
				return m_Property_DictionarySize;
			}
			virtual void SetProperty_DictionarySize(int value)
			{
				m_Property_DictionarySize = value;
			}

			virtual CompressionMethod GetProperty_CompressionMethod() const
			{
				return m_Property_CompressionMethod;
			}
			virtual void SetProperty_CompressionMethod(CompressionMethod nMethod)
			{
				m_Property_CompressionMethod = nMethod;
			}
			
			virtual bool GetProperty_Solid() const
			{
				return m_Property_Solid;
			}
			virtual void SetProperty_Solid(bool isSolid)
			{
				m_Property_Solid = isSolid;
			}
			
			virtual bool GetProperty_MultiThreaded() const
			{
				return m_Property_MultiThreaded;
			}
			virtual void SetProperty_MultiThreaded(bool isMT)
			{
				m_Property_MultiThreaded = isMT;
			}

			/* Listing */
			virtual bool ListArchive(ListingNotifier* callback = NULL) const;

			/* Extraction */
			// Extract entire archive into specified directory
			virtual bool ExtractArchive(const TString& directory, ProgressNotifier* notifier = NULL) const;
			
			// Extract only specified files into directory
			virtual bool ExtractArchive(const IndexesVector& filesIndices, const TString& destDirectory, ProgressNotifier* notifier = NULL) const;

			// Extract only specified files into corresponding files path
			virtual bool ExtractArchive(const IndexesVector& filesIndices, const TStringVector& finalPaths, ProgressNotifier* notifier = NULL) const;
			
			// Extract specified file into memory buffer
			virtual bool ExtractToMemory(const IndexesVector& filesIndices, DataBufferMap& bufferMap, ProgressNotifier* notifier = NULL) const;
			virtual bool ExtractToMemory(uint32_t index, DataBuffer& buffer, ProgressNotifier* notifier = NULL) const;
	
			/* Compression */
			// Includes the last directory as the root in the archive, e.g. specifying "C:\Temp\MyFolder"
			// makes "MyFolder" the single root item in archive with the files within it included.
			virtual bool CompressDirectory(const TString& directory, bool bRecursive = true, ProgressNotifier* notifier = NULL);

			// Excludes the last directory as the root in the archive, its contents are at root instead. E.g.
			// specifying "C:\Temp\MyFolder" make the files in "MyFolder" the root items in the archive.
			virtual bool CompressFiles(const TString& directory, const TString& searchFilter = TString(), bool bRecursive = true, ProgressNotifier* notifier = NULL);
			virtual bool CompressSpecifiedFiles(const TStringVector& sourceFiles, const TStringVector& archivePaths, ProgressNotifier* notifier = NULL);

			// Compress just this single file as the root item in the archive.
			virtual bool CompressFile(const TString& filePath, ProgressNotifier* notifier = NULL);

			// Same as above, but places compressed file into 'archivePath' folder inside the archive
			virtual bool CompressFile(const TString& filePath, const TString& archivePath, ProgressNotifier* notifier = NULL);
	};
}
