// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#include "StdAfx.h"
#include "Utility.h"
#include "ArchiveExtractCallback.h"
#include "VariantProperty.h"
#include "FileSystem.h"
#include "OutStreamWrapper.h"
#include <comdef.h>


namespace SevenZip
{
	const TString EmptyFileAlias = _T("[Content]");

	void ArchiveExtractCallback::GetPropertyFilePath(UInt32 index)
	{
		VariantProperty prop;
		HRESULT hr = m_ArchiveHandler->GetProperty(index, kpidPath, &prop);
		if (hr != S_OK)
		{
			_com_issue_error(hr);
		}

		if (prop.vt == VT_EMPTY)
		{
			m_RelativePath = EmptyFileAlias;
		}
		else if (prop.vt != VT_BSTR)
		{
			_com_issue_error(E_FAIL);
		}
		else
		{
			_bstr_t bstr = prop.bstrVal;
			#ifdef _UNICODE
			m_RelativePath = bstr;
			#else
			char relPath[MAX_PATH];
			int size = WideCharToMultiByte(CP_UTF8, 0, bstr, bstr.length(), relPath, MAX_PATH, nullptr, nullptr);
			m_RelativePath.assign(relPath, size);
			#endif
		}
	}
	void ArchiveExtractCallback::GetPropertyAttributes(UInt32 index)
	{
		VariantProperty prop;
		HRESULT hr = m_ArchiveHandler->GetProperty(index, kpidAttrib, &prop);
		if (hr != S_OK)
		{
			_com_issue_error(hr);
		}

		if (prop.vt == VT_EMPTY)
		{
			m_Attributes = 0;
			m_HasAttributes = false;
		}
		else if (prop.vt != VT_UI4)
		{
			_com_issue_error(E_FAIL);
		}
		else
		{
			m_Attributes = prop.ulVal;
			m_HasAttributes = true;
		}
	}
	void ArchiveExtractCallback::GetPropertyIsDir(UInt32 index)
	{
		VariantProperty prop;
		HRESULT hr = m_ArchiveHandler->GetProperty(index, kpidIsDir, &prop);
		if (hr != S_OK)
		{
			_com_issue_error(hr);
		}

		if (prop.vt == VT_EMPTY)
		{
			m_IsDirectory = false;
		}
		else if (prop.vt != VT_BOOL)
		{
			_com_issue_error(E_FAIL);
		}
		else
		{
			m_IsDirectory = prop.boolVal != VARIANT_FALSE;
		}
	}
	void ArchiveExtractCallback::GetPropertyModifiedTime(UInt32 index)
	{
		VariantProperty prop;
		HRESULT hr = m_ArchiveHandler->GetProperty(index, kpidMTime, &prop);
		if (hr != S_OK)
		{
			_com_issue_error(hr);
		}

		if (prop.vt == VT_EMPTY)
		{
			m_HasModificationTime = false;
		}
		else if (prop.vt != VT_FILETIME)
		{
			_com_issue_error(E_FAIL);
		}
		else
		{
			m_ModificationTime = prop.filetime;
			m_HasModificationTime = true;
		}
	}
	void ArchiveExtractCallback::GetPropertySize(UInt32 index)
	{
		VariantProperty prop;
		HRESULT hr = m_ArchiveHandler->GetProperty(index, kpidSize, &prop);
		if (hr != S_OK)
		{
			_com_issue_error(hr);
		}

		switch (prop.vt)
		{
			case VT_EMPTY:
			{
				m_HasNewFileSize = false;
				return;
			}
			case VT_UI1:
			{
				m_NewFileSize = prop.bVal;
				break;
			}
			case VT_UI2:
			{
				m_NewFileSize = prop.uiVal;
				break;
			}
			case VT_UI4:
			{
				m_NewFileSize = prop.ulVal;
				break;
			}
			case VT_UI8:
			{
				m_NewFileSize = (UInt64)prop.uhVal.QuadPart;
				break;
			}
			default:
			{
				_com_issue_error(E_FAIL);
			}
		};
		m_HasNewFileSize = true;
	}
	void ArchiveExtractCallback::EmitDoneCallback()
	{
		if (m_Notifier)
		{
			m_Notifier->OnDone(m_RelativePath);
		}
	}
	void ArchiveExtractCallback::EmitFileDoneCallback(const TString& path)
	{
		if (m_Notifier)
		{
			m_Notifier->OnMinorProgress(path, m_NewFileSize, m_NewFileSize);
		}
	}

