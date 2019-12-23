#include "StdAfx.h"
#include "PathScanner.h"
#include "FileSystem.h"

namespace SevenZip::PathScanner::Internal
{
	bool IsAllFilesPattern(const TString& searchPattern)
	{
		return searchPattern == _T("*") || searchPattern == _T("*.*");
	}
	bool IsSpecialFileName(const TString& fileName)
	{
		return fileName == _T(".") || fileName == _T("..");
	}
	bool IsDirectory(const WIN32_FIND_DATA& fileData)
	{
		return (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	FilePathInfo ConvertFindInfo(const TString& directory, const WIN32_FIND_DATA& fileData)
	{
		FilePathInfo file;
		file.FileName = fileData.cFileName;
		file.FilePath = FileSystem::AppendPath(directory, file.FileName);
		file.LastWriteTime = fileData.ftLastWriteTime;
		file.CreationTime = fileData.ftCreationTime;
		file.LastAccessTime = fileData.ftLastAccessTime;
		file.Attributes = fileData.dwFileAttributes;
		file.IsDirectory = IsDirectory(fileData);

		ULARGE_INTEGER size;
		size.LowPart = fileData.nFileSizeLow;
		size.HighPart = fileData.nFileSizeHigh;
		file.Size = size.QuadPart;

		return file;
	}
	
	bool SearchFiles(const TString& directory, const TString& searchPattern, Callback& callback)
	{
		TString searchQuery = FileSystem::AppendPath(directory, searchPattern);
		bool exit = false;

		WIN32_FIND_DATA findData = {};
		HANDLE handle = ::FindFirstFile(searchQuery.c_str(), &findData);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return exit;
		}

		callback.EnterDirectory(directory);

		do
		{
			FilePathInfo fileInfo = ConvertFindInfo(directory, findData);
			if (!fileInfo.IsDirectory && !IsSpecialFileName(fileInfo.FileName))
			{
				callback.ExamineFile(fileInfo, exit);
			}
		}
		while (!exit && ::FindNextFile(handle, &findData));

		if (!exit)
		{
			callback.LeaveDirectory(directory);
		}

		::FindClose(handle);
		return exit;
	}
	void SearchDirectories(const TString& directory, std::deque<TString>& subDirectories, Callback& callback)
	{
		TString searchQuery = FileSystem::AppendPath(directory, _T("*"));

		WIN32_FIND_DATA findData = {};
		HANDLE handle = ::FindFirstFile(searchQuery.c_str(), &findData);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		do
		{
			FilePathInfo fileInfo = ConvertFindInfo(directory, findData);
			if (fileInfo.IsDirectory && !IsSpecialFileName(fileInfo.FileName) && callback.ShouldDescend(fileInfo))
			{
				subDirectories.emplace_back(std::move(fileInfo.FilePath));
			}
		}
		while (::FindNextFile(handle, &findData));

		::FindClose(handle);
	}
}

namespace SevenZip::PathScanner
{
	void Scan(const TString& root, Callback& callback)
	{
		Scan(root, _T("*"), callback);
	}
	void Scan(const TString& root, const TString& searchPattern, Callback& callback)
	{
		std::deque<TString> directories;
		directories.push_back(root);

		while (!directories.empty())
		{
			TString directory = directories.front();
			directories.pop_front();

			if (Internal::SearchFiles(directory, searchPattern, callback))
			{
				break;
			}
			Internal::SearchDirectories(directory, directories, callback);
		}
	}
}
