#pragma once
#include <7zip/IStream.h>
#include "SevenZipArchive.h"
class ProgressNotifier;

namespace SevenZip
{
	class OutStreamWrapper: public IOutStream
	{
		protected:
			long m_RefCount = 0;
			CComPtr<IStream> m_BaseStream = nullptr;

			TString m_FilePath;
			ProgressNotifier* m_ProgressNotifier = nullptr;
			int64_t m_CurrentPos = 0;
			int64_t m_StreamSize = 0;

		public:
			OutStreamWrapper(ProgressNotifier* notifier = nullptr);
			OutStreamWrapper(const CComPtr<IStream>& baseStream, ProgressNotifier* notifier = nullptr);
			virtual ~OutStreamWrapper();

		public:
			STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) override;
			STDMETHOD_(ULONG, AddRef)() override;
			STDMETHOD_(ULONG, Release)() override;

			// ISequentialOutStream
			STDMETHOD(Write)(const void* data, UInt32 size, UInt32* processedSize) override;

			// IOutStream
			STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) override;
			STDMETHOD(SetSize)(UInt64 newSize) override;

		public:
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

namespace SevenZip
{
	class OutStreamWrapperMemory: public OutStreamWrapper
	{
		private:
			DataBuffer& m_Buffer;

		public:
			OutStreamWrapperMemory(DataBuffer& buffer, ProgressNotifier* notifier)
				:OutStreamWrapper(notifier), m_Buffer(buffer)
			{
				m_StreamSize = m_Buffer.size();
			}
			virtual ~OutStreamWrapperMemory()
			{
			}

		public:
			// ISequentialOutStream
			STDMETHOD(Write)(const void* data, UInt32 size, UInt32* processedSize) override;

			// IOutStream
			STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) override;
			STDMETHOD(SetSize)(UInt64 newSize) override;
	};
}
