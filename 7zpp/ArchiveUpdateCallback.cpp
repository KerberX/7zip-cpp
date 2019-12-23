// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#include "StdAfx.h"
#include "ArchiveUpdateCallback.h"
#include "VariantProperty.h"
#include "FileSystem.h"
#include "InStreamWrapper.h"


namespace SevenZip
{
	ArchiveUpdateCallback::ArchiveUpdateCallback(const TString& dirPrefix, const std::vector<FilePathInfo>& filePaths, const std::vector<TString>& inArchiveFilePaths, const TString& outputFilePath, ProgressNotifier* callback)
		:m_DirectoryPrefix(dirPrefix), m_FilePaths(filePaths), m_ArchiveRelativeFilePaths(inArchiveFilePaths), m_OutputPath(outputFilePath), m_ProgressNotifier(callback)
	{
	}
	ArchiveUpdateCallback::~ArchiveUpdateCallback()
	{
	}

	STDMETHODIMP ArchiveUpdateCallback::QueryInterface(REFIID iid, void** ppvObject)
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
	STDMETHODIMP_(ULONG) ArchiveUpdateCallback::AddRef()
	{
		return static_cast<ULONG>(InterlockedIncrement(&m_RefCount));
	}
	STDMETHODIMP_(ULONG) ArchiveUpdateCallback::Release()
	{
		ULONG res = static_cast<ULONG>(InterlockedDecrement(&m_RefCount));
		if (res == 0)
		{
			delete this;
		}
		return res;
	}

	STDMETHODIMP ArchiveUpdateCallback::SetTotal(UInt64 size)
	{
		if (m_ProgressNotifier)
		{
			m_ProgressNotifier->OnStartWithTotal(m_OutputPath, size);
		}
		return S_OK;
	}
	STDMETHODIMP ArchiveUpdateCallback::SetCompleted(const UInt64* completeValue)
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

	STDMETHODIMP ArchiveUpdateCallback::GetUpdateItemInfo(UInt32 index, Int32* newData, Int32* newProperties, UInt32* indexInArchive)
	{
		// Setting info for Create mode (vs. Append mode).
		// TODO: support append mode
		if (newData != nullptr)
		{
			*newData = 1;
		}

		if (newProperties != nullptr)
		{
			*newProperties = 1;
		}

		if (indexInArchive != nullptr)
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
	STDMETHODIMP ArchiveUpdateCallback::GetProperty(UInt32 index, PROPID propID, PROPVARIANT* value)
	{
		VariantProperty prop;

		if (propID == kpidIsAnti)
		{
			prop = false;
			prop.Detach(value);
			return S_OK;
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
				//prop = FileSys::ExtractRelativePath(m_DirectoryPrefix, fileInfo.FilePath).c_str();
				break;
			}
			case kpidIsDir:		prop = fileInfo.IsDirectory; break;
			case kpidSize:		prop = fileInfo.Size; break;
			case kpidAttrib:	prop = fileInfo.Attributes; break;
			case kpidCTime:		prop = fileInfo.CreationTime; break;
			case kpidATime:		prop = fileInfo.LastAccessTime; break;
			case kpidMTime:		prop = fileInfo.LastWriteTime; break;
		}

		prop.Detach(value);
		return S_OK;
	}
	STDMETHODIMP ArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream** inStream)
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

		CComPtr<IStream> fileStream = FileSystem::OpenFileToRead(fileInfo.FilePath);
		if (fileStream == nullptr)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		CComPtr<InStreamWrapper> wrapperStream = new InStreamWrapper(fileStream, m_ProgressNotifier);
		wrapperStream->SetFilePath(fileInfo.FilePath);
		wrapperStream->SetStreamSize(fileInfo.Size);
		*inStream = wrapperStream.Detach();

		return S_OK;
	}
	STDMETHODIMP ArchiveUpdateCallback::SetOperationResult(Int32 operationResult)
	{
		return S_OK;
	}

	STDMETHODIMP ArchiveUpdateCallback::CryptoGetTextPassword2(Int32* passwordIsDefined, BSTR* password)
	{
		// TODO: support passwords
		*passwordIsDefined = 0;
		*password = SysAllocString(L"");
		return *password != 0 ? S_OK : E_OUTOFMEMORY;
	}

	STDMETHODIMP ArchiveUpdateCallback::SetRatioInfo(const UInt64* inSize, const UInt64* outSize)
	{
		return S_OK;
	}
}
