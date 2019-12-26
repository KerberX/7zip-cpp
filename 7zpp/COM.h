#pragma once
#include <atomic>
#include <comdef.h>

namespace SevenZip::COM
{
	template<class T, class TRefCount = ULONG>
	class RefCount final
	{
		private:
			std::atomic<TRefCount> m_RefCount = 0;
			T* m_Object = nullptr;

		public:
			RefCount(T& object)
				:m_Object(&object)
			{
				static_assert(std::is_integral_v<TRefCount>);
			}

		public:
			TRefCount AddRef()
			{
				return ++m_RefCount;
			}
			TRefCount Release()
			{
				const TRefCount newCount = --m_RefCount;
				if (newCount == 0)
				{
					delete m_Object;
				}
				return newCount;
			}
	};
}
