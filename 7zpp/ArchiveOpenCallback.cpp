// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#include "StdAfx.h"
#include "ArchiveOpenCallback.h"
#include "ProgressNotifier.h"

namespace SevenZip::Callback
{
	OpenArchive::OpenArchive(ProgressNotifier* notifier)
		:m_RefCount(*this), m_Notifier(notifier)
	{
	}
	OpenArchive::~OpenArchive()
	{
	}

	STDMETHODIMP OpenArchive::QueryInterface(REFIID iid, void** ppvObject)
	{
		if (iid == __uuidof(IUnknown))
		{
			*ppvObject = reinterpret_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_IArchiveOpenCallback)
		{
			*ppvObject = static_cast<IArchiveOpenCallback*>(this);
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

	STDMETHODIMP OpenArchive::SetTotal(const UInt64* files, const UInt64* bytes)
	{
		if (bytes)
		{
			m_Total = *bytes;
		}
		return S_OK;
	}
	STDMETHODIMP OpenArchive::SetCompleted(const UInt64* files, const UInt64* bytes)
	{
		if (m_Notifier)
		{
			m_Notifier->OnMinorProgress(_T(""), bytes ? *bytes : 0, m_Total);
			if (m_Notifier->ShouldStop())
			{
				return HRESULT_FROM_WIN32(ERROR_CANCELLED);
			}
		}
		return S_OK;
	}
	STDMETHODIMP OpenArchive::CryptoGetTextPassword(BSTR* password)
	{
		// TODO: support passwords
		return E_ABORT;
	}
}
