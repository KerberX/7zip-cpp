#pragma once
#include "Common.h"
#include "FileInfo.h"

namespace SevenZip
{
	class Library;
	class ProgressNotifier;
	class ListingNotifier;

	namespace Callback
	{
		class Extractor;
	}
}

namespace SevenZip
{
	using FileIndexVector = std::vector<uint32_t>;

	class Archive
	{
		protected:
			const Library* m_Library = nullptr;
			ProgressNotifier* m_Notifier = nullptr;
			TString m_ArchivePath;

			// Metadata
			FileInfo::Vector m_Items;
			bool m_IsLoaded = false;
			bool m_OverrideCompressionFormat = false;

			// Properties
			CompressionFormat m_Property_CompressionFormat = CompressionFormat::Unknown;
			CompressionMethod m_Property_CompressionMethod = CompressionMethod::LZMA;
			int m_Property_CompressionLevel = (int)CompressionLevel::Normal;
			int m_Property_DictionarySize = 5;
			bool m_Property_Solid = false;
			bool m_Property_MultiThreaded = true;

		private:
			void InvalidateCache();
			bool InitCompressionFormat();
			bool InitItems();
			bool InitMetadata();

		protected:
			bool DoExtract(const FileIndexVector& files, Callback::Extractor& extractor) const;
			bool FindAndCompressFiles(const TString& directory, const TString& searchPattern, const TString& pathPrefix, bool recursion);
			bool CompressFilesToArchive(const TString& pathPrefix, const FilePathInfo::Vector& filePaths, const TStringVector& inArchiveFilePaths);

		public:
			Archive(const Library& library, ProgressNotifier* notifier = nullptr)
				:m_Library(&library), m_Notifier(notifier)
			{
			}
			Archive(const Library& library, TStringView filePath, ProgressNotifier* notifier = nullptr)
				:Archive(library, notifier)
			{
				Load(filePath);
			}
			Archive(const Archive&) = delete;
			Archive(Archive&& other)
			{
				*this = std::move(other);
			}
			virtual ~Archive() = default;

		public:
			bool Load(TStringView filePath);
			bool IsLoaded() const
			{
				return m_IsLoaded;
			}
			void SetNotifier(ProgressNotifier* notifier)
			{
				m_Notifier = notifier;
			}

			size_t GetItemCount() const
			{
				return m_Items.size();
			}
			const FileInfo::Vector& GetItems() const
			{
				return m_Items;
			}

		public:
			TString GetProperty_FilePath() const
			{
				return m_ArchivePath;
			}

			CompressionFormat GetProperty_CompressionFormat() const
			{
				return m_Property_CompressionFormat;
			}
			void SetProperty_CompressionFormat(CompressionFormat format)
			{
				m_OverrideCompressionFormat = true;
				InvalidateCache();
				m_Property_CompressionFormat = format;
			}
			
			int GetProperty_CompressionLevel() const
			{
				return m_Property_CompressionLevel;
			}
			void SetProperty_CompressionLevel(int value)
			{
				m_Property_CompressionLevel = value;
			}
			
			int GetProperty_DictionarySize() const
			{
				return m_Property_DictionarySize;
			}
			void SetProperty_DictionarySize(int value)
			{
				m_Property_DictionarySize = value;
			}

			CompressionMethod GetProperty_CompressionMethod() const
			{
				return m_Property_CompressionMethod;
			}
			void SetProperty_CompressionMethod(CompressionMethod nMethod)
			{
				m_Property_CompressionMethod = nMethod;
			}
			
			bool GetProperty_Solid() const
			{
				return m_Property_Solid;
			}
			void SetProperty_Solid(bool isSolid)
			{
				m_Property_Solid = isSolid;
			}
			
			bool GetProperty_MultiThreaded() const
			{
				return m_Property_MultiThreaded;
			}
			void SetProperty_MultiThreaded(bool isMT)
			{
				m_Property_MultiThreaded = isMT;
			}

		public:
			// Extracts specified files using provided extractor.
			// Extract entire archive if no files are specified.
			bool ExtractArchive(const FileIndexVector& files, Callback::Extractor& extractor) const;

			// Extract entire archive into specified directory
			bool ExtractArchive(const TString& directory) const;
			
			// Extract only specified files into directory. Retains relative paths.
			bool ExtractArchive(const FileIndexVector& files, const TString& directory) const;

			// Extract only specified files into corresponding files path.
			// Each file at index to the same file in 'finalPaths' vector.
			bool ExtractArchive(const FileIndexVector& files, const TStringVector& finalPaths) const;
	
		public:
			// Includes the last directory as the root in the archive, e.g. specifying "C:\Temp\MyFolder"
			// makes "MyFolder" the single root item in archive with the files within it included.
			bool CompressDirectory(const TString& directory, bool bRecursive = true);

			// Excludes the last directory as the root in the archive, its contents are at root instead. E.g.
			// specifying "C:\Temp\MyFolder" make the files in "MyFolder" the root items in the archive.
			bool CompressFiles(const TString& directory, const TString& searchFilter = TString(), bool recursive = true);
			bool CompressSpecifiedFiles(const TStringVector& sourceFiles, const TStringVector& archivePaths);

			// Compress just this single file as the root item in the archive.
			bool CompressFile(const TString& filePath);

			// Same as above, but places compressed file into 'archivePath' folder inside the archive
			bool CompressFile(const TString& filePath, const TString& archivePath);

		public:
			Archive& operator=(const Archive&) = delete;
			Archive& operator=(Archive&& other);

			explicit operator bool() const
			{
				return IsLoaded();
			}
			bool operator!() const
			{
				return !IsLoaded();
			}
	};
}
