#pragma warning (disable:4996)
#include <cstdint>
#include <vector>
#include <list>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <filesystem>
#include <iostream>

#ifndef IMPL_BUF_OSTREAM_H
#define IMPL_BUF_OSTREAM_H

struct buf_ostream
{
	typedef std::uint8_t value_type;
	typedef std::size_t size_type, pos_type;
	buf_ostream() = default;
	auto write(const void* pInput, std::size_t cbHowMany) -> buf_ostream&
	{
		auto cb = m_cbOffset + cbHowMany;
		if (m_buf.empty())
		{
			m_buf.reserve(cb);
			std::fill_n(std::back_inserter(m_buf), m_cbOffset, std::uint8_t());
			std::copy(static_cast<const std::uint8_t*>(pInput), static_cast<const std::uint8_t*>(pInput) + cbHowMany,
				std::back_inserter(m_buf));
			m_cbOffset += cbHowMany;
		}else if (m_cbOffset >= this->size())
		{
			auto cbFill = m_cbOffset - this->size();
			auto block = this->make_list_data_block(pInput, cbHowMany, cbFill);
			if (block.size() != 0)
			{
				auto cbBlock = m_lst_buf.emplace_back(std::move(block)).size();
				m_cbListSize += cbBlock;
				m_cbOffset += cbBlock;
			}
		}else
		{
			auto cbExtra = m_cbOffset + cbHowMany > this->size()?m_cbOffset + cbHowMany - this->size():std::size_t();
			auto cbInPlace = cbHowMany - cbExtra;
			this->serialize(cbExtra);
			std::copy(static_cast<const std::uint8_t*>(pInput), static_cast<const std::uint8_t*>(pInput) + cbInPlace,
				m_buf.begin() + m_cbOffset);
			std::copy(static_cast<const std::uint8_t*>(pInput) + cbInPlace, static_cast<const std::uint8_t*>(pInput) + cbHowMany,
				std::back_inserter(m_buf));
			m_cbOffset = cb;
		}
		return *this;
	}
	template <class T>
	auto write(const T* pInput, std::size_t cHowMany) -> std::enable_if_t<std::is_pod_v<T>, buf_ostream&>
	{
		return this->write(static_cast<const void*>(pInput), cHowMany * sizeof(T));
	}
	template <class T>
	auto write(const T& rInput) -> buf_ostream&
	{
		return this->write(std::addressof(rInput), 1);
	}
	template <class T, std::size_t N>
	auto write(const T(& pInput)[N]) -> buf_ostream&
	{
		return this->write(pInput, N);
	}
	const void* data() const
	{
		this->serialize();
		return m_buf.data();
	}
	std::size_t size() const
	{
		return m_buf.size() + m_cbListSize;
	}
	const std::vector<std::uint8_t>& get_vector() const
	{
		this->serialize();
		return m_buf;
	}
	std::vector<std::uint8_t>& get_vector()
	{
		this->serialize();
		return m_buf;
	}
	void clear()
	{
		m_buf.clear();
		m_lst_buf.clear();
		m_cbListSize = std::size_t();
		m_cbOffset = std::size_t();
	}
	pos_type tellp() const
	{
		return m_cbOffset;
	}
	buf_ostream& seekp(pos_type pos)
	{
		m_cbOffset = pos;
	}
	buf_ostream& seekp(std::ptrdiff_t off, std::ios_base::seekdir dir)
	{
		if (dir == std::ios_base::beg)
		{
			if (off < 0)
				throw std::invalid_argument("Invalid buf_ostream position");
			m_cbOffset = std::size_t(off);
		}else if (dir == std::ios_base::cur)
		{
			if ((std::ptrdiff_t) m_cbOffset + off < 0)
				throw std::invalid_argument("Invalid buf_ostream position");
			m_cbOffset += (std::size_t) off;
		}else if (dir == std::ios_base::end)
		{
			if ((std::ptrdiff_t) this->size() + off < 0)
				throw std::invalid_argument("Invalid buf_ostream position");
			m_cbOffset = this->size() + (std::size_t) off;
		}else
			throw std::invalid_argument("Invalid buf_ostream direction");
	}
private:
	class data_block
	{
		std::uint8_t* m_pData = nullptr;
		std::size_t m_cbData = 0;
	public:
		data_block() = default;
		data_block(std::uint8_t* pData, std::size_t cbData):m_pData(pData), m_cbData(cbData) {}
		data_block(const data_block&) = delete;
		data_block(data_block&& r)
		{
			using std::swap;
			swap(m_pData, r.m_pData);
			swap(m_cbData, r.m_cbData);
		}
		data_block& operator=(const data_block& r) = delete;
		data_block& operator=(data_block&& r)
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
		~data_block()
		{
			if (m_pData)
				delete [] m_pData;
		}
		std::uint8_t* data() const
		{
			return m_pData;
		}
		std::size_t size() const
		{
			return m_cbData;
		}
	};

	mutable std::vector<std::uint8_t> m_buf;
	mutable std::list<data_block> m_lst_buf;
	mutable std::size_t m_cbListSize = std::size_t();
	std::size_t m_cbOffset = std::size_t();

	auto make_list_data_block(const void* pData, std::size_t cbData, std::size_t cbFillBefore = 0) const -> data_block
	{
		auto cb = cbData + cbFillBefore;
		if (!cb)
			return data_block();
		auto ptr = std::make_unique<std::uint8_t[]>(cb);
		std::fill_n(static_cast<const std::uint8_t*>(pData), cbFillBefore, std::uint8_t());
		std::copy(static_cast<const std::uint8_t*>(pData) + cbFillBefore, static_cast<const std::uint8_t*>(pData) + cb, ptr.get());
		return data_block(ptr.release(), cbData);
	}
	const buf_ostream& serialize(std::size_t cbExtra = std::size_t()) const
	{
		m_buf.reserve(m_buf.size() + m_cbListSize);
		for (auto& nd:m_lst_buf)
			m_buf.emplace_back(nd.data(), nd.size());
		m_lst_buf.clear();
		m_cbListSize = std::size_t();
		return *this;
	}
};

template <class T>
buf_ostream& operator<<(buf_ostream& os, const T& obj)
{
	return os.write(obj);
}

struct temp_path:std::filesystem::path
{
	temp_path():std::filesystem::path(get_file_path()) {}
	~temp_path()
	{
		std::error_code ec;
		remove(*this, ec);
	}
private:
	static unsigned suffix;
	static std::filesystem::path get_file_path()
	{
		std::filesystem::path path;
		do
		{
			path = std::filesystem::temp_directory_path() / (std::string("coverter_buffer_") + std::to_string(++suffix));
		}while (exists(status(path)));
		return path;
	}
};

#endif // IMPL_BUF_OSTREAM_H
