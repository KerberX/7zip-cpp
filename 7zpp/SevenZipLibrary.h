#pragma once
#include "SevenZipException.h"
#include "CompressionFormat.h"

namespace SevenZip
{
	class Library
	{
		private:
			using CreateObjectFunc = UINT32(WINAPI*)(const GUID* classID, const GUID* interfaceID, void** outObject);

		private:
			HMODULE m_LibraryHandle = nullptr;
			CreateObjectFunc m_CreateObjectFunc = nullptr;

		public:
			Library();
			virtual ~Library();

		public:
			bool IsLoaded() const
			{
				return m_LibraryHandle != nullptr;
			}
			HMODULE GetHandle() const
			{
				return m_LibraryHandle;
			}

			bool Load();
			bool Load(const TString& libraryPath);
			void Free();

			bool CreateObject(const GUID& classID, const GUID& interfaceID, void** outObject) const;

			#ifdef SEVENZIPCPP_LIB
			template<class T>
			CComPtr<T> CreateObject(const GUID& classID, const GUID& interfaceID) const
			{
				static_assert(std::is_base_of_v<IUnknown, T>, "Must be COM class");

				CComPtr<T> outObject;
				if (CreateObject(classID, interfaceID, reinterpret_cast<void**>(&outObject)))
				{
					return outObject;
				}
				return nullptr;
			}
			#endif
	};
}
