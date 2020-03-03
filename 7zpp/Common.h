#pragma once
#include <utility>
#include <atlbase.h>

namespace SevenZip
{
	using FileIndex = uint32_t;
	using FileIndexVector = std::vector<FileIndex>;

	constexpr FileIndex InvalidFileIndex = std::numeric_limits<FileIndex>::max();

	enum class CompressionMethod
	{
		Unknown = -1,

		LZMA,
		LZMA2,
		PPMD,
		BZIP2,
	};
	enum class CompressionFormat
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
	enum class CompressionLevel
	{
		None = 0,
		Fastest = 1,
		Fast = 2,
		Normal = 5,
		Maximum = 7,
		Ultra = 9,
	};
}

namespace SevenZip
{
	class FileIndexView final
	{
		private:
			union
			{
				const FileIndex* Ptr = nullptr;
				FileIndex Index;
			} m_Data;
			size_t m_Size = 0;

		private:
			void AssignMultiple(const FileIndex* data, size_t count)
			{
				if (data && count != 0)
				{
					m_Data.Ptr = data;
					m_Size = count;

					if (count == 1)
					{
						AssignSingle(*data);
					}
				}
			}
			void AssignSingle(FileIndex fileIndex)
			{
				m_Data.Index = fileIndex;
				m_Size = 1;
			}
			bool IsSingleIndex() const
			{
				return m_Size == 1;
			}

		public:
			FileIndexView() = default;
			FileIndexView(const FileIndex* data, size_t count)
			{
				AssignMultiple(data, count);
			}
			FileIndexView(const FileIndexVector& files)
			{
				AssignMultiple(files.data(), files.size());
			}
			FileIndexView(FileIndex fileIndex)
			{
				AssignSingle(fileIndex);
			}

			template<class T, size_t N>
			FileIndexView(const T(&container)[N])
			{
				AssignMultiple(container, N);
			}

			template<class T, size_t N>
			FileIndexView(const std::array<T, N>& container)
			{
				AssignMultiple(container.data(), container.size());
			}

		public:
			const FileIndex* data() const
			{
				if (IsSingleIndex())
				{
					return &m_Data.Index;
				}
				return m_Data.Ptr;
			}
			size_t size() const
			{
				return m_Size;
			}
			bool empty() const
			{
				return m_Size == 0;
			}

			FileIndex operator[](size_t index) const
			{
				return data()[index];
			}
			FileIndex front() const
			{
				return *data();
			}
			FileIndex back() const
			{
				return data()[size() - 1];
			}

			FileIndexVector CopyToVector() const
			{
				const FileIndex* data = this->data();
				const size_t size = this->size();

				return FileIndexVector(data, data + size);
			}

			explicit operator bool() const
			{
				return !empty();
			}
			bool operator!() const
			{
				return empty();
			}
	};
}

namespace SevenZip
{
	template<class T, class... Args>
	CComPtr<T> CreateObject(Args&&... arg)
	{
		return new T(std::forward<Args>(arg)...);
	}

	template<class T, class TNull = T>
	void ExchangeAndReset(T& left, T& right, TNull nullValue = {})
	{
		left = std::move(right);
		right = std::move(nullValue);
	}

	template<class T>
	void ExchangeAndReset(T& left, T& right, std::nullptr_t)
	{
		left = std::move(right);
		right = nullptr;
	}
	
	template<class TLeft, class TRight>
	TLeft ExchangeResetAndReturn(TLeft& right, TRight nullValue)
	{
		static_assert(std::is_default_constructible_v<TLeft>, "left type must be default constructible");

		TLeft left{};
		ExchangeAndReset(left, right, std::move(nullValue));
		return left;
	}

	template<class TFunc>
	class CallAtExit final
	{
		private:
			TFunc m_Func;

		public:
			CallAtExit(TFunc func)
				:m_Func(std::move(func))
			{
			}
			~CallAtExit()
			{
				std::invoke(m_Func);
			}
	};
}
