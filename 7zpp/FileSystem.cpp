#include "stdafx.h"
#include "FileSystem.h"
#include "PathScanner.h"
#include <ShlObj.h>

namespace SevenZip::FileSystem
{
	class ScannerCallback: public PathScanner::Callback
	{
		private:
			std::vector<FilePathInfo> m_Files;
			bool m_IsRecursive = false;
			bool m_FirstOnly = false;

		public:
			ScannerCallback(bool recursive, bool firstOnly = false)
				:m_IsRecursive(recursive), m_FirstOnly(firstOnly)
			{
			}

		public:
			std::vector<FilePathInfo> GetFiles() const
			{
				return std::move(m_Files);
			}

			bool ShouldDescend(const FilePathInfo& directory) override
			{
				return m_IsRecursive;
			}
			void ExamineFile(const FilePathInfo& file, bool& exit) override
			{
				m_Files.push_back(file);
				if (m_FirstOnly)
				{
					exit = true;
				}
			}
	};

	class IsEmptyCallback: public PathScanner::Callback
	{
		private:
			bool m_IsEmpty = true;

		public:
			IsEmptyCallback() = default;

		public:
			bool IsEmpty() const
			{
				return m_IsEmpty;
			}
			
			bool ShouldDescend(const FilePathInfo& directory) override
			{
				return true;
			}
			void ExamineFile(const FilePathInfo& file, bool& exit) override
			{
				m_IsEmpty = false;
				exit = true;
			}
	};
}

namespace SevenZip::FileSystem
{
	TString GetPath(const TString& filePath)
	{
		// Find the last "\" or "/" in the string and return it and everything before it
		size_t index = filePath.rfind(_T('\\'));
		size_t index2 = filePath.rfind(_T('/'));

		if (index2 != std::string::npos && index2 > index)
		{
			index = index2;
		}

		if (index == std::string::npos)
		{
			// No path separator
			return TString();
		}
		else if (index + 1 >= filePath.size())
		{
			// Last char is path sep, the whole thing is a path
			return filePath;
		}
		else
		{
			return filePath.substr(0, index + 1);
		}
	}
	TString GetFileName(const TString& filePathOrName)
	{
		// Find the last "\" or "/" in the string and return everything after it
		size_t index = filePathOrName.rfind(_T('\\'));
		size_t index2 = filePathOrName.rfind(_T('/'));

		if (index2 != std::string::npos && index2 > index)
		{
			index = index2;
		}

		if (index == std::string::npos)
		{
			// No path separator, return the whole thing
			return filePathOrName;
		}
		else if (index + 1 >= filePathOrName.size())
		{
			// Last char is path separator, no filename
			return {};
		}
		else
		{
			return filePathOrName.substr(index + 1, filePathOrName.size() - (index + 1));
		}
	}
	TString AppendPath(const TString& left, const TString& right)
	{
		if (left.empty())
		{
			return right;
		}

		const TCHAR lastChar = left[left.size() - 1];
		if (lastChar == _T('\\') || lastChar == _T('/'))
		{
			return left + right;
		}
		else
		{
			return left + _T('\\') + right;
		}
	}
	TString ExtractRelativePath(const TString& basePath, const TString& fullPath)
	{
		if (basePath.size() >= fullPath.size())
		{
			return TString();
		}

		if (basePath != fullPath.substr(0, basePath.size()))
		{
			return {};
		}

		TString result = fullPath.substr(basePath.size(), fullPath.size() - basePath.size());
		if (!result.empty() && result.front() == _T('\\'))
		{
			result.erase(0, 1);
		}
		return result;
	}

	bool FileExists(const TString& path)
	{
		const DWORD attributes = GetFileAttributes(path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
	}
	bool DirectoryExists(const TString& path)
	{
		const DWORD attributes = GetFileAttributes(path.c_str());
		if (attributes == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
		else
		{
			return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}
	}
	bool IsDirectoryEmptyRecursive(const TString& path)
	{
		IsEmptyCallback callback;
		PathScanner::Scan(path, callback);
		return callback.IsEmpty();
	}
	bool CreateDirectoryTree(const TString& path)
	{
		return ::SHCreateDirectoryEx(nullptr, path.c_str(), nullptr) == ERROR_SUCCESS;
	}

	std::vector<FilePathInfo> GetFile(const TString& filePathOrName)
	{
		TString path = FileSystem::GetPath(filePathOrName);
		TString name = FileSystem::GetFileName(filePathOrName);

		ScannerCallback callback(false, true);
		PathScanner::Scan(path, name, callback);
		return callback.GetFiles();
	}
	std::vector<FilePathInfo> GetFilesInDirectory(const TString& directory, const TString& searchPattern, bool recursive)
	{
		ScannerCallback callback(recursive);
		PathScanner::Scan(directory, searchPattern, callback);
		return callback.GetFiles();
	}

	CComPtr<IStream> OpenFileToRW(const TString& filePath)
	{
		CComPtr<IStream> fileStream;
		if (SUCCEEDED(::SHCreateStreamOnFileEx(filePath.c_str(), (FileExists(filePath) ? 0 : STGM_CREATE)|STGM_READWRITE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &fileStream)))
		{
			return fileStream;
		}
		return nullptr;
	}
	CComPtr<IStream> OpenFileToRead(const TString& filePath)
	{
		CComPtr<IStream> fileStream;
		if (SUCCEEDED(::SHCreateStreamOnFileEx(filePath.c_str(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &fileStream)))
		{
			return fileStream;
		}
		return nullptr;
	}
	CComPtr<IStream> OpenFileToWrite(const TString& filePath)
	{
		CComPtr<IStream> fileStream;
		if (SUCCEEDED(::SHCreateStreamOnFileEx(filePath.c_str(), STGM_CREATE|STGM_WRITE, FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &fileStream)))
		{
			return fileStream;
		}
		return nullptr;
	}
}
