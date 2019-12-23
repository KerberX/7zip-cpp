// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
// ./CPP/Windows/PropVariant.cpp
#include "stdafx.h"
#include "VariantProperty.h"
#include <Common/Defs.h>

#define SET_PROP_FUNC(type, id, dest)					\
VariantProperty& VariantProperty::operator=(type value)	\
{	\
	if (vt != id)	\
	{	\
		InternalClear();	\
		vt = id;	\
	}	\
	dest = value;	\
	return *this;	\
}

namespace
{
	constexpr auto kMemException = "out of memory";

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
		return ::VariantClear((VARIANTARG*)&prop);
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
		HRESULT hr = Copy(&source);
		if (FAILED(hr))
		{
			if (hr == E_OUTOFMEMORY)
			{
				throw kMemException;
			}
			vt = VT_ERROR;
			scode = hr;
		}
	}

	VariantProperty::VariantProperty(const PROPVARIANT& value)
	{
		vt = VT_EMPTY;
		InternalCopy(value);
	}
	VariantProperty::VariantProperty(const VariantProperty& value)
	{
		vt = VT_EMPTY;
		InternalCopy(value);
	}
	VariantProperty::VariantProperty(BSTR value)
	{
		vt = VT_EMPTY;
		*this = value;
	}
	VariantProperty::VariantProperty(const char* value)
	{
		vt = VT_EMPTY;
		*this = value;
	}
	VariantProperty::VariantProperty(LPCOLESTR value)
	{
		vt = VT_EMPTY;
		*this = value;
	}

	VariantProperty& VariantProperty::operator=(const VariantProperty& value)
	{
		InternalCopy(value);
		return *this;
	}
	VariantProperty& VariantProperty::operator=(const PROPVARIANT& value)
	{
		InternalCopy(value);
		return *this;
	}
	VariantProperty& VariantProperty::operator=(BSTR value)
	{
		*this = (LPCOLESTR)value;
		return *this;
	}
	VariantProperty& VariantProperty::operator=(LPCOLESTR value)
	{
		InternalClear();
		vt = VT_BSTR;
		wReserved1 = 0;
		bstrVal = ::SysAllocString(value);
		if (bstrVal == nullptr && value != nullptr)
		{
			throw kMemException;
			// vt = VT_ERROR;
			// scode = E_OUTOFMEMORY;
		}
		return *this;
	}
	VariantProperty& VariantProperty::operator=(const char* value)
	{
		InternalClear();
		vt = VT_BSTR;
		wReserved1 = 0;
		UINT len = (UINT)strlen(value);
		bstrVal = ::SysAllocStringByteLen(nullptr, (UINT)len * sizeof(OLECHAR));
		if (bstrVal == nullptr)
		{
			throw kMemException;
			// vt = VT_ERROR;
			// scode = E_OUTOFMEMORY;
		}
		else
		{
			for (UINT i = 0; i <= len; i++)
			{
				bstrVal[i] = value[i];
			}
		}
		return *this;
	}
	VariantProperty& VariantProperty::operator=(bool value)
	{
		if (vt != VT_BOOL)
		{
			InternalClear();
			vt = VT_BOOL;
		}
		boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
		return *this;
	}
	SET_PROP_FUNC(Byte, VT_UI1, bVal)
	SET_PROP_FUNC(Int16, VT_I2, iVal)
	SET_PROP_FUNC(UInt16, VT_UI2, uiVal)
	SET_PROP_FUNC(Int32, VT_I4, lVal)
	SET_PROP_FUNC(UInt32, VT_UI4, ulVal)
	SET_PROP_FUNC(Int64, VT_UI8, hVal.QuadPart)
	SET_PROP_FUNC(UInt64, VT_UI8, uhVal.QuadPart)
	SET_PROP_FUNC(const FILETIME&, VT_FILETIME, filetime)

	HRESULT VariantProperty::Clear()
	{
		return ClearPropVariant(*this);
	}
	HRESULT VariantProperty::Copy(const PROPVARIANT* source)
	{
		if (source)
		{
			::VariantClear((tagVARIANT*)this);

			switch (source->vt)
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
					memmove((PROPVARIANT*)this, source, sizeof(PROPVARIANT));
					return S_OK;
				}
			}
			return ::VariantCopy((tagVARIANT*)this, (tagVARIANT*)const_cast<PROPVARIANT*>(source));
		}
		return E_INVALIDARG;
	}
	HRESULT VariantProperty::Attach(PROPVARIANT* source)
	{
		if (source)
		{
			HRESULT hr = Clear();
			if (FAILED(hr))
			{
				return hr;
			}

			memcpy(this, source, sizeof(PROPVARIANT));
			source->vt = VT_EMPTY;
			return S_OK;
		}
		return E_INVALIDARG;
	}
	HRESULT VariantProperty::Detach(PROPVARIANT* destination)
	{
		if (destination)
		{
			HRESULT hr = ClearPropVariant(*destination);
			if (FAILED(hr))
			{
				return hr;
			}

			memcpy(destination, this, sizeof(PROPVARIANT));
			vt = VT_EMPTY;
			return S_OK;
		}
		return E_INVALIDARG;
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

			case VT_BOOL:
			{
				return -MyCompare(boolVal, other.boolVal);
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

			// case VT_UINT: return MyCompare(uintVal, a.uintVal);
			case VT_I8:
			{
				return MyCompare(hVal.QuadPart, other.hVal.QuadPart);
			}
			case VT_UI8:
			{
				return MyCompare(uhVal.QuadPart, other.uhVal.QuadPart);
			}

			case VT_FILETIME:
			{
				return ::CompareFileTime(&filetime, &other.filetime);
			}
			case VT_BSTR:
			{
				if (bstrVal && other.bstrVal)
				{
					using Traits = std::char_traits<TCHAR>;
					return Traits::compare(bstrVal, other.bstrVal, std::min(Traits::length(bstrVal), Traits::length(other.bstrVal)));
				}
				return MyCompare(bstrVal, other.bstrVal);
			}
			// return MyCompare(aPropVarint.cVal);
		};
		return 0;
	}
}
