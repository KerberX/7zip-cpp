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
			notifier->OnStartWithTotal(_T(""), std::size(availableFormats));
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
					notifier->OnMajorProgress(String::Format(_T("Detecting format. Trying %d"), static_cast<int>(format)), counter);
					if (notifier->ShouldStop())
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
				InStreamWrapper inFile(fileStream, notifier);
				Callback::OpenArchive openCallback(notifier);

				if (SUCCEEDED(archive->Open(&inFile, nullptr, &openCallback)))
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
			if (notifier && notifier->ShouldStop())
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
			CallAtExit atExit([&]()
			{
				archive->Close();
			});
			InStreamWrapper inFile(fileStream, notifier);
			Callback::OpenArchive openCallback(notifier);

			if (FAILED(archive->Open(&inFile, nullptr, &openCallback)))
			{
				return false;
			}
			if (notifier)
			{
				notifier->OnMajorProgress(_T("Getting number of items"), 0);
			}

			UInt32 itemCount32 = 0;
			if (FAILED(archive->GetNumberOfItems(&itemCount32)))
			{
				return false;
			}
			itemCount = itemCount32;
			return true;
		}
		return false;
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
			InStreamWrapper inFile(fileStream, notifier);
			Callback::OpenArchive openCallback(notifier);

			if (FAILED(archive->Open(&inFile, nullptr, &openCallback)))
			{
				return false;
			}

			UInt32 itemCount32 = 0;
			if (FAILED(archive->GetNumberOfItems(&itemCount32)))
			{
				return false;
			}
			items.reserve(itemCount32);

			if (notifier)
			{
				notifier->OnStartWithTotal(_T("Enumerating items"), itemCount32);
			}

			for (UInt32 i = 0; i < itemCount32; i++)
			{
				FileInfo fileItem;
				VariantProperty property;

				// Get name of file
				if (FAILED(archive->GetProperty(i, kpidPath, &property)))
				{
					return false;
				}
				fileItem.FileName = property.ToString().value_or(TString());

				// Original size
				if (FAILED(archive->GetProperty(i, kpidSize, &property)))
				{
					return false;
				}
				fileItem.Size = property.ToInteger<decltype(fileItem.Size)>().value_or(-1);

				// Compressed size
				if (FAILED(archive->GetProperty(i, kpidPackSize, &property)))
				{
					return false;
				}
				fileItem.CompressedSize = property.ToInteger<decltype(fileItem.CompressedSize)>().value_or(-1);

				// Attributes
				if (FAILED(archive->GetProperty(i, kpidAttrib, &property)))
				{
					return false;
				}
				fileItem.Attributes = property.ToInteger<decltype(fileItem.Attributes)>().value_or(0);

				// Is directory
				if (FAILED(archive->GetProperty(i, kpidIsDir, &property)))
				{
					return false;
				}
				fileItem.IsDirectory = property.ToBool().value_or(false);

				// Creation time
				if (FAILED(archive->GetProperty(i, kpidCTime, &property)))
				{
					return false;
				}
				fileItem.CreationTime = property.ToFileTime().value_or(FILETIME());

				// Modification time
				if (FAILED(archive->GetProperty(i, kpidMTime, &property)))
				{
					return false;
				}
				fileItem.LastWriteTime = property.ToFileTime().value_or(FILETIME());

				// Last access time
				if (FAILED(archive->GetProperty(i, kpidATime, &property)))
				{
					return false;
				}
				fileItem.LastAccessTime = property.ToFileTime().value_or(FILETIME());

				// Notify callback
				if (notifier)
				{
					notifier->OnMajorProgress(fileItem.FileName, i);
					if (notifier->ShouldStop())
					{
						return false;
					}
				}

				// Add the item
				items.emplace_back(std::move(fileItem));
			}
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
}
