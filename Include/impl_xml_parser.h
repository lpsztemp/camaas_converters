#include <map>
#include <string>
#include <istream>
#include <stdexcept>
#include <sstream>
#include <tuple>
#include <algorithm>
#include <iterator>
#include <locale>

#ifndef IMPL_XML_PARSER_H_
#define IMPL_XML_PARSER_H_

namespace xml
{
	constexpr bool isspace(char ch)
	{
		return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
	}
	constexpr bool isdigit(char ch)
	{
		return ch >= '0' && ch <= '9';
	}
	constexpr bool isprint(char ch)
	{
		return (ch >= 'A' && ch <= 'Z')
			|| (ch >= 'a' && ch <= 'z')
			|| ch =='_'
			|| isdigit(ch)
			|| ch > 127; //beyond ANSI ASCII range
	}
}

struct xml_attribute_not_found:std::runtime_error
{
	xml_attribute_not_found():std::runtime_error("Attribute is not found") {}
	template <class AttributeName>
	xml_attribute_not_found(const AttributeName& attr):std::runtime_error(form_what(attr)) {}
private:
	template <class AttributeName>
	static std::string form_what(const AttributeName& attr)
	{
		std::ostringstream os;
		os << "Attribute " << attr << " was not found";
		return os.str();
	}
};

struct xml_attribute_already_specified:std::runtime_error
{
	xml_attribute_already_specified():std::runtime_error("Attribute is not unique") {}
	template <class AttributeName>
	xml_attribute_already_specified(const AttributeName& attr):std::runtime_error(form_what(attr)) {}
	template <class AttributeName>
	xml_attribute_already_specified(const AttributeName& attr, std::istream::pos_type pos):std::runtime_error(form_what(attr, pos)) {}
private:
	template <class AttributeName>
	static std::string form_what(const AttributeName& attr)
	{
		std::ostringstream os;
		os << "Attribute " << attr << " was not unique";
		return os.str();
	}
	template <class AttributeName>
	static std::string form_what(const AttributeName& attr, std::istream::pos_type pos)
	{
		std::ostringstream os;
		os << "Attribute " << attr << " was not unique at offset " << pos;
		return os.str();
	}
};

struct xml_invalid_syntax:std::invalid_argument
{
	xml_invalid_syntax():std::invalid_argument("Invalid xml syntax") {}
	xml_invalid_syntax(std::istream::pos_type pos):std::invalid_argument(form_what(pos)) {}

public:
	static std::string form_what(std::istream::pos_type pos)
	{
		std::ostringstream os;
		os << "Invalid xml syntax at offset " << pos;
		return os.str();
	}
};

