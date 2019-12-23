// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
// ./CPP/Windows/PropVariant.h
#pragma once
#include <Common/MyTypes.h>

namespace SevenZip
{
	class VariantProperty: public PROPVARIANT
	{
		private:
			HRESULT InternalClear();
			void InternalCopy(const PROPVARIANT& source);

		public:
			VariantProperty()
			{
				vt = VT_EMPTY;
				wReserved1 = 0;
			}
			VariantProperty(const PROPVARIANT& value);
			VariantProperty(const VariantProperty& value);
			VariantProperty(BSTR value);
			VariantProperty(const char* value);
			VariantProperty(LPCOLESTR value);
			VariantProperty(bool value)
			{
				vt = VT_BOOL;
				wReserved1 = 0;
				boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
			};
			VariantProperty(Byte value)
			{
				vt = VT_UI1;
				wReserved1 = 0;
				bVal = value;
			}
			VariantProperty(Int16 value)
			{
				vt = VT_I2;
				wReserved1 = 0;
				iVal = value;
			}
			VariantProperty(UInt16 value)
			{
				vt = VT_UI2;
				wReserved1 = 0;
				uiVal = value;
			}
			VariantProperty(Int32 value)
			{
				vt = VT_I4;
				wReserved1 = 0;
				lVal = value;
			}
			VariantProperty(UInt32 value)
			{
				vt = VT_UI4;
				wReserved1 = 0;
				ulVal = value;
			}
			VariantProperty(Int64 value)
			{
				vt = VT_I8;
				wReserved1 = 0;
				hVal.QuadPart = value;
			}
			VariantProperty(UInt64 value)
			{
				vt = VT_UI8;
				wReserved1 = 0;
				uhVal.QuadPart = value;
			}
			VariantProperty(const FILETIME& value)
			{
				vt = VT_FILETIME;
				wReserved1 = 0;
				filetime = value;
			}
			~VariantProperty()
			{
				Clear();
			}

		public:
			VariantProperty& operator=(const VariantProperty& value);
			VariantProperty& operator=(const PROPVARIANT& value);
			VariantProperty& operator=(BSTR value);
			VariantProperty& operator=(LPCOLESTR value);
			VariantProperty& operator=(const char* value);
			VariantProperty& operator=(bool value);
			VariantProperty& operator=(Byte value);
			VariantProperty& operator=(Int16 value);
			VariantProperty& operator=(UInt16 value);
			VariantProperty& operator=(Int32 value);
			VariantProperty& operator=(UInt32 value);
			VariantProperty& operator=(Int64 value);
			VariantProperty& operator=(UInt64 value);
			VariantProperty& operator=(const FILETIME& value);

			HRESULT Clear();
			HRESULT Copy(const PROPVARIANT* source);
			HRESULT Attach(PROPVARIANT* source);
			HRESULT Detach(PROPVARIANT* destination);

			int Compare(const VariantProperty& other) const;
			bool operator==(const VariantProperty& other) const
			{
				return Compare(other) == 0;
			}
			bool operator!=(const VariantProperty& other) const
			{
				return Compare(other) != 0;
			}
	};
}
