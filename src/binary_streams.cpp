#include <binary_streams.h>
#include <locale>
#include <codecvt>

#if CPP17_FILESYSTEM_SUPPORT
unsigned temp_path::suffix = unsigned();
#endif // CPP17_FILESYSTEM_SUPPORT

buf_ostream& buf_ostream::write(const void* pInput, std::size_t cbHowMany)
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

buf_ostream::pos_type buf_ostream::tellp() const
{
	return m_cbOffset;
}
buf_ostream& buf_ostream::seekp(buf_ostream::pos_type pos)
{
	m_cbOffset = pos;
	return *this;
}

buf_ostream& buf_ostream::seekp(std::ptrdiff_t off, std::ios_base::seekdir dir)
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
	return *this;
}

buf_ostream::data_block buf_ostream::make_list_data_block(const void* pData, std::size_t cbData, std::size_t cbFillBefore) const
{
	auto cb = cbData + cbFillBefore;
	if (!cb)
		return data_block();
	auto ptr = std::make_unique<value_type[]>(cb);
	std::fill_n(ptr.get(), cbFillBefore, value_type());
	std::copy(static_cast<const value_type*>(pData), static_cast<const value_type*>(pData) + cb, ptr.get() + cbFillBefore);
	return data_block(ptr.release(), cbData);
}

const buf_ostream& buf_ostream::serialize(std::size_t cbExtra) const
{
	m_buf.reserve(m_buf.size() + m_cbListSize);
	for (auto& nd:m_lst_buf)
		m_buf.insert(m_buf.end(), nd.data(), nd.data() + nd.size());
	m_lst_buf.clear();
	m_cbListSize = std::size_t();
	return *this;
}

binary_ofstream::binary_ofstream(const std::string_view& path, bool fDiscardIfExists)
	:m_os(std::string(path), std::ios_base::out | std::ios_base::binary | (fDiscardIfExists?std::ios_base::trunc:std::ios_base::app))
{
	if (off_type(this->tellp()) != 0)
	{
		m_os.close();
		this->setstate(std::ios_base::failbit);
	}
}

#if CPP17_FILESYSTEM_SUPPORT
binary_ofstream::binary_ofstream(const std::filesystem::path& path, bool fDiscardIfExists)
	:m_os(path, std::ios_base::out | std::ios_base::binary | (fDiscardIfExists?std::ios_base::trunc:std::ios_base::app))
{
	if (off_type(this->tellp()) != 0)
	{
		m_os.close();
		this->setstate(std::ios_base::failbit);
	}
}
#endif //CPP17_FILESYSTEM_SUPPORT

void binary_ofstream::open(const std::string_view& path, bool fDiscardIfExists)
{
	m_os.open(std::string(path), std::ios_base::out | std::ios_base::binary | (fDiscardIfExists?std::ios_base::trunc:std::ios_base::app));
	if (off_type(m_os.tellp()) != 0)
	{
		m_os.close();
		this->setstate(std::ios_base::failbit);
		if ((m_os.exceptions() & std::ios_base::failbit) != 0)
			throw std::ios_base::failure("binary_ofstream::open");
	}
}

#if CPP17_FILESYSTEM_SUPPORT
void binary_ofstream::open(const std::filesystem::path& path, bool fDiscardIfExists)
{
	m_os.open(path, std::ios_base::out | std::ios_base::binary | (fDiscardIfExists?std::ios_base::trunc:std::ios_base::app));
	if (off_type(m_os.tellp()) != 0)
	{
		m_os.close();
		this->setstate(std::ios_base::failbit);
		if ((m_os.exceptions() & std::ios_base::failbit) != 0)
			throw std::ios_base::failure("binary_ofstream::open");
	}
}
#endif //CPP17_FILESYSTEM_SUPPORT

binary_ofstream& binary_ofstream::write(const void* pInput, std::size_t cbHowMany)
{
	m_os.write(reinterpret_cast<const char*>(pInput), cbHowMany);
	return *this;
}
binary_ofstream::pos_type binary_ofstream::tellp() const
{
	return pos_type(m_os.tellp());
}
binary_ofstream& binary_ofstream::seekp(binary_ofstream::pos_type pos)
{
	m_os.seekp(pos);
	return *this;
}
binary_ofstream& binary_ofstream::seekp(std::ptrdiff_t off, std::ios_base::seekdir dir)
{
	m_os.seekp(off, dir);
	return *this;
}

std::string encode_string(const std::wstring& str)
{
	typedef std::codecvt_utf8<wchar_t> codecvt;
	return std::wstring_convert<codecvt>().to_bytes(str);
}
