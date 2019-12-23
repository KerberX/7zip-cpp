#pragma once
#include "Enum.h"

namespace SevenZip
{
	struct CompressionLevel
	{
		enum _Enum
		{
			None,
			Fast,
			Normal
		};

		using _Definition = Internal::EnumerationDefinitionNoStrings;
		using _Value = Internal::EnumerationValue<_Enum, _Definition, Normal>;
	};

	using CompressionLevelEnum = CompressionLevel::_Value;
}
