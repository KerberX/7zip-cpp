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
	STDMETHODIMP UpdateArchiveBase::QueryInterface(REFIID iid, void** ppvObject)
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

	STDMETHODIMP UpdateArchiveBase::SetTotal(UInt64 size)
	{
		m_Notifier.OnStart(m_OutputPath, size);
		return S_OK;
	}
	STDMETHODIMP UpdateArchiveBase::SetCompleted(const UInt64* completeValue)
	{
		m_Notifier.OnProgress({}, completeValue ? *completeValue : 0);
		return m_Notifier.ShouldCancel() ? E_ABORT : S_OK;
	}

	STDMETHODIMP UpdateArchiveBase::GetUpdateItemInfo(UInt32 index, Int32* newData, Int32* newProperties, UInt32* indexInArchive)
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

		if (m_Notifier && index < m_SourcePaths.size())
		{
			m_Notifier.OnProgress(m_SourcePaths[index].FilePath, 0);
			if (m_Notifier.ShouldCancel())
			{
				return E_ABORT;
			}
		}
		return S_OK;
	}
	STDMETHODIMP UpdateArchiveBase::GetProperty(UInt32 index, PROPID propID, PROPVARIANT* value)
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

		if (index >= m_SourcePaths.size())
		{
			return E_INVALIDARG;
		}

		const FilePathInfo& fileInfo = m_SourcePaths[index];
		switch (propID)
		{
			case kpidPath:
			{
				prop = m_TargetPaths[index].c_str();
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
	STDMETHODIMP UpdateArchiveBase::GetStream(UInt32 index, ISequentialInStream** inStream)
	{
		if (index >= m_SourcePaths.size())
		{
			return E_INVALIDARG;
		}

		const FilePathInfo& fileInfo = m_SourcePaths[index];
		if (fileInfo.IsDirectory)
		{
			return S_OK;
		}

		auto fileStream = FileSystem::OpenFileToRead(fileInfo.FilePath);
		if (!fileStream)
		{
			return HRESULT_FROM_WIN32(::GetLastError());
		}

		auto wrapperStream = CreateObject<InStreamWrapper>(fileStream, *m_Notifier);
		wrapperStream->SetSize(fileInfo.Size);
		*inStream = wrapperStream.Detach();

		m_Notifier.OnStart(fileInfo.FilePath, fileInfo.Size);
		return S_OK;
	}
	STDMETHODIMP UpdateArchiveBase::SetOperationResult(Int32 operationResult)
	{
		return S_OK;
	}

	STDMETHODIMP UpdateArchiveBase::CryptoGetTextPassword2(Int32* passwordIsDefined, BSTR* password)
	{
		// TODO: support passwords
		*passwordIsDefined = 0;
		*password = ::SysAllocString(L"");
		return *password != _T('\0') ? S_OK : E_OUTOFMEMORY;
	}

	STDMETHODIMP UpdateArchiveBase::SetRatioInfo(const UInt64* inSize, const UInt64* outSize)
	{
		return S_OK;
	}
}
