#include "stdafx.h"
#include "Utility.h"
#include "VariantProperty.h"
#include "ProgressNotifier.h"
#include "Common.h"
#include "Enum.h"

namespace SevenZip::Utility
{
	TString& MakeLower(TString& string)
	{
		::CharLowerBuff(string.data(), (DWORD)string.length());
		return string;
	}
	TString& MakeUpper(TString& string)
	{
		::CharUpperBuff(string.data(), (DWORD)string.length());
		return string;
	}
	
	std::optional<GUID> GetCompressionGUID(const CompressionFormatEnum& format)
	{
		switch (format)
		{
			case CompressionFormat::SevenZip:
			{
				return CLSID_CFormat7z;
			}
			case CompressionFormat::Zip:
			{
				return CLSID_CFormatZip;
			}
			case CompressionFormat::GZip:
			{
				return CLSID_CFormatGZip;
			}
			case CompressionFormat::BZip2:
			{
				return CLSID_CFormatBZip2;
			}
			case CompressionFormat::Rar:
			{
				return CLSID_CFormatRar;
			}
			case CompressionFormat::Rar5:
			{
				return CLSID_CFormatRar5;
			}
			case CompressionFormat::Tar:
			{
				return CLSID_CFormatTar;
			}
			case CompressionFormat::Iso:
			{
				return CLSID_CFormatIso;
			}
			case CompressionFormat::Cab:
			{
				return CLSID_CFormatCab;
			}
			case CompressionFormat::Lzma:
			{
				return CLSID_CFormatLzma;
			}
			case CompressionFormat::Lzma86:
			{
				return CLSID_CFormatLzma86;
			}
		}
		return CLSID_CFormat7z;
	}
	
	CComPtr<IInArchive> GetArchiveReader(const Library& library, const CompressionFormatEnum& format)
	{
		if (auto guid = GetCompressionGUID(format))
		{
			return library.CreateObject<IInArchive>(*guid, IID_IInArchive);
		}
		return nullptr;
	}
	CComPtr<IOutArchive> GetArchiveWriter(const Library& library, const CompressionFormatEnum& format)
	{
		if (auto guid = GetCompressionGUID(format))
		{
			return library.CreateObject<IOutArchive>(*guid, IID_IOutArchive);
		}
		return nullptr;
	}
	
	bool GetNumberOfItems(const Library& library, const TString& archivePath, CompressionFormatEnum& format, size_t& itemCount, ProgressNotifier* notifier)
	{
		auto fileStream = FileSystem::OpenFileToRead(archivePath);
		if (!fileStream)
		{
			return false;
		}

		auto archive = Utility::GetArchiveReader(library, format);
		auto inFile = CreateObject<InStreamWrapper>(fileStream, notifier);
		auto openCallback = CreateObject<Callback::OpenArchive>(notifier);

		HRESULT hr = archive->Open(inFile, nullptr, openCallback);
		if (hr != S_OK)
		{
			return false;
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
		}

		itemCount = itemCount32;
		archive->Close();
		return true;
	}
	bool GetItemsNames(const Library& library, const TString& archivePath, CompressionFormatEnum& format, size_t& itemCount, std::vector<TString>& itemnames, std::vector<size_t>& origsizes, ProgressNotifier* notifier)
	{
		auto fileStream = FileSystem::OpenFileToRead(archivePath);
		if (!fileStream)
		{
			return false;
		}

		auto archive = Utility::GetArchiveReader(library, format);
		auto inFile = CreateObject<InStreamWrapper>(fileStream, notifier);
		auto openCallback = CreateObject<Callback::OpenArchive>(notifier);

		HRESULT hr = archive->Open(inFile, nullptr, openCallback);
		if (hr != S_OK)
		{
			return false;
		}

		UInt32 itemCount32;
		hr = archive->GetNumberOfItems(&itemCount32);
		if (hr != S_OK)
		{
			return false;
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
			VariantProperty prop;
			hr = archive->GetProperty(i, kpidSize, &prop);
			if (hr != S_OK)
			{
				return false;
			}

			origsizes[i] = prop.intVal;

			// Get name of file
			hr = archive->GetProperty(i, kpidPath, &prop);
			if (hr != S_OK)
			{
				return false;
			}

			// Valid string? Pass back the found value and call the callback function if set
			if (prop.vt == VT_BSTR)
			{
				const wchar_t* path = prop.bstrVal;
				itemnames[i] = path;
			}

			if (notifier)
			{
				notifier->OnMajorProgress(itemnames[i], i);
				if (notifier->ShouldStop())
				{
					return false;
				}
			}
		}

		archive->Close();
		return true;
	}
	
