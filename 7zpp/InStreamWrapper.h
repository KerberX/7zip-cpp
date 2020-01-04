#pragma once
#include <7zip/IStream.h>
#include "ProgressNotifier.h"
#include "COM.h"

namespace SevenZip
{
	class InStreamWrapper: public IInStream, public IStreamGetSize
	{
		private:
			COM::RefCount<InStreamWrapper> m_RefCount;

		protected:
			ProgressNotifierDelegate m_Notifier;
			CComPtr<IStream> m_BaseStream = nullptr;

			int64_t m_BytesRead = 0;
			int64_t m_BytesTotal = 0;

		public:
			InStreamWrapper(ProgressNotifier* notifier = nullptr)
				:m_RefCount(*this), m_Notifier(notifier)
			{
			}
			InStreamWrapper(const CComPtr<IStream>& baseStream, ProgressNotifier* notifier = nullptr)
				:m_RefCount(*this), m_Notifier(notifier), m_BaseStream(baseStream)
			{
			}
			virtual ~InStreamWrapper() = default;

		public:
			void SetNotifier(ProgressNotifier* notifier)
			{
				m_Notifier = notifier;
			}
			void SetSize(int64_t size)
			{
				m_BytesTotal = size;
			}

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

			// ISequentialInStream
			STDMETHOD(Read)(void* data, UInt32 size, UInt32* processedSize) override;

			// IInStream
			STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) override;

			// IStreamGetSize
			STDMETHOD(GetSize)(UInt64* size) override;
	};
}
