#pragma once
#include "SevenZipException.h"
#include "CompressionFormat.h"

namespace SevenZip
{
	class SevenZipLibrary
	{
		private:
			typedef UINT32(WINAPI *CreateObjectFunc)(const GUID* clsID, const GUID* nInterfaceID, void** pOutObject);

		private:
			HMODULE m_LibraryHandle = NULL;
			CreateObjectFunc m_CreateObjectFunc = NULL;

		public:
			SevenZipLibrary();
			virtual ~SevenZipLibrary();

		public:
			bool IsLoaded() const
			{
				return m_LibraryHandle != NULL && m_CreateObjectFunc != NULL;
			}
			HMODULE GetHandle() const
			{
				return m_LibraryHandle;
			}

			bool Load();
			bool Load(const TString& libraryPath);
			void Free();

			bool CreateObject(const GUID& clsID, const GUID& nInterfaceID, void** pOutObject) const;
	};
}
