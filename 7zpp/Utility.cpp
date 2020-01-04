#include "stdafx.h"
#include "Utility.h"
#include "SevenZipLibrary.h"
#include "VariantProperty.h"
#include "ProgressNotifier.h"
#include "FileSystem.h"
#include "Common.h"

namespace SevenZip::Utility
{
	std::optional<GUID> GetCompressionGUID(CompressionFormat format)
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
	
	CComPtr<IInArchive> GetArchiveReader(const Library& library, CompressionFormat format)
	{
		if (auto guid = GetCompressionGUID(format))
		{
			return library.CreateObject<IInArchive>(*guid, IID_IInArchive);
		}
		return nullptr;
	}
	CComPtr<IOutArchive> GetArchiveWriter(const Library& library, CompressionFormat format)
	{
		if (auto guid = GetCompressionGUID(format))
		{
			return library.CreateObject<IOutArchive>(*guid, IID_IOutArchive);
		}
		return nullptr;
	}
	
	CompressionFormat GetCompressionFormat(const Library& library, const TString& archivePath, ProgressNotifier* notifier)
	{
		// Add more formats here if 7zip supports more formats in the future
		CompressionFormat availableFormats[] =
		{
			CompressionFormat::Unknown,
			CompressionFormat::SevenZip,
			CompressionFormat::Zip,
			CompressionFormat::Rar,
			CompressionFormat::Rar,
			CompressionFormat::Rar5,
			CompressionFormat::Cab,
			CompressionFormat::Iso,
			CompressionFormat::GZip,
			CompressionFormat::BZip2,
			CompressionFormat::Tar,
			CompressionFormat::Lzma,
			CompressionFormat::Lzma86,
		};

		// Add most believable by scanning file extension
		const size_t dotPos = archivePath.rfind(_T('.'));
		if (dotPos != TString::npos)
		{
			availableFormats[0] = CompressionFormatFromExtension(archivePath.substr(dotPos));
		}

		if (notifier)
		{
			notifier->OnStart(_T("Trying to detect archive compression format"), std::size(availableFormats));
		}

		// Check each format for one that works
		int64_t counter = 0;
		for (const CompressionFormat format: availableFormats)
		{
			if (format != CompressionFormat::Unknown)
			{
				// Skip the same format as the one added by the file extension as we already tested for it
				if (counter != 0 && format == availableFormats[0])
				{
					continue;
				}

				if (notifier)
				{
					notifier->OnProgress(String::Format(_T("Detecting format. Trying %s"), GetCompressionFormatName(format)), counter);
					if (notifier->ShouldCancel())
					{
						return CompressionFormat::Unknown;
					}
				}

				auto fileStream = FileSystem::OpenFileToRead(archivePath);
				if (!fileStream)
				{
					return CompressionFormat::Unknown;
				}

				auto archive = Utility::GetArchiveReader(library, format);
				CallAtExit atExit([&]()
				{
					archive->Close();
				});
				auto inFile = CreateObject<InStreamWrapper>(fileStream, notifier);
				auto openCallback = CreateObject<Callback::OpenArchive>(archivePath, notifier);

				if (archive->Open(inFile, nullptr, openCallback) == S_OK)
				{
					// We know the format if we get here, so return
					return format;
				}
			}
			counter++;
		}

		// There is a problem that GZip files will not be detected using the above method. This is a fix.
		if (true)
		{
			if (notifier && notifier->ShouldCancel())
			{
				return CompressionFormat::Unknown;
			}

			size_t itemCount = 0;
			if (GetNumberOfItems(library, archivePath, CompressionFormat::GZip, itemCount, notifier) && itemCount != 0)
			{
				// We know this file is a GZip file, so return
				return CompressionFormat::GZip;
			}
		}

		// If we get here, the format is unknown
		return CompressionFormat::Unknown;
	}
	bool GetNumberOfItems(const Library& library, const TString& archivePath, CompressionFormat format, size_t& itemCount, ProgressNotifier* notifier)
	{
		if (auto fileStream = FileSystem::OpenFileToRead(archivePath))
		{
			auto archive = Utility::GetArchiveReader(library, format);
			auto inFile = CreateObject<InStreamWrapper>(fileStream, notifier);
			auto openCallback = CreateObject<Callback::OpenArchive>(archivePath, notifier);

			if (SUCCEEDED(archive->Open(inFile, nullptr, openCallback)))
			{
				CallAtExit atExit([&]()
				{
					archive->Close();
				});

				if (notifier)
				{
					notifier->OnStart(_T("Getting number of items"), 0);
				}

				UInt32 itemCount32 = 0;
				if (archive->GetNumberOfItems(&itemCount32) != S_OK)
				{
					if (notifier)
					{
						notifier->OnEnd();
					}

					itemCount = itemCount32;
					return true;
				}
			}
		}
		return false;
	}
	
