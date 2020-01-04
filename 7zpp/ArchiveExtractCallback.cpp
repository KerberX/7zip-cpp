// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#include "StdAfx.h"
#include "Utility.h"
#include "ArchiveExtractCallback.h"
#include "VariantProperty.h"
#include "FileSystem.h"
#include "OutStreamWrapper.h"
#include "ProgressNotifier.h"
#include "Common.h"
#include <comdef.h>

namespace SevenZip::Callback
{
	std::optional<SevenZip::FileInfo> Extractor::GetFileInfo(FileIndex fileIndex) const
	{
		return Utility::GetArchiveItem(m_Archive, fileIndex);
	}

	STDMETHODIMP Extractor::QueryInterface(REFIID iid, void** ppvObject)
	{
		if (iid == __uuidof(IUnknown))
		{
			*ppvObject = reinterpret_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_IArchiveExtractCallback)
		{
			*ppvObject = static_cast<IArchiveExtractCallback*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_ICryptoGetTextPassword)
		{
			*ppvObject = static_cast<ICryptoGetTextPassword*>(this);
			AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	STDMETHODIMP Extractor::SetTotal(UInt64 size)
	{
		// SetTotal is never called for ZIP and 7z formats
		return S_OK;
	}
	STDMETHODIMP Extractor::SetCompleted(const UInt64* completeValue)
	{
		// For ZIP format SetCompleted only called once per 1000 files in central directory and once per 100 in local ones.
		// For 7Z format SetCompleted is never called.
		return S_OK;
	}

	STDMETHODIMP Extractor::CryptoGetTextPassword(BSTR* password)
	{
		// TODO: Support passwords
		return E_ABORT;
	}
}

namespace SevenZip::Callback
{
	STDMETHODIMP FileExtractor::GetStream(UInt32 fileIndex, ISequentialOutStream** outStream, Int32 askExtractMode)
	{
		m_TargetPath.clear();
		m_FileInfo = GetFileInfo(fileIndex);

		if (m_FileInfo)
		{
			if (m_Notifier.ShouldCancel())
			{
				return E_ABORT;
			}

			HRESULT hr = GetTargetPath(fileIndex, *m_FileInfo, m_TargetPath);
			if (hr == S_FALSE)
			{
				// TODO: m_Directory could be a relative path
				m_TargetPath = FileSystem::AppendPath(m_Directory, m_FileInfo->FileName);
			}

			if (SUCCEEDED(hr))
			{
				if (m_TargetPath.empty())
				{
					return E_UNEXPECTED;
				}

				// Increment counters and call notifier
				m_BytesCompleted += m_FileInfo->Size;
				m_ItemCount++;

				if (m_FileInfo->IsDirectory)
				{
					// Creating the directory here supports having empty directories.
					FileSystem::CreateDirectoryTree(m_TargetPath);
					*outStream = nullptr;

					return S_OK;
				}

				FileSystem::CreateDirectoryTree(FileSystem::GetPath(m_TargetPath));
				auto fileStream = FileSystem::OpenFileToWrite(m_TargetPath);
				if (!fileStream)
				{
					return HRESULT_FROM_WIN32(::GetLastError());
				}

				auto wrapperStream = CreateObject<OutStreamWrapper_IStream>(fileStream, *m_Notifier);
				wrapperStream->SetSize(m_FileInfo->Size);
				*outStream = wrapperStream.Detach();

				m_Notifier.OnStart(m_FileInfo->FileName, m_FileInfo->Size);
				return S_OK;
			}
		}
		return E_FAIL;
	}
	STDMETHODIMP FileExtractor::PrepareOperation(Int32 askExtractMode)
	{
		m_TargetPath.clear();
		return S_OK;
	}
	STDMETHODIMP FileExtractor::SetOperationResult(Int32 operationResult)
	{
		if (m_FileInfo)
		{
			HANDLE fileHandle = ::CreateFile(m_TargetPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (fileHandle != INVALID_HANDLE_VALUE)
			{
				FILE_BASIC_INFO info = {};
				info.CreationTime = reinterpret_cast<LARGE_INTEGER&>(m_FileInfo->CreationTime);
				info.LastAccessTime = reinterpret_cast<LARGE_INTEGER&>(m_FileInfo->LastAccessTime);
				info.LastWriteTime = reinterpret_cast<LARGE_INTEGER&>(m_FileInfo->LastWriteTime);
				info.ChangeTime = reinterpret_cast<LARGE_INTEGER&>(m_FileInfo->LastWriteTime);
				info.FileAttributes = m_FileInfo->Attributes;
				::SetFileInformationByHandle(fileHandle, FILE_INFO_BY_HANDLE_CLASS::FileBasicInfo, &info, sizeof(info));

				::CloseHandle(fileHandle);
			}

			m_Notifier.OnEnd();
		}
		return S_OK;
	}
}
