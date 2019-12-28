#include "stdafx.h"
#include "OutStreamWrapper.h"
#include "ProgressNotifier.h"
#include "Utility.h"

namespace SevenZip
{
	HRESULT STDMETHODCALLTYPE OutStream::QueryInterface(REFIID iid, void** ppvObject)
	{
		if (iid == __uuidof(IUnknown))
		{
			*ppvObject = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_ISequentialOutStream)
		{
			*ppvObject = static_cast<ISequentialOutStream*>(this);
			AddRef();
			return S_OK;
		}

		if (iid == IID_IOutStream)
		{
			*ppvObject = static_cast<IOutStream*>(this);
			AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}
}

namespace SevenZip
{
	STDMETHODIMP OutStreamWrapper::Write(const void* data, UInt32 size, UInt32* written)
	{
		if (m_Notifier)
		{
			m_Notifier->OnMinorProgress(m_FilePath, m_CurrentPosition, m_StreamSize);
			if (m_Notifier->ShouldStop())
			{
				return HRESULT_FROM_WIN32(ERROR_CANCELLED);
			}
		}

		ULONG writtenBase = 0;
		HRESULT hr = m_BaseStream->Write(data, size, &writtenBase);
		if (written)
		{
			m_CurrentPosition += writtenBase;
			*written = writtenBase;
		}
		return hr;
	}
	
	STDMETHODIMP OutStreamWrapper::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition)
	{
		LARGE_INTEGER move = {};
		move.QuadPart = offset;

		ULARGE_INTEGER newPos = {};
		HRESULT hr = m_BaseStream->Seek(move, seekOrigin, &newPos);
		if (newPosition)
		{
			*newPosition = newPos.QuadPart;
		}
		return hr;
	}
	STDMETHODIMP OutStreamWrapper::SetSize(UInt64 newSize)
	{
		m_StreamSize = newSize;

		ULARGE_INTEGER size = {};
		size.QuadPart = newSize;
		return m_BaseStream->SetSize(size);
	}
}
