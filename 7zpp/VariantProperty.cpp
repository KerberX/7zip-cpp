// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
// ./CPP/Windows/PropVariant.cpp
#include "stdafx.h"
#include "VariantProperty.h"
#include <Common/Defs.h>

namespace
{
	HRESULT ClearPropVariant(PROPVARIANT& prop)
	{
		switch (prop.vt)
		{
			case VT_UI1:
			case VT_I1:
			case VT_I2:
			case VT_UI2:
			case VT_BOOL:
			case VT_I4:
			case VT_UI4:
			case VT_R4:
			case VT_INT:
			case VT_UINT:
			case VT_ERROR:
			case VT_FILETIME:
			case VT_UI8:
			case VT_R8:
			case VT_CY:
			case VT_DATE:
			{
				prop.vt = VT_EMPTY;
				prop.wReserved1 = 0;
				return S_OK;
			}
		}
		return ::VariantClear(reinterpret_cast<VARIANTARG*>(&prop));
	}
}

namespace SevenZip
{
	HRESULT VariantProperty::InternalClear()
	{
		HRESULT hr = Clear();
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
		}
		return hr;
	}
	void VariantProperty::InternalCopy(const PROPVARIANT& source)
	{
		HRESULT hr = Copy(source);
		if (FAILED(hr))
		{
			if (hr == E_OUTOFMEMORY)
			{
				throw std::bad_alloc();
			}
			vt = VT_ERROR;
			scode = hr;
		}
	}
	void VariantProperty::InternalSetType(VARENUM type)
	{
		vt = type;
	}
	
	void VariantProperty::AssignString(std::string_view value)
	{
		InternalClear();

		vt = VT_BSTR;
		wReserved1 = 0;
		bstrVal = ::SysAllocStringByteLen(nullptr, static_cast<UINT>(value.size() * sizeof(OLECHAR)));
		if (!bstrVal)
		{
			throw std::bad_alloc();
		}
		else
		{
			for (size_t i = 0; i <= value.size(); i++)
			{
				bstrVal[i] = value[i];
			}
		}
	}
	void VariantProperty::AssignString(std::wstring_view value)
	{
		InternalClear();

		vt = VT_BSTR;
		wReserved1 = 0;
		bstrVal = ::SysAllocString(value.data());
		if (!bstrVal)
		{
			throw std::bad_alloc();
		}
	}

	HRESULT VariantProperty::Clear()
	{
		return ClearPropVariant(*this);
	}
	HRESULT VariantProperty::Copy(const PROPVARIANT& source)
	{
		::VariantClear((tagVARIANT*)this);

		switch (source.vt)
		{
			case VT_UI1:
			case VT_I1:
			case VT_I2:
			case VT_UI2:
			case VT_BOOL:
			case VT_I4:
			case VT_UI4:
			case VT_R4:
			case VT_INT:
			case VT_UINT:
			case VT_ERROR:
			case VT_FILETIME:
			case VT_UI8:
			case VT_R8:
			case VT_CY:
			case VT_DATE:
			{
				memmove((PROPVARIANT*)this, &source, sizeof(PROPVARIANT));
				return S_OK;
			}
		}
		return ::VariantCopy(reinterpret_cast<tagVARIANT*>(this), reinterpret_cast<const tagVARIANT*>(&source));
	}
	HRESULT VariantProperty::Attach(PROPVARIANT& source)
	{
		HRESULT hr = Clear();
		if (FAILED(hr))
		{
			return hr;
		}

		memcpy(this, &source, sizeof(PROPVARIANT));
		source.vt = VT_EMPTY;
		return S_OK;
	}
	HRESULT VariantProperty::Detach(PROPVARIANT& destination)
	{
		HRESULT hr = ClearPropVariant(destination);
		if (FAILED(hr))
		{
			return hr;
		}

		memcpy(&destination, this, sizeof(PROPVARIANT));
		vt = VT_EMPTY;
		return S_OK;
	}

	int VariantProperty::Compare(const VariantProperty& other) const
	{
		if (vt != other.vt)
		{
			return MyCompare(vt, other.vt);
		}

		switch (vt)
		{
			case VT_EMPTY:
			{
				return 0;
			}

			case VT_I1:
			{
				return MyCompare(cVal, other.cVal);
			}
			case VT_UI1:
			{
				return MyCompare(bVal, other.bVal);
			}

			case VT_I2:
			{
				return MyCompare(iVal, other.iVal);
			}
			case VT_UI2:
			{
				return MyCompare(uiVal, other.uiVal);
			}

			case VT_I4:
			{
				return MyCompare(lVal, other.lVal);
			}
			case VT_UI4:
			{
				return MyCompare(ulVal, other.ulVal);
			}

			case VT_I8:
			{
				return MyCompare(hVal.QuadPart, other.hVal.QuadPart);
			}
			case VT_UI8:
			{
				return MyCompare(uhVal.QuadPart, other.uhVal.QuadPart);
			}

			case VT_BOOL:
			{
				return -MyCompare(boolVal, other.boolVal);
			}
			case VT_FILETIME:
			{
				return ::CompareFileTime(&filetime, &other.filetime);
			}
			case VT_BSTR:
			{
				if (bstrVal && other.bstrVal)
				{
					using Traits = std::char_traits<OLECHAR>;
					return Traits::compare(bstrVal, other.bstrVal, std::min(Traits::length(bstrVal), Traits::length(other.bstrVal)));
				}
				return MyCompare(bstrVal, other.bstrVal);
			}
		};
		return MyCompare(this, &other);
	}
}
