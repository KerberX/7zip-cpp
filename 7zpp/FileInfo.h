#pragma once

namespace SevenZip
{
	struct FileInfo
	{
		TString	FileName;
		FILETIME LastWriteTime = {};
		FILETIME CreationTime = {};
		FILETIME LastAccessTime = {};
		int64_t Size = -1;
		uint32_t Attributes = 0;
		bool IsDirectory = false;
	};
	struct FilePathInfo: public FileInfo
	{
		TString	FilePath;
	};
}
