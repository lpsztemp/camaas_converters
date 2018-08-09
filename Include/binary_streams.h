#if !defined(__GNUC__) || __GNUC__ > 7
#define CPP17_FILESYSTEM 1
#endif
#include <cstdint>
#include <vector>
#include <list>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include <string_view>
#if CPP17_FILESYSTEM
#include <filesystem>
#endif

#ifndef IMPL_BUF_OSTREAM_H
#define IMPL_BUF_OSTREAM_H

struct binary_ostream
{
	typedef std::size_t size_type, pos_type;

	virtual binary_ostream& write(const void* pInput, std::size_t cbHowMany) = 0;
	virtual pos_type tellp() const = 0;
	virtual binary_ostream& seekp(pos_type pos) = 0;
	virtual binary_ostream& seekp(std::ptrdiff_t off, std::ios_base::seekdir dir) = 0;

	template <class T>
	auto write(const T* pInput, std::size_t cHowMany) -> std::enable_if_t<std::is_pod_v<T>, binary_ostream&>
	{
		return this->write(static_cast<const void*>(pInput), cHowMany * sizeof(T));
	}
	template <class T>
	auto write(const T& rInput) -> binary_ostream&
	{
		return this->write(std::addressof(rInput), 1);
	}
	template <class T, std::size_t N>
	auto write(const T(& pInput)[N]) -> binary_ostream&
	{
		return this->write(pInput, N);
	}

	virtual ~binary_ostream(){}
};

template <class T>
binary_ostream& operator<<(binary_ostream& os, const T& obj)
{
	return os.write(obj);
}

struct buf_ostream:binary_ostream
{
	typedef std::uint8_t value_type;
	typedef binary_ostream::pos_type pos_type;
	typedef binary_ostream::size_type size_type;
	buf_ostream() = default;
	buf_ostream& write(const void* pInput, std::size_t cbHowMany);
	inline const void* data() const
	{
		this->serialize();
		return m_buf.data();
	}
	inline std::size_t size() const
	{
		return m_buf.size() + m_cbListSize;
	}
	inline const std::vector<std::uint8_t>& get_vector() const
	{
		this->serialize();
		return m_buf;
	}
	inline std::vector<std::uint8_t>& get_vector()
	{
		this->serialize();
		return m_buf;
	}
	inline void clear()
	{
		m_buf.clear();
		m_lst_buf.clear();
		m_cbListSize = std::size_t();
		m_cbOffset = std::size_t();
	}
	pos_type tellp() const;
	buf_ostream& seekp(pos_type pos);
	buf_ostream& seekp(std::ptrdiff_t off, std::ios_base::seekdir dir);
private:
	class data_block
	{
		std::uint8_t* m_pData = nullptr;
		std::size_t m_cbData = 0;
	public:
		data_block() = default;
		inline data_block(std::uint8_t* pData, std::size_t cbData):m_pData(pData), m_cbData(cbData) {}
		data_block(const data_block&) = delete;
		inline data_block(data_block&& r)
		{
			using std::swap;
			swap(m_pData, r.m_pData);
			swap(m_cbData, r.m_cbData);
		}
		data_block& operator=(const data_block& r) = delete;
		inline data_block& operator=(data_block&& r)
		{
			if (this != &r)
			{
				m_pData = r.m_pData;
				m_cbData = r.m_cbData;
				r.m_pData = nullptr;
				r.m_cbData = 0;
			}
			return *this;
		}
		inline ~data_block()
		{
			if (m_pData)
				delete [] m_pData;
		}
		inline std::uint8_t* data() const
		{
			return m_pData;
		}
		inline std::size_t size() const
		{
			return m_cbData;
		}
	};

	mutable std::vector<value_type> m_buf;
	mutable std::list<data_block> m_lst_buf;
	mutable std::size_t m_cbListSize = std::size_t();
	std::size_t m_cbOffset = std::size_t();

	data_block make_list_data_block(const void* pData, std::size_t cbData, std::size_t cbFillBefore = 0) const;
	const buf_ostream& serialize(std::size_t cbExtra = std::size_t()) const;
};

struct binary_fostream:binary_ostream
{
	typedef binary_ostream::pos_type pos_type;
	typedef binary_ostream::size_type size_type;
	inline explicit binary_fostream(const std::string_view& path):m_os(path, std::ios_base::out | std::ios_base::binary)
	{
	}
	inline explicit binary_fostream(const std::filesystem::path& path):m_os(path, std::ios_base::out | std::ios_base::binary)
	{
	}
	virtual binary_fostream& write(const void* pInput, std::size_t cbHowMany);
	virtual pos_type tellp() const;
	virtual binary_fostream& seekp(pos_type pos);
	virtual binary_fostream& seekp(std::ptrdiff_t off, std::ios_base::seekdir dir);
private:
	mutable std::ofstream m_os; //mutable because of tellp
};

struct temp_path:std::filesystem::path
{
	inline temp_path():std::filesystem::path(get_file_path()) {}
	inline ~temp_path()
	{
		std::error_code ec;
		remove(*this, ec);
	}
private:
	static unsigned suffix;
	inline static std::filesystem::path get_file_path()
	{
		std::filesystem::path path;
		do
		{
			path = std::filesystem::temp_directory_path() / (std::string("coverter_buffer_") + std::to_string(++suffix));
		}while (exists(status(path)));
		return path;
	}
};

std::string encode_string(const std::wstring& str);

#endif // IMPL_BUF_OSTREAM_H
