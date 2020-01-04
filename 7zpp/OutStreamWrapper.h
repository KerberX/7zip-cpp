#pragma once
#include <7zip/IStream.h>
#include "SevenZipArchive.h"
#include "ProgressNotifier.h"
#include "COM.h"
#include "Stream.h"

namespace SevenZip
{
	class OutStream: public IOutStream
	{
		private:
			COM::RefCount<OutStream> m_RefCount;

		protected:
			ProgressNotifierDelegate m_Notifier;

		public:
			OutStream(ProgressNotifier* notifier = nullptr)
				:m_RefCount(*this), m_Notifier(notifier)
			{
			}
			virtual ~OutStream() = default;

		public:
			STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) override;
			STDMETHOD_(ULONG, AddRef)() override
			{
				return m_RefCount.AddRef();
			}
			STDMETHOD_(ULONG, Release)() override
			{
				return m_RefCount.Release();
			}

		public:
			void SetNotifier(ProgressNotifier* notifier)
			{
				m_Notifier = notifier;
			}
	};
}

namespace SevenZip
{
	class OutStreamWrapper: public OutStream
	{
		protected:
			int64_t m_BytesWritten = 0;
			int64_t m_BytesTotal = 0;
			bool m_BytesTotalKnown = false;

		protected:
			virtual HRESULT DoWrite(const void* data, uint32_t size, uint32_t& written) = 0;
			virtual HRESULT DoSeek(int64_t offset, uint32_t seekMode, int64_t& newPosition) = 0;
			virtual HRESULT DoSetSize(int64_t size) = 0;

		public:
			OutStreamWrapper(ProgressNotifier* notifier = nullptr)
				:OutStream(notifier)
			{
			}

		public:
			// ISequentialOutStream
			STDMETHOD(Write)(const void* data, UInt32 size, UInt32* written) override;

			// IOutStream
			STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) override;
			STDMETHOD(SetSize)(UInt64 newSize) override;
	};

	class OutStreamWrapper_IStream: public OutStreamWrapper
	{
		protected:
			CComPtr<IStream> m_BaseStream;

		protected:
			HRESULT DoWrite(const void* data, uint32_t size, uint32_t& written) override
			{
				ULONG writtenBase = 0;
				HRESULT hr = m_BaseStream->Write(data, size, &writtenBase);
				written = writtenBase;

				return hr;
			}
			HRESULT DoSeek(int64_t offset, uint32_t seekMode, int64_t& newPosition) override
			{
				LARGE_INTEGER offsetLI = {};
				offsetLI.QuadPart = offset;

				ULARGE_INTEGER newPositionLI = {};
				HRESULT hr = m_BaseStream->Seek(offsetLI, seekMode, &newPositionLI);
				newPosition = newPositionLI.QuadPart;

				return hr;
			}
			HRESULT DoSetSize(int64_t size) override
			{
				ULARGE_INTEGER value = {};
				value.QuadPart = size;
				return m_BaseStream->SetSize(value);
			}

		public:
			OutStreamWrapper_IStream(const CComPtr<IStream>& baseStream, ProgressNotifier* notifier = nullptr)
				:OutStreamWrapper(notifier), m_BaseStream(baseStream)
			{
			}
	};
}
