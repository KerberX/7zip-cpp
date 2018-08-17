#include "stdafx.h"
#include "Utility.h"
#include "PropVariant.h"
#include "ProgressNotifier.h"
#include "Enum.h"

namespace SevenZip
{
	using namespace intl;

	TString& Utility::ToLower(TString& string)
	{
		::CharLowerBuff(string.data(), (DWORD)string.length());
		return string;
	}
	TString& Utility::ToUpper(TString& string)
	{
		::CharUpperBuff(string.data(), (DWORD)string.length());
		return string;
	}
	
	const GUID* Utility::GetCompressionGUID(const CompressionFormatEnum& format)
	{
		switch (format)
		{
			case CompressionFormat::Zip:
			{
				return &SevenZip::intl::CLSID_CFormatZip;
			}
			case CompressionFormat::GZip:
			{
				return &SevenZip::intl::CLSID_CFormatGZip;
			}
			case CompressionFormat::BZip2:
			{
				return &SevenZip::intl::CLSID_CFormatBZip2;
			}
			case CompressionFormat::Rar:
			{
				return &SevenZip::intl::CLSID_CFormatRar;
			}
			case CompressionFormat::Rar5:
			{
				return &SevenZip::intl::CLSID_CFormatRar5;
			}
			case CompressionFormat::Tar:
			{
				return &SevenZip::intl::CLSID_CFormatTar;
			}
			case CompressionFormat::Iso:
			{
				return &SevenZip::intl::CLSID_CFormatIso;
			}
			case CompressionFormat::Cab:
			{
				return &SevenZip::intl::CLSID_CFormatCab;
			}
			case CompressionFormat::Lzma:
			{
				return &SevenZip::intl::CLSID_CFormatLzma;
			}
			case CompressionFormat::Lzma86:
			{
				return &SevenZip::intl::CLSID_CFormatLzma86;
			}
		}
		return &SevenZip::intl::CLSID_CFormat7z;
	}
	
	CComPtr<IInArchive> Utility::GetArchiveReader(const SevenZipLibrary& library, const CompressionFormatEnum& format)
	{
		const GUID* guid = GetCompressionGUID(format);

		CComPtr<IInArchive> archive;
		library.CreateObject(*guid, IID_IInArchive, reinterpret_cast<void**>(&archive));
		return archive;
	}
	CComPtr<IOutArchive> Utility::GetArchiveWriter(const SevenZipLibrary& library, const CompressionFormatEnum& format)
	{
		const GUID* guid = GetCompressionGUID(format);

		CComPtr<IOutArchive> archive;
		library.CreateObject(*guid, IID_IOutArchive, reinterpret_cast<void**>(&archive));
		return archive;
	}
	
