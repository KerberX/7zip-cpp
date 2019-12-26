// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#include "StdAfx.h"
#include "ArchiveUpdateCallback.h"
#include "VariantProperty.h"
#include "FileSystem.h"
#include "InStreamWrapper.h"
#include "ProgressNotifier.h"
#include "Common.h"

namespace SevenZip::Callback
{
	UpdateArchive::UpdateArchive(const TString& dirPrefix, const std::vector<FilePathInfo>& filePaths, const std::vector<TString>& inArchiveFilePaths, const TString& outputFilePath, ProgressNotifier* callback)
		:m_RefCount(*this), m_DirectoryPrefix(dirPrefix), m_FilePaths(filePaths), m_ArchiveRelativeFilePaths(inArchiveFilePaths), m_OutputPath(outputFilePath), m_ProgressNotifier(callback)
	{
	}
	UpdateArchive::~UpdateArchive()
	{
	}

	STDMETHODIMP UpdateArchive::QueryInterface(REFIID iid, void** ppvObject)
	{
		if (iid == __uuidof(IUnknown))
		{
			*ppvObject = reinterpret_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_IArchiveUpdateCallback)
		{
			*ppvObject = static_cast<IArchiveUpdateCallback*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_ICryptoGetTextPassword2)
		{
			*ppvObject = static_cast<ICryptoGetTextPassword2*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_ICompressProgressInfo)
		{
			*ppvObject = static_cast<ICompressProgressInfo*>(this);
			AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	STDMETHODIMP UpdateArchive::SetTotal(UInt64 size)
	{
		if (m_ProgressNotifier)
		{
			m_ProgressNotifier->OnStartWithTotal(m_OutputPath, size);
		}
		return S_OK;
	}
	STDMETHODIMP UpdateArchive::SetCompleted(const UInt64* completeValue)
	{
		if (m_ProgressNotifier)
		{
			m_ProgressNotifier->OnMajorProgress(_T(""), *completeValue);
			if (m_ProgressNotifier->ShouldStop())
			{
				return HRESULT_FROM_WIN32(ERROR_CANCELLED);
			}
		}
		return S_OK;
	}

	STDMETHODIMP UpdateArchive::GetUpdateItemInfo(UInt32 index, Int32* newData, Int32* newProperties, UInt32* indexInArchive)
	{
		// Setting info for Create mode (vs. Append mode).
		// TODO: support append mode

		if (newData)
		{
			*newData = 1;
		}
		if (newProperties)
		{
			*newProperties = 1;
		}
		if (indexInArchive)
		{
			*indexInArchive = std::numeric_limits<UInt32>::max();
		}

		if (m_ProgressNotifier && index < m_FilePaths.size())
		{
			m_ProgressNotifier->OnMinorProgress(m_FilePaths[index].FilePath, 0, 0);
			if (m_ProgressNotifier->ShouldStop())
			{
				return HRESULT_FROM_WIN32(ERROR_CANCELLED);
			}
		}

		return S_OK;
	}
	STDMETHODIMP UpdateArchive::GetProperty(UInt32 index, PROPID propID, PROPVARIANT* value)
	{
		VariantProperty prop;

		if (propID == kpidIsAnti)
		{
			prop = false;
			if (value)
			{
				return prop.Detach(*value);
			}
			return E_INVALIDARG;
		}

		if (index >= m_FilePaths.size())
		{
			return E_INVALIDARG;
		}

		const FilePathInfo& fileInfo = m_FilePaths.at(index);
		switch (propID)
		{
			case kpidPath:
			{
				prop = m_ArchiveRelativeFilePaths[index].c_str();
				break;
			}
			case kpidIsDir:
			{
				prop = fileInfo.IsDirectory;
				break;
			}
			case kpidSize:
			{
				prop = fileInfo.Size;
				break;
			}
			case kpidAttrib:
			{
				prop = fileInfo.Attributes;
				break;
			}
			case kpidCTime:
			{
				prop = fileInfo.CreationTime;
				break;
			}
			case kpidATime:
			{
				prop = fileInfo.LastAccessTime;
				break;
			}
			case kpidMTime:
			{
				prop = fileInfo.LastWriteTime;
				break;
			}
		};

		if (value)
		{
			return prop.Detach(*value);
		}
		return E_INVALIDARG;
	}
	STDMETHODIMP UpdateArchive::GetStream(UInt32 index, ISequentialInStream** inStream)
	{
		if (index >= m_FilePaths.size())
		{
			return E_INVALIDARG;
		}

		const FilePathInfo& fileInfo = m_FilePaths.at(index);
		if (fileInfo.IsDirectory)
		{
			return S_OK;
		}

		auto fileStream = FileSystem::OpenFileToRead(fileInfo.FilePath);
		if (!fileStream)
		{
			return HRESULT_FROM_WIN32(::GetLastError());
		}

		auto wrapperStream = CreateObject<InStreamWrapper>(fileStream, m_ProgressNotifier);
		wrapperStream->SetFilePath(fileInfo.FilePath);
		wrapperStream->SetStreamSize(fileInfo.Size);
		*inStream = wrapperStream.Detach();

		return S_OK;
	}
	STDMETHODIMP UpdateArchive::SetOperationResult(Int32 operationResult)
	{
		return S_OK;
	}

	STDMETHODIMP UpdateArchive::CryptoGetTextPassword2(Int32* passwordIsDefined, BSTR* password)
	{
		// TODO: support passwords
		*passwordIsDefined = 0;
		*password = ::SysAllocString(L"");
		return *password != _T('\0') ? S_OK : E_OUTOFMEMORY;
	}

	STDMETHODIMP UpdateArchive::SetRatioInfo(const UInt64* inSize, const UInt64* outSize)
	{
		return S_OK;
	}
}
