#pragma once
#include <vector>

namespace SevenZip
{
	struct FileInfo
	{
		using Vector = std::vector<FileInfo>;
		using RefVector = std::vector<FileInfo*>;

		TString	FileName;
		FILETIME LastWriteTime = {};
		FILETIME CreationTime = {};
		FILETIME LastAccessTime = {};
		int64_t Size = -1;
		int64_t CompressedSize = -1;
		uint32_t Attributes = 0;
		bool IsDirectory = false;
	};
	struct FilePathInfo: public FileInfo
	{
		using Vector = std::vector<FilePathInfo>;
		using RefVector = std::vector<FilePathInfo*>;

		TString	FilePath;
	};
}
