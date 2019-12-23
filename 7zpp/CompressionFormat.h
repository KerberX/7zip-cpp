#pragma once
#include "Enum.h"

namespace SevenZip
{
	struct CompressionFormat
	{
		enum _Enum
		{
			Unknown,
			SevenZip,
			Zip,
			GZip,
			BZip2,
			Rar,
			Rar5,
			Tar,
			Iso,
			Cab,
			Lzma,
			Lzma86
		};

		struct _Definition: public Internal::EnumerationDefinition<_Enum, _Definition>
		{
			static StringValue Strings[];
		};
		using _Value = Internal::EnumerationValue<_Enum, _Definition, Unknown>;
	};

	using CompressionFormatEnum = CompressionFormat::_Value;
}
