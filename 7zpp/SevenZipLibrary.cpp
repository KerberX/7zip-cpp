#include "stdafx.h"
#include "SevenZipLibrary.h"
#include "GUIDs.h"

namespace SevenZip
{
	const TString DefaultLibraryPath = _T("7z.dll");

	SevenZipLibrary::SevenZipLibrary()
	{
	}
	SevenZipLibrary::~SevenZipLibrary()
	{
		Free();
	}

	bool SevenZipLibrary::Load()
	{
		return Load(DefaultLibraryPath);
	}
	bool SevenZipLibrary::Load(const TString& libraryPath)
	{
		Free();
		m_LibraryHandle = LoadLibrary(libraryPath.c_str());
		if (m_LibraryHandle == NULL)
		{
			return false;
			//throw SevenZipException( GetWinErrMsg( _T( "LoadLibrary" ), GetLastError() ) );
		}

		m_CreateObjectFunc = reinterpret_cast<CreateObjectFunc>(GetProcAddress(m_LibraryHandle, "CreateObject"));
		if (m_CreateObjectFunc == NULL)
		{
			Free();
			return false;
			//throw SevenZipException( _T( "Loaded library is missing required CreateObject function" ) );
		}
		return true;
	}
	void SevenZipLibrary::Free()
	{
		if (m_LibraryHandle != NULL)
		{
			FreeLibrary(m_LibraryHandle);
			m_LibraryHandle = NULL;
			m_CreateObjectFunc = NULL;
		}
	}
	
	bool SevenZipLibrary::CreateObject(const GUID& clsID, const GUID& nInterfaceID, void** pOutObject) const
	{
		if (m_CreateObjectFunc == NULL)
		{
			return false;
			//throw SevenZipException( _T( "Library is not loaded" ) );
		}

		HRESULT nStatus = m_CreateObjectFunc(&clsID, &nInterfaceID, pOutObject);
		if (FAILED(nStatus))
		{
			return false;
			//throw SevenZipException( GetCOMErrMsg( _T( "CreateObject" ), hr ) );
		}
		return true;
	}
}
