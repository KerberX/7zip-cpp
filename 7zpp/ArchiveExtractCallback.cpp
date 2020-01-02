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
	TString Extractor::GetPropertyFilePath(uint32_t fileIndex) const
	{
		VariantProperty property;
		HRESULT hr = m_Archive->GetProperty(fileIndex, kpidPath, &property);
		if (FAILED(hr))
		{
			_com_issue_error(hr);
		}

		if (property.vt == VT_EMPTY)
		{
			return String::Format(_T("%s %u"), EmptyFileAlias, fileIndex);
		}
		else if (property.vt != VT_BSTR)
		{
			_com_issue_error(E_FAIL);
		}
		else
		{
			_bstr_t bstr = property.bstrVal;
			#ifdef _UNICODE
			return TString(bstr.GetBSTR(), bstr.length());
			#else
			return ToUTF8(bstr, bstr.length());
			#endif
		}
		return {};
	}
	std::optional<int64_t> Extractor::GetPropertyFileSize(uint32_t fileIndex) const
	{
		VariantProperty property;
		if (SUCCEEDED(m_Archive->GetProperty(fileIndex, kpidSize, &property)))
		{
			return property.ToInteger<int64_t>();
		}
		return std::nullopt;
	}
	std::optional<int64_t> Extractor::GetPropertyCompressedFileSize(uint32_t fileIndex) const
	{
		VariantProperty property;
		if (SUCCEEDED(m_Archive->GetProperty(fileIndex, kpidPackSize, &property)))
		{
			return property.ToInteger<int64_t>();
		}
		return std::nullopt;
	}
	std::optional<uint32_t> Extractor::GetPropertyAttributes(uint32_t fileIndex) const
	{
		VariantProperty property;
		if (SUCCEEDED(m_Archive->GetProperty(fileIndex, kpidAttrib, &property)))
		{
			return property.ToInteger<int32_t>();
		}
		return std::nullopt;
	}
	std::optional<FILETIME> Extractor::GetPropertyModifiedTime(uint32_t fileIndex) const
	{
		VariantProperty prop;
		HRESULT hr = m_Archive->GetProperty(fileIndex, kpidMTime, &prop);
		if (FAILED(hr))
		{
			_com_issue_error(hr);
		}

		if (prop.vt == VT_EMPTY)
		{
			return std::nullopt;
		}
		else if (prop.vt == VT_FILETIME)
		{
			return prop.filetime;
		}
		else
		{
			_com_issue_error(E_FAIL);
		}
		return std::nullopt;
	}
	bool Extractor::GetPropertyIsDirectory(uint32_t fileIndex) const
	{
		VariantProperty prop;
		HRESULT hr = m_Archive->GetProperty(fileIndex, kpidIsDir, &prop);
		if (hr != S_OK)
		{
			_com_issue_error(hr);
		}

		if (prop.vt == VT_EMPTY)
		{
			return false;
		}
		else if (prop.vt == VT_BOOL)
		{
			return prop.boolVal != VARIANT_FALSE;
		}
		else
		{
			_com_issue_error(E_FAIL);
		}
	}
	HRESULT Extractor::RetrieveAllProperties(uint32_t fileIndex, int32_t askExtractMode)
	{
		try
		{
			// Get basic properties
			m_RelativePath = GetPropertyFilePath(fileIndex);

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
			m_FileSize = GetPropertyFileSize(fileIndex);
			m_CompressedFileSize = GetPropertyCompressedFileSize(fileIndex);
			m_Attributes = GetPropertyAttributes(fileIndex);
			m_IsDirectory = GetPropertyIsDirectory(fileIndex);
			m_ModificationTime = GetPropertyModifiedTime(fileIndex);

			return S_OK;
		}
		catch (const _com_error& ex)
		{
			return ex.Error();
		}
	}
	
	void Extractor::EmitDoneCallback()
	{
		if (m_Notifier)
		{
			m_Notifier->OnDone(m_RelativePath);
		}
	}
	void Extractor::EmitFileDoneCallback(const TString& path)
	{
		if (m_Notifier)
		{
			m_Notifier->OnMinorProgress(path, m_FileSize.value_or(-1), m_FileSize.value_or(-1));
		}
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
		if (m_Notifier)
		{
			m_Notifier->OnStartWithTotal(m_RelativePath, size);
		}
		return S_OK;
	}
	STDMETHODIMP Extractor::SetCompleted(const UInt64* completeValue)
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

	STDMETHODIMP Extractor::CryptoGetTextPassword(BSTR* password)
	{
		// TODO: support passwords
		return E_ABORT;
	}
}