	bool Utility::GetNumberOfItems(const SevenZipLibrary& library, const TString& archivePath, CompressionFormatEnum& format, size_t& itemCount, ProgressNotifier* notifier)
	{
		CComPtr<IStream> fileStream = FileSys::OpenFileToRead(archivePath);

		if (fileStream == NULL)
		{
			return false;
			//throw SevenZipException( StrFmt( _T( "Could not open archive \"%s\"" ), m_archivePath.c_str() ) );
		}

		CComPtr<IInArchive> archive = Utility::GetArchiveReader(library, format);
		CComPtr<InStreamWrapper> inFile = new InStreamWrapper(fileStream);
		CComPtr<ArchiveOpenCallback> openCallback = new ArchiveOpenCallback(notifier);

		HRESULT hr = archive->Open(inFile, 0, openCallback);
		if (hr != S_OK)
		{
			return false;
			//throw SevenZipException( GetCOMErrMsg( _T( "Open archive" ), hr ) );
		}
		
		if (notifier)
		{
			notifier->OnMajorProgress(_T("Getting number of items"), 0);
		}

		UInt32 itemCount32;
		hr = archive->GetNumberOfItems(&itemCount32);
		if (hr != S_OK)
		{
			return false;
			//throw SevenZipException( GetCOMErrMsg( _T( "Open archive" ), hr ) );
		}
		itemCount = itemCount32;

		archive->Close();
		return true;
	}
	bool Utility::GetItemsNames(const SevenZipLibrary& library, const TString& archivePath, CompressionFormatEnum& format, size_t& itemCount, std::vector<TString>& itemnames, std::vector<size_t>& origsizes, ProgressNotifier* notifier)
	{
		CComPtr<IStream> fileStream = FileSys::OpenFileToRead(archivePath);

		if (fileStream == NULL)
		{
			return false;
			//throw SevenZipException( StrFmt( _T( "Could not open archive \"%s\"" ), m_archivePath.c_str() ) );
		}

		CComPtr<IInArchive> archive = Utility::GetArchiveReader(library, format);
		CComPtr<InStreamWrapper> inFile = new InStreamWrapper(fileStream);
		CComPtr<ArchiveOpenCallback> openCallback = new ArchiveOpenCallback(notifier);

		HRESULT hr = archive->Open(inFile, 0, openCallback);
		if (hr != S_OK)
		{
			return false;
			//throw SevenZipException( GetCOMErrMsg( _T( "Open archive" ), hr ) );
		}

		UInt32 itemCount32;
		hr = archive->GetNumberOfItems(&itemCount32);
		if (hr != S_OK)
		{
			return false;
			//throw SevenZipException( GetCOMErrMsg( _T( "Open archive" ), hr ) );
		}
		itemCount = itemCount32;

		itemnames.clear();
		itemnames.resize(itemCount);

		origsizes.clear();
		origsizes.resize(itemCount);

		if (notifier)
		{
			notifier->OnStartWithTotal(_T("Enumerating item names"), itemCount);
		}

		for (UInt32 i = 0; i < itemCount; i++)
		{
			// Get uncompressed size of file
			CPropVariant prop;
			hr = archive->GetProperty(i, kpidSize, &prop);
			if (hr != S_OK)
			{
				return false;
				//throw SevenZipException( GetCOMErrMsg( _T( "Open archive" ), hr ) );
			}

			origsizes[i] = prop.intVal;

			// Get name of file
			hr = archive->GetProperty(i, kpidPath, &prop);
			if (hr != S_OK)
			{
				return false;
				//throw SevenZipException( GetCOMErrMsg( _T( "Open archive" ), hr ) );
			}

			//valid string? pass back the found value and call the callback function if set
			if (prop.vt == VT_BSTR)
			{
				WCHAR* path = prop.bstrVal;
				itemnames[i] = path;
			}

			if (notifier)
			{
				notifier->OnMajorProgress(itemnames[i], i);
				if (notifier->Stop())
				{
					return false;
				}
			}
		}

		archive->Close();
		return true;
	}
	
