// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
// ./CPP/Windows/PropVariant.h
#pragma once
#include <Common/MyTypes.h>
#include <cstdint>
#include <string_view>

namespace SevenZip
{
	class VariantProperty final: public PROPVARIANT
	{
		private:
			HRESULT InternalClear();
			void InternalCopy(const PROPVARIANT& source);
			void AssignString(std::string_view value);
			void AssignString(std::wstring_view value);

			template<VARENUM type, class TValue1, class TValue2>
			void CreateValue(TValue1& valueStore, TValue2 value)
			{
				this->vt = type;
				this->wReserved1 = 0;
				valueStore = std::move(value);
			}

			template<VARENUM type, class TValue1, class TValue2>
			VariantProperty& AssignValue(TValue1& valueStore, TValue2 value)
			{
				if (this->vt != type)
				{
					InternalClear();
					this->vt = type;
				}
				valueStore = std::move(value);

				return *this;
			}

		public:
			VariantProperty()
			{
				vt = VT_EMPTY;
				wReserved1 = 0;
			}
			VariantProperty(const PROPVARIANT& value)
			{
				vt = VT_EMPTY;
				InternalCopy(value);
			}
			VariantProperty(const VariantProperty& value)
			{
				vt = VT_EMPTY;
				InternalCopy(value);
			}
			VariantProperty(const char* value)
			{
				vt = VT_EMPTY;
				*this = value;
			}
			VariantProperty(const OLECHAR* value)
			{
				vt = VT_EMPTY;
				*this = value;
			}
			VariantProperty(BSTR value)
			{
				vt = VT_EMPTY;
				*this = value;
			}
			VariantProperty(bool value)
			{
				CreateValue<VT_BOOL>(boolVal, value ? VARIANT_TRUE : VARIANT_FALSE);
			};
			VariantProperty(int8_t value)
			{
				CreateValue<VT_I1>(cVal, value);
			}
			VariantProperty(uint8_t value)
			{
				CreateValue<VT_UI1>(bVal, value);
			}
			VariantProperty(int16_t value)
			{
				CreateValue<VT_I2>(iVal, value);
			}
			VariantProperty(uint16_t value)
			{
				CreateValue<VT_UI2>(uiVal, value);
			}
			VariantProperty(int32_t value)
			{
				CreateValue<VT_I4>(lVal, value);
			}
			VariantProperty(uint32_t value)
			{
				CreateValue<VT_UI4>(ulVal, value);
			}
			VariantProperty(int64_t value)
			{
				CreateValue<VT_I8>(hVal.QuadPart, value);
			}
			VariantProperty(uint64_t value)
			{
				CreateValue<VT_UI8>(uhVal.QuadPart, value);
			}
			VariantProperty(const FILETIME& value)
			{
				CreateValue<VT_FILETIME>(filetime, value);
			}
			~VariantProperty()
			{
				Clear();
			}

		public:
			HRESULT Clear();
			HRESULT Copy(const PROPVARIANT& source);
			HRESULT Attach(PROPVARIANT& source);
			HRESULT Detach(PROPVARIANT& destination);

		public:
			VariantProperty& operator=(const VariantProperty& value)
			{
				InternalCopy(value);
				return *this;
			}
			VariantProperty& operator=(const PROPVARIANT& value)
			{
				InternalCopy(value);
				return *this;
			}
			VariantProperty& operator=(const OLECHAR* value)
			{
				AssignString(value);
				return *this;
			}
			VariantProperty& operator=(const char* value)
			{
				AssignString(value);
				return *this;
			}
			VariantProperty& operator=(BSTR value)
			{
				AssignString(value);
				return *this;
			}
			VariantProperty& operator=(bool value)
			{
				return AssignValue<VT_BOOL>(boolVal, value ? VARIANT_TRUE : VARIANT_FALSE);
			}
			VariantProperty& operator=(int8_t value)
			{
				return AssignValue<VT_I1>(cVal, value);
			}
			VariantProperty& operator=(uint8_t value)
			{
				return AssignValue<VT_UI1>(bVal, value);
			}
			VariantProperty& operator=(int16_t value)
			{
				return AssignValue<VT_I2>(iVal, value);
			}
			VariantProperty& operator=(uint16_t value)
			{
				return AssignValue<VT_UI2>(uiVal, value);
			}
			VariantProperty& operator=(int32_t value)
			{
				return AssignValue<VT_I4>(lVal, value);
			}
			VariantProperty& operator=(uint32_t value)
			{
				return AssignValue<VT_UI4>(ulVal, value);
			}
			VariantProperty& operator=(int64_t value)
			{
				return AssignValue<VT_I8>(hVal.QuadPart, value);
			}
			VariantProperty& operator=(uint64_t value)
			{
				return AssignValue<VT_UI8>(uhVal.QuadPart, value);
			}
			VariantProperty& operator=(const FILETIME& value)
			{
				return AssignValue<VT_FILETIME>(filetime, value);
			}

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
