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

		//typedef intl::EnumerationDefinitionNoStrings _Definition;
		struct _Definition: public intl::EnumerationDefinition<_Enum, _Definition>
		{
			static StringValue Strings[];
		};
		typedef intl::EnumerationValue< _Enum, _Definition, Unknown > _Value;
	};

	typedef CompressionFormat::_Value CompressionFormatEnum;
}
