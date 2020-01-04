// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#include "StdAfx.h"
#include "ArchiveOpenCallback.h"
#include "ProgressNotifier.h"

namespace SevenZip::Callback
{
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
		m_BytesTotal = bytes ? *bytes : 0;
		m_Notifier.OnStart(m_ArchivePath, m_BytesTotal);

		return S_OK;
	}
	STDMETHODIMP OpenArchive::SetCompleted(const UInt64* files, const UInt64* bytes)
	{
		m_BytesCompleted = bytes ? *bytes : 0;

		m_Notifier.OnProgress({}, m_BytesCompleted);
		return m_Notifier.ShouldCancel() ? E_ABORT : S_OK;
	}
	STDMETHODIMP OpenArchive::CryptoGetTextPassword(BSTR* password)
	{
		// TODO: support passwords
		return E_ABORT;
	}
}
