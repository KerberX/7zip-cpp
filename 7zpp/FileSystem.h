#pragma once
#include <vector>
#include "FileInfo.h"

namespace SevenZip::FileSystem
{
	TString GetPath(const TString& filePath);
	TString GetFileName(const TString& filePathOrName);
	TString AppendPath(const TString& left, const TString& right);
	TString ExtractRelativePath(const TString& basePath, const TString& fullPath);

	bool FileExists(const TString& path);
	bool DirectoryExists(const TString& path);
	bool IsDirectoryEmptyRecursive(const TString& path);
	bool CreateDirectoryTree(const TString& path);

	std::vector<FilePathInfo> GetFile(const TString& filePathOrName);
	std::vector<FilePathInfo> GetFilesInDirectory(const TString& directory, const TString& searchPattern, bool recursive);

	CComPtr<IStream> OpenFileToRW(const TString& filePath);
	CComPtr<IStream> OpenFileToRead(const TString& filePath);
	CComPtr<IStream> OpenFileToWrite(const TString& filePath);
}