	HRESULT ArchiveExtractCallback::RetrieveAllProperties(UInt32 index, Int32 askExtractMode, bool* pContinue)
	{
		try
		{
			// Get basic properties
			GetPropertyFilePath(index);

			// Call notifier
			if (m_Notifier)
			{
				m_Notifier->OnMinorProgress(m_RelativePath, 0, 0);
				if (m_Notifier->ShouldStop())
				{
					return HRESULT_FROM_WIN32(ERROR_CANCELLED);
				}
			}

			// Return if this is not an extract request
			if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
			{
				return S_OK;
			}

			// Get all other properties
			GetPropertySize(index);
			GetPropertyAttributes(index);
			GetPropertyIsDir(index);
			GetPropertyModifiedTime(index);

			*pContinue = true;
			return S_OK;
		}
		catch (_com_error& ex)
		{
			return ex.Error();
		}
	}

	ArchiveExtractCallback::ArchiveExtractCallback(const CComPtr<IInArchive>& archiveHandler, const TString& directory, ProgressNotifier* notifier)
		:m_RefCount(0), m_ArchiveHandler(archiveHandler), m_Directory(directory), m_Notifier(notifier)
	{
	}
	ArchiveExtractCallback::~ArchiveExtractCallback()
	{
	}

	STDMETHODIMP ArchiveExtractCallback::QueryInterface(REFIID iid, void** ppvObject)
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
	STDMETHODIMP_(ULONG) ArchiveExtractCallback::AddRef()
	{
		return static_cast<ULONG>(InterlockedIncrement(&m_RefCount));
	}
	STDMETHODIMP_(ULONG) ArchiveExtractCallback::Release()
	{
		ULONG res = static_cast<ULONG>(InterlockedDecrement(&m_RefCount));
		if (res == 0)
		{
			delete this;
		}
		return res;
	}

	STDMETHODIMP ArchiveExtractCallback::SetTotal(UInt64 size)
	{
		// SetTotal is never called for ZIP and 7z formats
		if (m_Notifier)
		{
			m_Notifier->OnStartWithTotal(m_AbsolutePath, size);
		}
		return S_OK;
	}
	STDMETHODIMP ArchiveExtractCallback::SetCompleted(const UInt64* completeValue)
	{
		//Callback Event calls
		/*
		NB:
		- For ZIP format SetCompleted only called once per 1000 files in central directory and once per 100 in local ones.
		- For 7Z format SetCompleted is never called.
		*/
		if (m_Notifier != nullptr)
		{
			//Don't call this directly, it will be called per file which is more consistent across archive types
			//TODO: incorporate better progress tracking
			//m_callback->OnMajorProgress(m_absPath, *completeValue);
		}
		return S_OK;
	}

