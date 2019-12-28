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
	bool Library::Load(TStringView libraryPath)
	{
		if (!IsLoaded())
		{
			TString path(libraryPath);
			m_LibraryHandle = ::LoadLibrary(path.c_str());
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
