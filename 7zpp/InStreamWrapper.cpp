#include "stdafx.h"
#include "InStreamWrapper.h"
#include "ProgressNotifier.h"
#include "Utility.h"

namespace SevenZip
{
	HRESULT STDMETHODCALLTYPE InStreamWrapper::QueryInterface(REFIID iid, void** ppvObject)
	{
		if (iid == __uuidof(IUnknown))
		{
			*ppvObject = reinterpret_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_ISequentialInStream)
		{
			*ppvObject = static_cast<ISequentialInStream*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_IInStream)
		{
			*ppvObject = static_cast<IInStream*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_IStreamGetSize)
		{
			*ppvObject = static_cast<IStreamGetSize*>(this);
			AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	STDMETHODIMP InStreamWrapper::Read(void* data, UInt32 size, UInt32* processedSize)
	{
		m_Notifier.OnProgress({}, m_BytesRead);
		if (m_Notifier.ShouldCancel())
		{
			return E_ABORT;
		}

		ULONG read = 0;
		HRESULT hr = m_BaseStream->Read(data, size, &read);
		if (processedSize)
		{
			*processedSize = read;
			m_BytesRead += read;
		}

		// Transform S_FALSE to S_OK
		return SUCCEEDED(hr) ? S_OK : hr;
	}
	STDMETHODIMP InStreamWrapper::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition)
	{
		LARGE_INTEGER move;
		ULARGE_INTEGER newPos;

		move.QuadPart = offset;
		HRESULT hr = m_BaseStream->Seek(move, seekOrigin, &newPos);
		if (newPosition != nullptr)
		{
			*newPosition = newPos.QuadPart;
			m_BytesRead = *newPosition;
		}
		return hr;
	}
	STDMETHODIMP InStreamWrapper::GetSize(UInt64* size)
	{
		STATSTG statInfo = {};
		HRESULT hr = m_BaseStream->Stat(&statInfo, STATFLAG_NONAME);
		if (SUCCEEDED(hr))
		{
			*size = statInfo.cbSize.QuadPart;
			m_BytesTotal = *size;
		}
		return hr;
	}
}