	STDMETHODIMP ArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode)
	{
		bool canContinue = false;
		HRESULT ret = RetrieveAllProperties(index, askExtractMode, &canContinue);

		if (canContinue)
		{
			if (m_FinalPaths.empty())
			{
				// TODO: m_Directory could be a relative path
				m_AbsolutePath = FileSystem::AppendPath(m_Directory, m_RelativePath);
			}
			else if (m_FinalPathsCounter < m_FinalPaths.size())
			{
				m_AbsolutePath = m_FinalPaths[m_FinalPathsCounter];
			}
			else
			{
				return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
			}

			// Increment counters and call notifier
			m_FinalPathsCounter++;
			m_BytesCompleted += m_NewFileSize;

			if (m_Notifier)
			{
				m_Notifier->OnMajorProgress(m_RelativePath, m_BytesCompleted);
				if (m_Notifier->ShouldStop())
				{
					return HRESULT_FROM_WIN32(ERROR_CANCELLED);
				}
			}

			if (m_IsDirectory)
			{
				// Creating the directory here supports having empty directories.
				FileSystem::CreateDirectoryTree(m_AbsolutePath);
				*outStream = nullptr;
				return S_OK;
			}

			FileSystem::CreateDirectoryTree(FileSystem::GetPath(m_AbsolutePath));

			CComPtr<IStream> fileStream = FileSystem::OpenFileToWrite(m_AbsolutePath);
			if (fileStream == nullptr)
			{
				m_AbsolutePath.clear();
				return HRESULT_FROM_WIN32(GetLastError());
			}

			CComPtr<OutStreamWrapper> wrapperStream = new OutStreamWrapper(fileStream, m_Notifier);
			wrapperStream->SetFilePath(m_RelativePath);
			wrapperStream->SetStreamSize(m_NewFileSize);
			*outStream = wrapperStream.Detach();

			return S_OK;
		}
		return ret;
	}
	STDMETHODIMP ArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
	{
		return S_OK;
	}
	STDMETHODIMP ArchiveExtractCallback::SetOperationResult(Int32 operationResult)
	{
		if (m_AbsolutePath.empty())
		{
			EmitDoneCallback();
			return S_OK;
		}

		if (m_HasModificationTime)
		{
			HANDLE fileHandle = CreateFile(m_AbsolutePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (fileHandle != INVALID_HANDLE_VALUE)
			{
				SetFileTime(fileHandle, nullptr, nullptr, &m_ModificationTime);
				CloseHandle(fileHandle);
			}
		}

		if (m_HasAttributes)
		{
			SetFileAttributes(m_AbsolutePath.c_str(), m_Attributes);
		}

		//EmitFileDoneCallback(m_RelativePath);
		return S_OK;
	}

	STDMETHODIMP ArchiveExtractCallback::CryptoGetTextPassword(BSTR* password)
	{
		// TODO: support passwords
		return E_ABORT;
	}
}

namespace SevenZip
{
	ArchiveExtractCallbackMemory::ArchiveExtractCallbackMemory(const CComPtr<IInArchive>& archiveHandler, SevenZip::DataBufferMap& bufferMap, ProgressNotifier* notifier)
		:ArchiveExtractCallback(archiveHandler, TString(), notifier), m_BufferMap(bufferMap)
	{
	}
	ArchiveExtractCallbackMemory::~ArchiveExtractCallbackMemory()
	{
	}

	STDMETHODIMP ArchiveExtractCallbackMemory::SetTotal(UInt64 size)
	{
		if (m_Notifier)
		{
			m_Notifier->OnStartWithTotal(m_AbsolutePath, size);
		}
		return S_OK;
	}
	STDMETHODIMP ArchiveExtractCallbackMemory::SetCompleted(const UInt64* completeValue)
	{
		if (m_Notifier)
		{
			m_Notifier->OnMajorProgress(m_RelativePath, *completeValue);
		}
		return S_OK;
	}

	STDMETHODIMP ArchiveExtractCallbackMemory::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode)
	{
		bool canContinue = false;
		HRESULT ret = RetrieveAllProperties(index, askExtractMode, &canContinue);
		if (canContinue)
		{
			if (m_BufferMap.count(index))
			{
				CComPtr<OutStreamWrapperMemory> wrapperStream = new OutStreamWrapperMemory(m_BufferMap.at(index), m_Notifier);
				wrapperStream->SetFilePath(m_RelativePath);
				*outStream = wrapperStream.Detach();
				return S_OK;
			}
			return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		}
		return ret;
	}
	STDMETHODIMP ArchiveExtractCallbackMemory::SetOperationResult(Int32 operationResult)
	{
		return ArchiveExtractCallback::SetOperationResult(operationResult);
	}
}
