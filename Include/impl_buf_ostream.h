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

struct memstreambuf:std::streambuf
{
	typedef std::streambuf base_buf_t;
	typedef std::uint8_t value_type;
	typedef base_buf_t::char_type char_type;
	typedef std::size_t size_type;
	typedef base_buf_t::pos_type pos_type;
	typedef base_buf_t::traits_type traits_type;
	memstreambuf() = default;
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
	virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, 
		std::ios_base::openmode /*which*/ = std::ios_base::in | std::ios_base::out) final
	{
		if (dir == std::ios_base::beg)
		{
			if (off < 0)
				throw std::invalid_argument("Invalid memstreambuf position");
			m_cbOffset = std::size_t(off);
		}else if (dir == std::ios_base::cur)
		{
			if ((std::ptrdiff_t) m_cbOffset + off < 0)
				throw std::invalid_argument("Invalid memstreambuf position");
			m_cbOffset += (std::size_t) off;
		}else if (dir == std::ios_base::end)
		{
			if ((std::ptrdiff_t) this->size() + off < 0)
				throw std::invalid_argument("Invalid memstreambuf position");
			m_cbOffset = this->size() + (std::size_t) off;
		}else
			throw std::invalid_argument("Invalid memstreambuf direction");
		return m_cbOffset;
	}
	virtual pos_type seekpos(pos_type pos,
		std::ios_base::openmode /*which*/ = std::ios_base::in | std::ios_base::out) noexcept final
	{
		return m_cbOffset = pos;
	}
	virtual std::streamsize xsputn(const char_type* pInput, std::streamsize cbHowMany) final
	{
		auto cb = m_cbOffset + cbHowMany;
		auto pbInput = reinterpret_cast<const value_type*>(pInput);
		if (m_buf.empty())
		{
			m_buf.reserve(cb);
			std::fill_n(std::back_inserter(m_buf), m_cbOffset, std::uint8_t());
			std::copy(pbInput, pbInput + cbHowMany, std::back_inserter(m_buf));
			m_cbOffset += cbHowMany;
		}else if (m_cbOffset >= this->size())
		{
			auto cbFill = m_cbOffset - this->size();
			auto block = this->make_list_data_block(pbInput, cbHowMany, cbFill);
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
			std::copy(pbInput, pbInput + cbInPlace, m_buf.begin() + m_cbOffset);
			std::copy(pbInput + cbInPlace, pbInput + cbHowMany, std::back_inserter(m_buf));
			m_cbOffset = cb;
		}
		return cbHowMany;
	}
	virtual int_type overflow(int_type ch = traits_type::eof())
	{
		if (!traits_type::eq_int_type(ch, traits_type::eof()))
		{
			auto l_ch = char_type(ch);
			xsputn(&l_ch, 1);
		}
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
		auto ptr = std::make_unique<value_type[]>(cb);
		std::fill_n(static_cast<const value_type*>(pData), cbFillBefore, value_type());
		std::copy(static_cast<const value_type*>(pData) + cbFillBefore, static_cast<const value_type*>(pData) + cb, ptr.get());
		return data_block(ptr.release(), cbData);
	}
	const memstreambuf& serialize(std::size_t cbExtra = std::size_t()) const
	{
		m_buf.reserve(m_buf.size() + m_cbListSize);
		for (auto& nd:m_lst_buf)
			m_buf.emplace_back(nd.data(), nd.size());
		m_lst_buf.clear();
		m_cbListSize = std::size_t();
		return *this;
	}
};

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
