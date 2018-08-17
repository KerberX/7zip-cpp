#pragma once
#include <7zip/IStream.h>

namespace SevenZip
{
	class ProgressNotifier;

	namespace intl
	{
		class InStreamWrapper: public IInStream, public IStreamGetSize
		{
			private:
				long m_RefCount = 0;
				CComPtr<IStream> m_BaseStream = NULL;
				
				int64_t m_CurrentPos = 0;
				int64_t m_StreamSize = 0;
				TString m_FilePath;
				ProgressNotifier* m_ProgressNotifier = NULL;

			public:
				InStreamWrapper(ProgressNotifier* notifier = NULL);
				InStreamWrapper(const CComPtr<IStream>& baseStream, ProgressNotifier* notifier = NULL);
				virtual ~InStreamWrapper();

			public:
				STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) override;
				STDMETHOD_(ULONG, AddRef)() override;
				STDMETHOD_(ULONG, Release)() override;

				// ISequentialInStream
				STDMETHOD(Read)(void* data, UInt32 size, UInt32* processedSize) override;

				// IInStream
				STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) override;

				// IStreamGetSize
				STDMETHOD(GetSize)(UInt64* size) override;

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
}
