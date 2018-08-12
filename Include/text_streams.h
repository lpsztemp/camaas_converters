#include <basedefs.h>
#include <istream>
#include <locale>
#include <streambuf>
#include <memory>
#include <climits>
#include <ostream>

#include <fstream>
#include <string_view>
#if CPP17_FILESYSTEM_SUPPORT
#include <filesystem>
#endif

#ifndef TEXT_STREAMS_H_
#define TEXT_STREAMS_H_

struct resource_locator
{
	unsigned column;
	unsigned row;
	std::string resource_id;
};

inline std::ostream& operator<<(std::ostream& os, const resource_locator& loc)
{
	if (!loc.resource_id.empty())
		os << loc.resource_id << " ";
	return os << "(column " << loc.column << ", row " << loc.row << ")";
}

enum class TextEncoding
{
	Default,
	UTF8,
	UTF16LE,
	UTF16BE,
	ANSI,
	Windows_1251
};

class text_istream:public std::wistream
{
	typedef std::istream stream_type;
public:
	typedef wchar_t char_type;
	typedef std::char_traits<char_type> traits_type;
	typedef traits_type::int_type int_type;
	typedef traits_type::off_type off_type;
	typedef traits_type::pos_type pos_type;
	struct locator
	{
		unsigned col, row;
	};
	

	struct streambuf:std::wstreambuf
	{
		virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
		virtual pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
		virtual int_type pbackfail(int_type c = traits_type::eof());
		virtual int_type underflow();
		streambuf() = default;
		streambuf(std::streambuf* pBuf, TextEncoding encoding = TextEncoding::UTF8);
		streambuf(streambuf&&) = default;
		streambuf& operator=(streambuf&&) = default;
		void set_encoding(TextEncoding encoding);
		inline const locator& get_locator() const noexcept
		{
			return m_locator;
		}
	private:
		char m_mb_buf[MB_LEN_MAX];
		std::size_t m_mb_buf_off = sizeof(m_mb_buf);
		//std::size_t m_mb_buf_valid = std::size_t();
		std::size_t m_mb_buf_len = sizeof(m_mb_buf); //number of bytes in the buffer
		std::mbstate_t m_conv_state = std::mbstate_t();
		char_type m_chLast = char_type();

		typedef std::streambuf implbuf;
		typedef implbuf::char_type impl_char_type;
		typedef implbuf::traits_type impl_traits_type;
		typedef implbuf::int_type impl_int_type;
		typedef implbuf::off_type impl_off_type;
		typedef implbuf::pos_type impl_pos_type;

		std::streambuf* m_pBufImpl = nullptr;

		locator m_locator = {0u, 1u};
		unsigned m_prev_col = 0u;

		int_type readone();
	public:
		inline std::streambuf* rdbuf() const noexcept
		{
			return m_pBufImpl;
		}
		inline std::streambuf* rdbuf(std::streambuf* pNewBuf) noexcept
		{
			auto old = m_pBufImpl;
			m_pBufImpl = pNewBuf;
			return old;
		}
	};
protected:
	inline text_istream():std::wistream(nullptr) {}
	enum class use_bom_t {use_bom};
public:
	static constexpr use_bom_t use_bom = use_bom_t::use_bom;
	inline explicit text_istream(std::istream& is, TextEncoding enc = TextEncoding::UTF8):std::wistream(nullptr), /*m_pIs(&is), */m_buf(is.rdbuf(), enc)
	{
		this->rdbuf(&m_buf);
		if (is.fail())
			this->setfail();
	}
	text_istream(std::istream& is, use_bom_t);
	inline text_istream(text_istream&& right):std::wistream(std::move(right)), /*m_pIs(right.m_pIs), */m_buf(std::move(right.m_buf))
	{
		auto state = this->rdstate();
		this->rdbuf(&m_buf);
		this->setstate(state);
	}
	text_istream& operator=(text_istream&& right);
	inline virtual ~text_istream() = default;


	inline text_istream& set_encoding(TextEncoding encoding)
	{
		m_buf.set_encoding(encoding);
		return *this;
	}
	inline const locator& get_locator() const
	{
		return m_buf.get_locator();
	}
	virtual const std::wstring& get_resource_id() const;
	resource_locator get_resource_locator() const;
	inline const streambuf* rdbuf() const noexcept
	{
		return &m_buf;
	}
	inline streambuf* rdbuf() noexcept
	{
		return &m_buf;
	}
	inline streambuf* rdbuf(streambuf* pNewBuf)
	{
		if (pNewBuf != nullptr)
			m_buf = std::move(*pNewBuf);
		return static_cast<streambuf*>(this->std::wistream::rdbuf(pNewBuf));
	}
private:
	//stream_type* m_pIs;
	mutable streambuf m_buf;
	static const std::wstring m_strUnknownResourceId;

	inline void setfail()
	{
		this->setstate(std::ios_base::failbit);
		if (this->exceptions() & std::ios_base::failbit)
			throw std::ios_base::failure("text_istream::setfail");
	}
};

class text_file_istream:public text_istream
{
public:
	virtual const std::wstring& get_resource_id() const;
	template <class CharT, class TraitsT>
	text_file_istream(const std::basic_string_view<CharT, TraitsT>& path, TextEncoding encoding)
		:m_path(path_init(path)), m_is(stream_init(path))
	{
		static_cast<text_istream&>(*this) = text_istream(m_is, encoding);
	}
	template <class CharT, class TraitsT>
	text_file_istream(const std::basic_string_view<CharT, TraitsT>& path)
		:m_path(path_init(path)), m_is(stream_init(path))
	{
		static_cast<text_istream&>(*this) = text_istream(m_is, text_istream::use_bom);
	}
#if CPP17_FILESYSTEM_SUPPORT
	inline text_file_istream(std::filesystem::path path, TextEncoding encoding):m_path(path.wstring()), m_is(path, std::ios_base::in) 
	{
		static_cast<text_istream&>(*this) = text_istream(m_is, encoding);
	}
	inline text_file_istream(std::filesystem::path path):m_path(path.wstring()), m_is(path, std::ios_base::in) 
	{
		static_cast<text_istream&>(*this) = text_istream(m_is, text_istream::use_bom);
	}
#endif //CPP17_FILESYSTEM_SUPPORT
	inline text_file_istream(text_file_istream&& right):text_istream(static_cast<text_istream&&>(right)), m_path(std::move(right.m_path)), m_is(std::move(right.m_is)) 
	{
		this->rdbuf()->rdbuf(m_is.rdbuf());
	}
	text_file_istream& operator=(text_file_istream&&) = default;
private:
	std::wstring m_path;
	std::ifstream m_is;

	static inline std::wstring path_init(const std::string_view& path)
	{
		return std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>>(
			&std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale(""))
			).from_bytes(path.data(), path.data() + path.size());
	}
	static inline std::ifstream stream_init(const std::string_view& path)
	{
		return std::ifstream{std::string(path), std::ios_base::in};
	}
	static inline std::wstring path_init(const std::wstring_view& path)
	{
		return std::wstring(path);
	}
	static inline std::ifstream stream_init(const std::wstring_view& path)
	{
		return std::ifstream{std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>>(
			&std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale(""))
			).to_bytes(path.data(), path.data() + path.size()), std::ios_base::in};
	}
};

#endif //TEXT_STREAMS_H_
