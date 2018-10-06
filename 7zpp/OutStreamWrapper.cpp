#include "stdafx.h"
#include "OutStreamWrapper.h"
#include "ProgressNotifier.h"
#include "Utility.h"

namespace SevenZip
{
	namespace intl
	{
		OutStreamWrapper::OutStreamWrapper(ProgressNotifier* notifier)
			:m_ProgressNotifier(notifier)
		{
		}
		OutStreamWrapper::OutStreamWrapper(const CComPtr<IStream>& baseStream, ProgressNotifier* notifier)
			:m_BaseStream(baseStream), m_ProgressNotifier(notifier)
		{
		}
		OutStreamWrapper::~OutStreamWrapper()
		{
		}

		HRESULT STDMETHODCALLTYPE OutStreamWrapper::QueryInterface(REFIID iid, void** ppvObject)
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
		ULONG STDMETHODCALLTYPE OutStreamWrapper::AddRef()
		{
			return static_cast<ULONG>(InterlockedIncrement(&m_RefCount));
		}
		ULONG STDMETHODCALLTYPE OutStreamWrapper::Release()
		{
			ULONG res = static_cast<ULONG>(InterlockedDecrement(&m_RefCount));
			if (res == 0)
			{
				delete this;
			}
			return res;
		}
		
		STDMETHODIMP OutStreamWrapper::Write(const void* data, UInt32 size, UInt32* processedSize)
		{
			if (m_ProgressNotifier)
			{
				m_ProgressNotifier->OnMinorProgress(m_FilePath, m_CurrentPos, m_StreamSize);
				if (m_ProgressNotifier->ShouldStop())
				{
					return HRESULT_FROM_WIN32(ERROR_CANCELLED);
				}
			}

			ULONG written = 0;
			HRESULT hr = m_BaseStream->Write(data, size, &written);
			if (processedSize != NULL)
			{
				m_CurrentPos += written;
				*processedSize = written;
			}
			return hr;
		}
		STDMETHODIMP OutStreamWrapper::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition)
		{
			LARGE_INTEGER move;
			ULARGE_INTEGER newPos;

			move.QuadPart = offset;
			HRESULT hr = m_BaseStream->Seek(move, seekOrigin, &newPos);
			if (newPosition != NULL)
			{
				*newPosition = newPos.QuadPart;
			}
			return hr;
		}
		STDMETHODIMP OutStreamWrapper::SetSize(UInt64 newSize)
		{
			ULARGE_INTEGER size;
			size.QuadPart = newSize;
			m_StreamSize = newSize;
			return m_BaseStream->SetSize(size);
		}

		//////////////////////////////////////////////////////////////////////////
		STDMETHODIMP OutStreamWrapperMemory::Write(const void* data, UInt32 size, UInt32* processedSize)
		{
			if (m_ProgressNotifier)
			{
				m_ProgressNotifier->OnMinorProgress(m_FilePath, m_CurrentPos, m_StreamSize);
				if (m_ProgressNotifier->ShouldStop())
				{
					return HRESULT_FROM_WIN32(ERROR_CANCELLED);
				}
			}

			if (m_CurrentPos + size <= (int64_t)m_Buffer.size())
			{
				std::memcpy(m_Buffer.data() + m_CurrentPos, data, size);

				m_CurrentPos += size;
				*processedSize = size;
				return S_OK;
			}
			return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
		STDMETHODIMP OutStreamWrapperMemory::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition)
		{
			if (seekOrigin == FILE_BEGIN && offset >= 0)
			{
				m_CurrentPos = offset;
				*newPosition = m_CurrentPos;
			}
			else
			{
				return HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK);
			}

			if (seekOrigin == FILE_CURRENT)
			{
				m_CurrentPos += offset;
				*newPosition = m_CurrentPos;
			}

			if (seekOrigin == FILE_END)
			{
				return S_FALSE;
			}
			return S_OK;
		}
		STDMETHODIMP OutStreamWrapperMemory::SetSize(UInt64 newSize)
		{
			m_Buffer.resize((size_t)newSize);
			m_StreamSize = newSize;
			return S_OK;
		}
	}
}
