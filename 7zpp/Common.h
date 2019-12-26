#pragma once
#include <utility>
#include <atlbase.h>

namespace SevenZip
{
	template<class T, class... Args>
	CComPtr<T> CreateObject(Args&&... arg)
	{
		return new T(std::forward<Args>(arg)...);
	}
}