	std::optional<FileInfo> GetArchiveItem(const CComPtr<IInArchive>& archive, FileIndex fileIndex)
	{
		FileInfo fileItem;
		VariantProperty property;

		// Get name of file
		if (FAILED(archive->GetProperty(fileIndex, kpidPath, &property)))
		{
			return std::nullopt;
		}
		fileItem.FileName = property.ToString().value_or(TString());

		// Original size
		if (FAILED(archive->GetProperty(fileIndex, kpidSize, &property)))
		{
			return std::nullopt;
		}
		fileItem.Size = property.ToInteger<decltype(fileItem.Size)>().value_or(-1);

		// Compressed size
		if (FAILED(archive->GetProperty(fileIndex, kpidPackSize, &property)))
		{
			return std::nullopt;
		}
		fileItem.CompressedSize = property.ToInteger<decltype(fileItem.CompressedSize)>().value_or(-1);

		// Attributes
		if (FAILED(archive->GetProperty(fileIndex, kpidAttrib, &property)))
		{
			return std::nullopt;
		}
		fileItem.Attributes = property.ToInteger<decltype(fileItem.Attributes)>().value_or(0);

		// Is directory
		if (FAILED(archive->GetProperty(fileIndex, kpidIsDir, &property)))
		{
			return std::nullopt;
		}
		fileItem.IsDirectory = property.ToBool().value_or(false);

		// Creation time
		if (FAILED(archive->GetProperty(fileIndex, kpidCTime, &property)))
		{
			return std::nullopt;
		}
		fileItem.CreationTime = property.ToFileTime().value_or(FILETIME());

		// Modification time
		if (FAILED(archive->GetProperty(fileIndex, kpidMTime, &property)))
		{
			return std::nullopt;
		}
		fileItem.LastWriteTime = property.ToFileTime().value_or(FILETIME());

		// Last access time
		if (FAILED(archive->GetProperty(fileIndex, kpidATime, &property)))
		{
			return std::nullopt;
		}
		fileItem.LastAccessTime = property.ToFileTime().value_or(FILETIME());

		return fileItem;
	}
	bool GetArchiveItems(const Library& library, const TString& archivePath, CompressionFormat format, FileInfo::Vector& items, ProgressNotifier* notifier)
	{
		if (auto fileStream = FileSystem::OpenFileToRead(archivePath))
		{
			auto archive = Utility::GetArchiveReader(library, format);
			CallAtExit atExit([&]()
			{
				archive->Close();
			});
			auto inFile = CreateObject<InStreamWrapper>(fileStream, notifier);
			auto openCallback = CreateObject<Callback::OpenArchive>(archivePath, notifier);

			if (FAILED(archive->Open(inFile, nullptr, openCallback)))
			{
				return false;
			}

			UInt32 itemCount32 = 0;
			if (archive->GetNumberOfItems(&itemCount32) != S_OK)
			{
				return false;
			}
			items.reserve(itemCount32);

			if (notifier)
			{
				notifier->OnStart(_T("Enumerating items"), itemCount32);
			}

			for (UInt32 i = 0; i < itemCount32; i++)
			{
				if (auto fileItem = GetArchiveItem(archive, i))
				{
					// Notify callback
					if (notifier)
					{
						notifier->OnProgress(fileItem->FileName, i);
						if (notifier->ShouldCancel())
						{
							return false;
						}

						// Add the item
						items.emplace_back(std::move(*fileItem));
					}
				}
			}
			return true;
		}
		return false;
	}

	TString ExtensionFromCompressionFormat(CompressionFormat format)
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
				return _T(".bz2");
			}
			case CompressionFormat::Tar:
			{
				return _T(".tar");
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
	CompressionFormat CompressionFormatFromExtension(const TString& extWithDot)
	{
		TString ext = String::ToLower(extWithDot);
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
	TString GetCompressionFormatName(CompressionFormat format)
	{
		switch (format)
		{
			case CompressionFormat::SevenZip:
			{
				return _T("7-Zip");
			}
			case CompressionFormat::Zip:
			{
				return _T("ZIP");
			}
			case CompressionFormat::Rar:
			{
				return _T("RAR");
			}
			case CompressionFormat::Rar5:
			{
				return _T(".RAR 5");
			}
			case CompressionFormat::GZip:
			{
				return _T("GZip");
			}
			case CompressionFormat::BZip2:
			{
				return _T("BZip2");
			}
			case CompressionFormat::Tar:
			{
				return _T("TAR");
			}
			case CompressionFormat::Lzma:
			{
				return _T("LZMA");
			}
			case CompressionFormat::Lzma86:
			{
				return _T("LZMA 86");
			}
			case CompressionFormat::Cab:
			{
				return _T("CAB");
			}
			case CompressionFormat::Iso:
			{
				return _T("ISO");
			}
		}
		return {};
	}
}
