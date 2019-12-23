#include "stdafx.h"
#include "SevenZipLibrary.h"
#include "GUIDs.h"

namespace
{
	constexpr auto DefaultLibraryPath = _T("7z.dll");
}

namespace SevenZip
{
	Library::Library()
	{
	}
	Library::~Library()
	{
		Free();
	}

	bool Library::Load()
	{
		return Load(DefaultLibraryPath);
	}
	bool Library::Load(const TString& libraryPath)
	{
		if (!IsLoaded())
		{
			m_LibraryHandle = ::LoadLibrary(libraryPath.c_str());
			if (m_LibraryHandle)
			{
				m_CreateObjectFunc = reinterpret_cast<CreateObjectFunc>(::GetProcAddress(m_LibraryHandle, "CreateObject"));
				if (m_CreateObjectFunc)
				{
					return true;
				}
			}

			Free();
		}
		return false;
	}
	void Library::Free()
	{
		if (m_LibraryHandle)
		{
			::FreeLibrary(m_LibraryHandle);

			m_LibraryHandle = nullptr;
			m_CreateObjectFunc = nullptr;
		}
	}
	
	bool Library::CreateObject(const GUID& classID, const GUID& interfaceID, void** outObject) const
	{
		if (m_CreateObjectFunc)
		{
			return SUCCEEDED(m_CreateObjectFunc(&classID, &interfaceID, outObject));
		}
		return false;
	}
}