	bool DetectCompressionFormat(const Library& library, const TString& archivePath, CompressionFormatEnum& archiveCompressionFormat, ProgressNotifier* notifier)
	{
		// Add more formats here if 7zip supports more formats in the future
		std::vector<CompressionFormatEnum> availableFormats;

		// Add most believable by scanning file extension
		size_t dotPos = archivePath.rfind(_T('.'));
		if (dotPos != TString::npos)
		{
			auto format = CompressionFormatFromExtension(archivePath.substr(dotPos));
			if (format != CompressionFormat::Unknown)
			{
				availableFormats.push_back(format);
			}
		}

		availableFormats.emplace_back(CompressionFormat::SevenZip);
		availableFormats.emplace_back(CompressionFormat::Zip);
		availableFormats.emplace_back(CompressionFormat::Rar);
		availableFormats.emplace_back(CompressionFormat::Rar5);
		availableFormats.emplace_back(CompressionFormat::GZip);
		availableFormats.emplace_back(CompressionFormat::BZip2);
		availableFormats.emplace_back(CompressionFormat::Tar);
		availableFormats.emplace_back(CompressionFormat::Lzma);
		availableFormats.emplace_back(CompressionFormat::Lzma86);
		availableFormats.emplace_back(CompressionFormat::Cab);
		availableFormats.emplace_back(CompressionFormat::Iso);

		if (notifier)
		{
			notifier->OnStartWithTotal(_T(""), availableFormats.size());
		}

		// Check each format for one that works
		for (size_t i = 0; i < availableFormats.size(); i++)
		{
			CComPtr<IStream> fileStream = FileSystem::OpenFileToRead(archivePath);
			if (!fileStream)
			{
				return false;
			}

			if (notifier)
			{
				notifier->OnMajorProgress(_T("Detecting format. Trying ") + availableFormats[i].GetString(), i);
				if (notifier->ShouldStop())
				{
					return false;
				}
			}

			archiveCompressionFormat = availableFormats[i];
			auto archive = Utility::GetArchiveReader(library, archiveCompressionFormat);
			auto inFile = CreateObject<InStreamWrapper>(fileStream, notifier);
			auto openCallback = CreateObject<Callback::OpenArchive>(notifier);

			HRESULT hr = archive->Open(inFile, nullptr, openCallback);
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
			if (notifier && notifier->ShouldStop())
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
	TString ExtensionFromCompressionFormat(const CompressionFormatEnum& format)
	{
		switch (format)
		{
			case CompressionFormat::SevenZip:
			{
				return _T(".7z");
			}
			case CompressionFormat::Zip:
			{
				return _T(".zip");
			}
			case CompressionFormat::Rar:
			case CompressionFormat::Rar5:
			{
				return _T(".rar");
			}
			case CompressionFormat::GZip:
			{
				return _T(".gz");
			}
			case CompressionFormat::BZip2:
			{
				return _T(".bz");
			}
			case CompressionFormat::Tar:
			{
				return _T(".tar");
				break;
			}
			case CompressionFormat::Lzma:
			{
				return _T(".lzma");
			}
			case CompressionFormat::Lzma86:
			{
				return _T(".lzma86");
			}
			case CompressionFormat::Cab:
			{
				return _T(".cab");
			}
			case CompressionFormat::Iso:
			{
				return _T(".iso");
			}
		}
		return {};
	}
	CompressionFormatEnum CompressionFormatFromExtension(const TString& extWithDot)
	{
		TString ext = ToLower(extWithDot);
		if (ext == _T(".7z"))
		{
			return CompressionFormat::SevenZip;
		}
		if (ext == _T(".zip"))
		{
			return CompressionFormat::Zip;
		}
		if (ext == _T(".rar"))
		{
			return CompressionFormat::Rar;
		}
		if (ext == _T(".rar5"))
		{
			return CompressionFormat::Rar5;
		}
		if (ext == _T(".gz") || ext == _T(".gzip"))
		{
			return CompressionFormat::GZip;
		}
		if (ext == _T(".bz2") || ext == _T(".bzip2"))
		{
			return CompressionFormat::BZip2;
		}
		if (ext == _T(".tar"))
		{
			return CompressionFormat::Tar;
		}
		if (ext == _T(".lz") || ext == _T(".lzma"))
		{
			return CompressionFormat::Lzma;
		}
		if (ext == _T(".lz86") || ext == _T(".lzma86"))
		{
			return CompressionFormat::Lzma86;
		}
		if (ext == _T(".cab"))
		{
			return CompressionFormat::Cab;
		}
		if (ext == _T(".iso"))
		{
			return CompressionFormat::Iso;
		}

		return CompressionFormat::Unknown;
	}
}
