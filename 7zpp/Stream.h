#pragma once
#include <atomic>
#include <comdef.h>

namespace SevenZip::Stream
{
	enum class SeekMode
	{
		Current = SEEK_CUR,
		FromStart = SEEK_SET,
		fromEnd = SEEK_END
	};
}