	bool Utility::DetectCompressionFormat(const SevenZipLibrary& library, const TString& archivePath, CompressionFormatEnum& archiveCompressionFormat, ProgressNotifier* notifier)
	{
		// Add more formats here if 7zip supports more formats in the future
		std::vector<CompressionFormatEnum> availableFormats;

		// Add most believable by scanning file extension
		size_t nDotPos = archivePath.rfind(_T('.'));
		if (nDotPos != TString::npos)
		{
			auto format = CompressionFormatFromEnding(archivePath.substr(nDotPos));
			if (format != CompressionFormat::Unknown)
			{
				availableFormats.push_back(format);
			}
		}

		availableFormats.push_back(CompressionFormat::SevenZip);
		availableFormats.push_back(CompressionFormat::Zip);
		availableFormats.push_back(CompressionFormat::Rar);
		availableFormats.push_back(CompressionFormat::Rar5);
		availableFormats.push_back(CompressionFormat::GZip);
		availableFormats.push_back(CompressionFormat::BZip2);
		availableFormats.push_back(CompressionFormat::Tar);
		availableFormats.push_back(CompressionFormat::Lzma);
		availableFormats.push_back(CompressionFormat::Lzma86);
		availableFormats.push_back(CompressionFormat::Cab);
		availableFormats.push_back(CompressionFormat::Iso);

		if (notifier)
		{
			notifier->OnStartWithTotal(_T(""), availableFormats.size());
		}

		// Check each format for one that works
		for (size_t i = 0; i < availableFormats.size(); i++)
		{
			CComPtr<IStream> fileStream = FileSys::OpenFileToRead(archivePath);
			if (fileStream == NULL)
			{
				return false;
			}

			if (notifier)
			{
				TString message = _T("Detecting format. Trying ") + availableFormats[i].GetString();
				notifier->OnMajorProgress(message, i);
				if (notifier->Stop())
				{
					return false;
				}
			}

			archiveCompressionFormat = availableFormats[i];
			CComPtr<IInArchive> archive = Utility::GetArchiveReader(library, archiveCompressionFormat);
			CComPtr<InStreamWrapper> inFile = new InStreamWrapper(fileStream, notifier);
			CComPtr<ArchiveOpenCallback> openCallback = new ArchiveOpenCallback(notifier);

			HRESULT hr = archive->Open(inFile, 0, openCallback);
			archive->Close();
			if (hr == S_OK)
			{
				// We know the format if we get here, so return
				return true;
			}
		}

		// There is a problem that GZip files will not be detected using the above method. This is a fix.
		if (true)
		{
			if (notifier && notifier->Stop())
			{
				return false;
			}

			size_t itemCount = 0;
			archiveCompressionFormat = CompressionFormat::GZip;
			bool result = GetNumberOfItems(library, archivePath, archiveCompressionFormat, itemCount);
			if (result && itemCount > 0)
			{
				// We know this file is a GZip file, so return
				return true;
			}
		}

		// If we get here, the format is unknown
		archiveCompressionFormat = CompressionFormat::Unknown;
		return true;
	}
	TString Utility::EndingFromCompressionFormat(const CompressionFormatEnum& format)
	{
		switch (format)
		{
			case CompressionFormat::SevenZip:
			{
				return _T(".7z");
				break;
			}
			case CompressionFormat::Zip:
			{
				return _T(".zip");
				break;
			}
			case CompressionFormat::Rar:
			case CompressionFormat::Rar5:
			{
				return _T(".rar");
				break;
			}
			case CompressionFormat::GZip:
			{
				return _T(".gz");
				break;
			}
			case CompressionFormat::BZip2:
			{
				return _T(".bz");
				break;
			}
			case CompressionFormat::Tar:
			{
				return _T(".tar");
				break;
			}
			case CompressionFormat::Lzma:
			{
				return _T(".lzma");
				break;
			}
			case CompressionFormat::Lzma86:
			{
				return _T(".lzma86");
				break;
			}
			case CompressionFormat::Cab:
			{
				return _T(".cab");
				break;
			}
			case CompressionFormat::Iso:
			{
				return _T(".iso");
				break;
			}
		}
		return _T(".zip");
	}
	CompressionFormatEnum Utility::CompressionFormatFromEnding(const TString& extWithDot)
	{
		TString sExt = ToLower(extWithDot);
		if (sExt == _T(".7z"))
		{
			return CompressionFormat::SevenZip;
		}
		if (sExt == _T(".zip"))
		{
			return CompressionFormat::Zip;
		}
		if (sExt == _T(".rar"))
		{
			return CompressionFormat::Rar;
		}
		if (sExt == _T(".rar5"))
		{
			return CompressionFormat::Rar5;
		}
		if (sExt == _T(".gz") || sExt == _T(".gzip"))
		{
			return CompressionFormat::GZip;
		}
		if (sExt == _T(".bz2") || sExt == _T(".bzip2"))
		{
			return CompressionFormat::BZip2;
		}
		if (sExt == _T(".tar"))
		{
			return CompressionFormat::Tar;
		}
		if (sExt == _T(".lz") || sExt == _T(".lzma"))
		{
			return CompressionFormat::Lzma;
		}
		if (sExt == _T(".lz86") || sExt == _T(".lzma86"))
		{
			return CompressionFormat::Lzma86;
		}
		if (sExt == _T(".cab"))
		{
			return CompressionFormat::Cab;
		}
		if (sExt == _T(".iso"))
		{
			return CompressionFormat::Iso;
		}

		return CompressionFormat::Unknown;
	}
}