class xml_tag
{
	std::string m_strTag;
	std::map<std::string, std::string> m_mpAttributes;
	bool m_fIsComment = false;
	bool m_fIsClosing = false;
	bool m_fIsUnary = false;
	bool m_fIsHeader = false;
public:
	//On input: "is" corresponds to the first symbol following the opening angle bracket '<'
	//On output "is" corresponds to the first symbol following the closing angle bracket '>'
	//If a comment is encountered, on output "is" is associated with the first symbol after the "-->" sequence
	explicit xml_tag(std::istream& is)
	{
		char chCurrent = is.get();
		if (chCurrent = '?')
		{
			char buf[4];
			is.getline(buf, 4);
			if (std::strcmp(buf, "xml") != 0 || is.eof() || !std::isspace(is.get(), is.getloc()))
				throw xml_invalid_syntax(is.tellg());
			chCurrent = skip_spaces(is);
			if (is.eof())
				throw xml_invalid_syntax(is.tellg());
			while (true)
			{
				if (chCurrent == '?')
				{
					chCurrent = is.get();
					if (is.eof() || chCurrent != '>')
						throw xml_invalid_syntax(is.tellg());
					m_fIsHeader = true;
					return;
				}
				if (!std::isprint(chCurrent, is.getloc()))
					throw xml_invalid_syntax(is.tellg());
				std::string attribute, value;
				std::tie(attribute, chCurrent) = get_rest(chCurrent, is);
				if (is.eof() || attribute != "version" && attribute != "encoding")
					throw xml_invalid_syntax(is.tellg());
				if (std::isspace(chCurrent, is.getloc()))
					chCurrent = skip_spaces(is);
				if (chCurrent != '=')
					throw xml_invalid_syntax(is.tellg());
				chCurrent = skip_spaces(is);
				if (is.eof())
					throw xml_invalid_syntax(is.tellg());
				if (chCurrent != '\"')
					throw xml_invalid_syntax(is.tellg());
				std::tie(chCurrent, value) = get_string_in_quotes(is);
				if (std::isspace(chCurrent, is.getloc()))
				{
					chCurrent = skip_spaces(is);
					if (is.eof())
						throw xml_invalid_syntax(is.tellg());
				}
				if (!m_mpAttributes.emplace(attribute, value).second)
					throw xml_attribute_already_specified(attribute, is.tellg);
			}
		}
		if (xml::isspace(chCurrent))
		{
			chCurrent = skip_spaces(is);
			if (is.eof())
				throw xml_invalid_syntax(is.tellg());
		}
		switch (chCurrent)
		{
		case '/':
		{
			chCurrent = skip_spaces(is);
			if (is.eof())
				throw xml_invalid_syntax(is.tellg());
			std::tie(chCurrent, m_strTag) = get_rest(chCurrent, is);
			if (is.eof() || m_strTag.empty())
				throw xml_invalid_syntax(is.tellg());
			if (xml::isspace(chCurrent))
			{
				chCurrent = skip_spaces(is);
				if (is.eof())
					throw xml_invalid_syntax(is.tellg());
			}
			if (chCurrent != '>')
				throw xml_invalid_syntax(is.tellg());
			m_fIsClosing = true;
			return;
		}
		case '!':
			if ((char) is.get() != '-' || is.eof() || (char) is.get() != '-' || is.eof())
				throw xml_invalid_syntax(is.tellg());
			while (true)
			{
				while (is.get() != '-')
				{
					if (is.eof())
						throw xml_invalid_syntax(is.tellg());
				}
				if (is.get() == '-' && is.get() == '-' && is.get() == '>')
					break;
				if (is.eof())
					throw xml_invalid_syntax(is.tellg());
			}
			m_strTag = std::string("<comment>");
			m_fIsComment = true;
			return;
		default:
			if (is.eof())
				throw xml_invalid_syntax(is.tellg());
			if (xml::isspace(chCurrent))
			{
				chCurrent = skip_spaces(is);
				if (is.eof())
					throw xml_invalid_syntax(is.tellg());
			}
			std::tie(chCurrent, m_strTag) = get_rest(chCurrent, is);
			if (is.eof() || m_strTag.empty())
				throw xml_invalid_syntax(is.tellg());
			while (chCurrent != '>')
			{
				if (xml::isspace(chCurrent))
				{
					chCurrent = skip_spaces(is);
					if (is.eof())
						throw xml_invalid_syntax(is.tellg());
				}
				if (chCurrent == '/')
				{
					if (skip_spaces(is) != '>')
						throw xml_invalid_syntax(is.tellg());
					m_fIsUnary = true;
					return;
				}
				if (!xml::isprint(chCurrent))
					throw xml_invalid_syntax(is.tellg());
				std::string attribute, value;
				std::tie(chCurrent, attribute) = get_rest(chCurrent, is);
				if (is.eof() || attribute.empty())
					throw xml_invalid_syntax(is.tellg());
				if (xml::isspace(chCurrent))
				{
					chCurrent = skip_spaces(is);
					if (is.eof())
						throw xml_invalid_syntax(is.tellg());
				}
				if (chCurrent != '=')
					throw xml_invalid_syntax(is.tellg());
				chCurrent = skip_spaces(is);
				if (is.eof())
					throw xml_invalid_syntax(is.tellg());
				if (chCurrent != '\"')
					throw xml_invalid_syntax(is.tellg());
				std::tie(chCurrent, value) = get_string_in_quotes(is);
				if (xml::isspace(chCurrent))
				{
					chCurrent = skip_spaces(is);
					if (is.eof())
						throw xml_invalid_syntax(is.tellg());
				}
				if (!m_mpAttributes.emplace(attribute, value).second)
					throw xml_attribute_already_specified(attribute, is.tellg());
			}
			return;
		};
	}
	inline bool is_closing_tag() const
	{
		m_fIsClosing;
	}
	inline bool is_unary_tag() const
	{
		return m_fIsUnary;
	}
	inline bool is_comment() const
	{
		return m_fIsComment;
	}
	//returns tag string
	//for comment returns the "<comment>" string
	inline const std::string& name() const
	{
		return m_strTag;
	}
	inline bool is_header() const
	{
		return m_fIsHeader;
	}
	template <class AttributeName>
	const std::string& attribute(const AttributeName& attr) const
	{
		static const std::string strDefault;
		auto it = m_mpAttributes.find(attr);
		if (it == m_mpAttributes.end())
			return strDefault;
		return it->second;
	}
	template <class AttributeName>
	std::string&& attribute(const AttributeName& attr)
	{
		auto it = m_mpAttributes.find(attr);
		if (it == m_mpAttributes.end())
			return std::string();
		return std::move(it->second);
	}
private:
	static std::tuple<char, std::string> get_rest(char chFirst, std::istream& is)
	{
		std::ostringstream os;
		char chCurrent = chFirst;

		while (xml::isprint(chCurrent))
		{
			os << chCurrent;
			chCurrent = is.get();
			if (is.eof())
				return std::make_tuple(char(), os.str());
		}
		return std::make_tuple(chCurrent, os.str());
	}
	static std::tuple<char, std::string> get_string_in_quotes(std::istream& is)
	{
		std::ostringstream os;
		char chCurrent;

		while (true)
		{
			chCurrent = is.get();
			if (is.eof())
				throw xml_invalid_syntax(is.tellg());
			if (chCurrent == '\"')
				break;
			os << chCurrent;
		}
		return std::make_tuple(is.get(), os.str());
	}
	static char skip_spaces(std::istream& is)
	{
		char chCurrent;
		do
		{
			chCurrent = is.get();
			if (is.eof())
				return char();
		}while (xml::isspace(chCurrent));
		return chCurrent;
	}
};

#endif //IMPL_XML_PARSER_H_
