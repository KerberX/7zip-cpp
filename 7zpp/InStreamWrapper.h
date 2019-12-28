#pragma once
#include <7zip/IStream.h>
#include "COM.h"

namespace SevenZip
{
	class ProgressNotifier;
}

namespace SevenZip
{
	class InStreamWrapper: public IInStream, public IStreamGetSize
	{
		private:
			COM::RefCount<InStreamWrapper> m_RefCount;

		protected:
			CComPtr<IStream> m_BaseStream = nullptr;
			ProgressNotifier* m_Notifier = nullptr;
				
			TString m_FilePath;
			int64_t m_CurrentPos = 0;
			int64_t m_StreamSize = 0;

		public:
			InStreamWrapper(ProgressNotifier* notifier = nullptr)
				:m_RefCount(*this), m_Notifier(notifier)
			{
			}
			InStreamWrapper(const CComPtr<IStream>& baseStream, ProgressNotifier* notifier = nullptr)
				:m_RefCount(*this), m_BaseStream(baseStream), m_Notifier(notifier)
			{
			}
			virtual ~InStreamWrapper() = default;

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

		public:
			void SetNotifier(ProgressNotifier* notifier)
			{
				m_Notifier = notifier;
			}
			void SetFilePath(const TString& path)
			{
				m_FilePath = path;
			}
			void SetStreamSize(int64_t size)
			{
				m_StreamSize = size;
			}
	};
}
