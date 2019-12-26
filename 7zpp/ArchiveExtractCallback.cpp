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

namespace
{
	constexpr auto EmptyFileAlias = _T("[Content]");
}

namespace SevenZip::Callback
{
	void ExtractArchive::GetPropertyFilePath(UInt32 index)
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
	void ExtractArchive::GetPropertyAttributes(UInt32 index)
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
	void ExtractArchive::GetPropertyIsDir(UInt32 index)
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
	void ExtractArchive::GetPropertyModifiedTime(UInt32 index)
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
	void ExtractArchive::GetPropertySize(UInt32 index)
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
	void ExtractArchive::EmitDoneCallback()
	{
		if (m_Notifier)
		{
			m_Notifier->OnDone(m_RelativePath);
		}
	}
	void ExtractArchive::EmitFileDoneCallback(const TString& path)
	{
		if (m_Notifier)
		{
			m_Notifier->OnMinorProgress(path, m_NewFileSize, m_NewFileSize);
		}
	}

	HRESULT ExtractArchive::RetrieveAllProperties(UInt32 index, Int32 askExtractMode, bool* shouldContinue)
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

			*shouldContinue = true;
			return S_OK;
		}
		catch (_com_error& ex)
		{
			return ex.Error();
		}
	}

	ExtractArchive::ExtractArchive(const CComPtr<IInArchive>& archiveHandler, const TString& directory, ProgressNotifier* notifier)
		:m_RefCount(*this), m_ArchiveHandler(archiveHandler), m_Directory(directory), m_Notifier(notifier)
	{
	}

	STDMETHODIMP ExtractArchive::QueryInterface(REFIID iid, void** ppvObject)
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

	STDMETHODIMP ExtractArchive::SetTotal(UInt64 size)
	{
		// SetTotal is never called for ZIP and 7z formats
		if (m_Notifier)
		{
			m_Notifier->OnStartWithTotal(m_AbsolutePath, size);
		}
		return S_OK;
	}
	STDMETHODIMP ExtractArchive::SetCompleted(const UInt64* completeValue)
	{
		//Callback Event calls
		/*
		NB:
		- For ZIP format SetCompleted only called once per 1000 files in central directory and once per 100 in local ones.
		- For 7Z format SetCompleted is never called.
		*/
		if (m_Notifier)
		{
			//Don't call this directly, it will be called per file which is more consistent across archive types
			//TODO: incorporate better progress tracking
			//m_callback->OnMajorProgress(m_absPath, *completeValue);
		}
		return S_OK;
	}

	STDMETHODIMP ExtractArchive::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode)
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
			auto fileStream = FileSystem::OpenFileToWrite(m_AbsolutePath);
			if (!fileStream)
			{
				m_AbsolutePath.clear();
				return HRESULT_FROM_WIN32(GetLastError());
			}

			auto wrapperStream = CreateObject<OutStreamWrapper>(fileStream, m_Notifier);
			wrapperStream->SetFilePath(m_RelativePath);
			wrapperStream->SetStreamSize(m_NewFileSize);
			*outStream = wrapperStream.Detach();

			return S_OK;
		}
		return ret;
	}
	STDMETHODIMP ExtractArchive::PrepareOperation(Int32 askExtractMode)
	{
		return S_OK;
	}
	STDMETHODIMP ExtractArchive::SetOperationResult(Int32 operationResult)
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

	STDMETHODIMP ExtractArchive::CryptoGetTextPassword(BSTR* password)
	{
		// TODO: support passwords
		return E_ABORT;
	}
}

namespace SevenZip::Callback
{
	ExtractArchiveToBuffer::ExtractArchiveToBuffer(const CComPtr<IInArchive>& archiveHandler, SevenZip::DataBufferMap& bufferMap, ProgressNotifier* notifier)
		:ExtractArchive(archiveHandler, TString(), notifier), m_BufferMap(bufferMap)
	{
	}

	STDMETHODIMP ExtractArchiveToBuffer::SetTotal(UInt64 size)
	{
		return ExtractArchive::SetTotal(size);
	}
	STDMETHODIMP ExtractArchiveToBuffer::SetCompleted(const UInt64* completeValue)
	{
		if (m_Notifier)
		{
			m_Notifier->OnMajorProgress(m_RelativePath, *completeValue);
		}
		return S_OK;
	}

	STDMETHODIMP ExtractArchiveToBuffer::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode)
	{
		bool canContinue = false;
		HRESULT ret = RetrieveAllProperties(index, askExtractMode, &canContinue);
		if (canContinue)
		{
			if (m_BufferMap.count(index))
			{
				auto wrapperStream = CreateObject<OutStreamWrapperMemory>(m_BufferMap.at(index), m_Notifier);
				wrapperStream->SetFilePath(m_RelativePath);
				*outStream = wrapperStream.Detach();
				return S_OK;
			}
			return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		}
		return ret;
	}
	STDMETHODIMP ExtractArchiveToBuffer::SetOperationResult(Int32 operationResult)
	{
		return ExtractArchive::SetOperationResult(operationResult);
	}
}
