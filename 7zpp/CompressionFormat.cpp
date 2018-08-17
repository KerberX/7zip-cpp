#pragma once
#include "stdafx.h"
#include "CompressionFormat.h"

namespace SevenZip
{
	CompressionFormat::_Definition::StringValue CompressionFormat::_Definition::Strings[] =
	{
		{CompressionFormat::Unknown, _T("Unknown")},
		{CompressionFormat::SevenZip, _T("SevenZip")},
		{CompressionFormat::Zip, _T("Zip")},
		{CompressionFormat::GZip, _T("GZip")},
		{CompressionFormat::BZip2, _T("BZip2")},
		{CompressionFormat::Rar, _T("Rar")},
		{CompressionFormat::Rar5, _T("Rar5")},
		{CompressionFormat::Tar, _T("Tar")},
		{CompressionFormat::Iso, _T("Iso")},
		{CompressionFormat::Cab, _T("Cab")},
		{CompressionFormat::Lzma, _T("Lzma")},
		{CompressionFormat::Lzma86, _T("Lzma86")},
	};
}
