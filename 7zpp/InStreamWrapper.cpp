#include "stdafx.h"
#include "InStreamWrapper.h"
#include "ProgressNotifier.h"
#include "Utility.h"

namespace SevenZip
{
	namespace intl
	{
		InStreamWrapper::InStreamWrapper(ProgressNotifier* notifier)
			:m_ProgressNotifier(notifier)
		{
		}
		InStreamWrapper::InStreamWrapper(const CComPtr<IStream>& baseStream, ProgressNotifier* notifier)
			:m_BaseStream(baseStream), m_ProgressNotifier(notifier)
		{
		}
		InStreamWrapper::~InStreamWrapper()
		{
		}

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
		ULONG STDMETHODCALLTYPE InStreamWrapper::AddRef()
		{
			return static_cast<ULONG>(InterlockedIncrement(&m_RefCount));
		}
		ULONG STDMETHODCALLTYPE InStreamWrapper::Release()
		{
			ULONG res = static_cast<ULONG>(InterlockedDecrement(&m_RefCount));
			if (res == 0)
			{
				delete this;
			}
			return res;
		}

		STDMETHODIMP InStreamWrapper::Read(void* data, UInt32 size, UInt32* processedSize)
		{
			if (m_ProgressNotifier)
			{
				m_ProgressNotifier->OnMinorProgress(m_FilePath, m_CurrentPos, m_StreamSize);
				if (m_ProgressNotifier->ShouldStop())
				{
					return HRESULT_FROM_WIN32(ERROR_CANCELLED);
				}
			}

			ULONG read = 0;
			HRESULT hr = m_BaseStream->Read(data, size, &read);
			if (processedSize != NULL)
			{
				*processedSize = read;
				m_CurrentPos += read;
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
			if (newPosition != NULL)
			{
				*newPosition = newPos.QuadPart;
				m_CurrentPos = *newPosition;
			}
			return hr;
		}
		STDMETHODIMP InStreamWrapper::GetSize(UInt64* size)
		{
			STATSTG statInfo;
			HRESULT hr = m_BaseStream->Stat(&statInfo, STATFLAG_NONAME);
			if (SUCCEEDED(hr))
			{
				*size = statInfo.cbSize.QuadPart;
				m_StreamSize = *size;
			}
			return hr;
		}
	}
}