namespace SevenZip::Callback
{
	STDMETHODIMP FileExtractor::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode)
	{
		HRESULT hr = RetrieveAllProperties(index, askExtractMode);
		if (SUCCEEDED(hr))
		{
			if (SUCCEEDED(GetTargetPath(index, m_RelativePath, m_TargetPath)))
			{
				if (m_TargetPath.empty())
				{
					// TODO: m_Directory could be a relative path
					m_TargetPath = FileSystem::AppendPath(m_Directory, m_RelativePath);
				}
				if (m_TargetPath.empty())
				{
					return E_UNEXPECTED;
				}
			}
			else
			{
				return E_UNEXPECTED;
			}

			// Increment counters and call notifier
			m_BytesCompleted += m_FileSize.value_or(0);
			m_FilesCount++;

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
				FileSystem::CreateDirectoryTree(m_TargetPath);
				*outStream = nullptr;

				return S_OK;
			}

			FileSystem::CreateDirectoryTree(FileSystem::GetPath(m_TargetPath));
			auto fileStream = FileSystem::OpenFileToWrite(m_TargetPath);
			if (!fileStream)
			{
				m_TargetPath.clear();
				return HRESULT_FROM_WIN32(GetLastError());
			}

			auto wrapperStream = CreateObject<OutStreamWrapper_IStream>(fileStream, m_Notifier);
			wrapperStream->SetFilePath(m_TargetPath);
			wrapperStream->SetSize(m_FileSize.value_or(0));
			*outStream = wrapperStream.Detach();

			return S_OK;
		}
		return hr;
	}
	STDMETHODIMP FileExtractor::PrepareOperation(Int32 askExtractMode)
	{
		return S_OK;
	}
	STDMETHODIMP FileExtractor::SetOperationResult(Int32 operationResult)
	{
		if (m_TargetPath.empty())
		{
			EmitDoneCallback();
			return S_OK;
		}

		if (m_ModificationTime)
		{
			HANDLE fileHandle = ::CreateFile(m_TargetPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (fileHandle != INVALID_HANDLE_VALUE)
			{
				::SetFileTime(fileHandle, nullptr, nullptr, &*m_ModificationTime);
				::CloseHandle(fileHandle);
			}
		}

		if (m_Attributes)
		{
			::SetFileAttributes(m_TargetPath.c_str(), *m_Attributes);
		}

		EmitFileDoneCallback(m_RelativePath);
		return S_OK;
	}
}

namespace SevenZip::Callback
{
	STDMETHODIMP StreamExtractor::SetTotal(UInt64 size)
	{
		if (m_Notifier)
		{
			m_Notifier->OnStartWithTotal(m_RelativePath, size);
		}
		return S_OK;
	}
	STDMETHODIMP StreamExtractor::SetCompleted(const UInt64* completeValue)
	{
		if (m_Notifier)
		{
			m_Notifier->OnMajorProgress(m_RelativePath, *completeValue);
		}
		return S_OK;
	}

	STDMETHODIMP StreamExtractor::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode)
	{
		HRESULT hr = RetrieveAllProperties(index, askExtractMode);
		if (SUCCEEDED(hr))
		{
			*outStream = nullptr;

			if (m_IsDirectory)
			{
				return OnDirectory(index, m_RelativePath);
			}
			else if (auto stream = CreateStream(index, m_RelativePath))
			{
				stream->SetFilePath(m_RelativePath);

				*outStream = stream.Detach();
				return S_OK;
			}
			return E_FAIL;
		}
		return hr;
	}
}
