#pragma once
#include <7zip/IStream.h>
#include "SevenZipArchive.h"
#include "COM.h"
#include "Stream.h"

namespace SevenZip
{
	class ProgressNotifier;
}

namespace SevenZip
{
	class OutStream: public IOutStream
	{
		private:
			COM::RefCount<OutStream> m_RefCount;

		protected:
			TString m_FilePath;
			ProgressNotifier* m_Notifier = nullptr;

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
			void SetFilePath(const TString& path)
			{
				m_FilePath = path;
			}
	};
}

namespace SevenZip
{
	class OutStreamWrapper: public OutStream
	{
		protected:
			CComPtr<IStream> m_BaseStream;
			int64_t m_StreamSize = 0;
			int64_t m_CurrentPosition = 0;

		public:
			OutStreamWrapper(ProgressNotifier* notifier = nullptr)
				:OutStream(notifier)
			{
			}
			OutStreamWrapper(const CComPtr<IStream>& baseStream, ProgressNotifier* notifier = nullptr)
				:OutStream(notifier), m_BaseStream(baseStream)
			{
			}

		public:
			// ISequentialOutStream
			STDMETHOD(Write)(const void* data, UInt32 size, UInt32* written) override;

			// IOutStream
			STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) override;
			STDMETHOD(SetSize)(UInt64 newSize) override;

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
