#include <xml2bin.h>
#include <impl_radio_hf_domain_xml2bin.h>
#include <impl_arch_ac_domain_xml2bin.h>
#include <impl_xml_parser.h>
#include <impl_buf_ostream.h>
#include <algorithm>
#include <iterator>
#include <functional>
#include <list>
#include <filesystem>
#include <fstream>

struct improper_xml_tag:std::invalid_argument
{
	improper_xml_tag():std::invalid_argument("Improper XML tag") {}
	template <class TagName>
	improper_xml_tag(const TagName& tag):std::invalid_argument(form_what(tag)) {}
private:
	template <class TagName>
	static std::string form_what(const TagName& tag)
	{
		std::ostringstream ss;
		ss << "Improper XML tag \"" << tag << "\"";
		return ss.str();
	}
};

struct invalid_xml_data:std::invalid_argument
{
	invalid_xml_data():std::invalid_argument("Invalid xml data") {}
	invalid_xml_data(std::istream::pos_type pos):std::invalid_argument(form_what(pos)) {}
	template <class WhatString>
	invalid_xml_data(const WhatString& strWhat):std::invalid_argument(strWhat) {}

public:
	static std::string form_what(std::istream::pos_type pos)
	{
		std::ostringstream os;
		os << "Invalid xml data at offset " << pos;
		return os.str();
	}
};


static void skip_domain_data(std::istream& is) //"is" is associated with the first character after the closing '>'
{
	int level = 1;
	char linebuf[100];
	do
	{
		is.getline(linebuf, sizeof(linebuf), '<');
		if (is.eof())
			throw xml_invalid_syntax(is.tellg());
		auto tag = xml_tag(is);
		if (tag.name() != "domain")
			continue;
		if (!tag.is_closing_tag())
		{
			++level;
			continue;
		}
		--level;
	}while (level != 0);
}

//conversion of domain_data:
/*
struct
{
	void model_domain_data(is, os);
	void poly_domain_data(is, os);
	void face_domain_data(is, os);
	void source_domain_data(is, os);
	void plain_domain_data(is, os);
};
*/

template <class T, class = void> struct has_model_domain_data:std::false_type {};
template <class T> struct has_model_domain_data<T, std::void_t<decltype(std::declval<T>().model_domain_data(std::declval<std::istream&>(), std::declval<std::ostream&>()))>>:std::true_type {};
template <class T, class = void> struct has_poly_domain_data:std::false_type {};
template <class T> struct has_poly_domain_data<T, std::void_t<decltype(std::declval<T>().poly_domain_data(std::declval<std::istream&>(), std::declval<std::ostream&>()))>>:std::true_type {};
template <class T, class = void> struct has_face_domain_data:std::false_type {};
template <class T> struct has_face_domain_data<T, std::void_t<decltype(std::declval<T>().face_domain_data(std::declval<std::istream&>(), std::declval<std::ostream&>()))>>:std::true_type {};
template <class T, class = void> struct has_source_domain_data:std::false_type {};
template <class T> struct has_source_domain_data<T, std::void_t<decltype(std::declval<T>().source_domain_data(std::declval<std::istream&>(), std::declval<std::ostream&>()))>>:std::true_type {};
template <class T, class = void> struct has_plain_domain_data:std::false_type {};
template <class T> struct has_plain_domain_data<T, std::void_t<decltype(std::declval<T>().plain_domain_data(std::declval<std::istream&>(), std::declval<std::ostream&>()))>>:std::true_type {};

template <class Convert>
auto convert_model_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_model_domain_data<Convert&>::value>
{
	return conv.model_domain_data(is, os);
}
template <class Convert>
void convert_model_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_model_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}
template <class Convert>
auto convert_poly_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_poly_domain_data<Convert&>::value>
{
	return conv.poly_domain_data(is, os);
}
template <class Convert>
void convert_poly_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_poly_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}

template <class Convert>
auto convert_face_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_face_domain_data<Convert&>::value>
{
	return conv.face_domain_data(is, os);
}
template <class Convert>
void convert_face_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_face_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}

template <class Convert>
auto convert_source_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_source_domain_data<Convert&>::value>
{
	return conv.source_domain_data(is, os);
}
template <class Convert>
void convert_source_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_source_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}

template <class Convert>
auto convert_plain_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_plain_domain_data<Convert&>::value>
{
	return conv.plain_domain_data(is, os);
}
template <class Convert>
void convert_plain_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_plain_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}

template <class DomainName, class DomainConvert>
static void convert_model(const xml_tag& rModelTag, const DomainName& strDomain, std::istream& is, std::ostream& os, DomainConvert conv)
{
	auto strAttr = rModelTag.attribute("cx");
	if (strAttr.empty())
		throw xml_attribute_not_found("cx");
	auto cx = stod(strAttr);
	auto strAttr = rModelTag.attribute("cy");
	if (strAttr.empty())
		throw xml_attribute_not_found("cy");
	auto cy = stod(strAttr);
	auto strAttr = rModelTag.attribute("cz");
	if (strAttr.empty())
		throw xml_attribute_not_found("cz");
	auto cz = stod(strAttr);
	auto strAttr = rModelTag.attribute("name");
	os << std::uint32_t(strAttr.size()) << strAttr << std::uint32_t(1) << std::uint32_t(3) << cx << cy << cz;
	std::list<std::filesystem::path> mpDomainData;
	while (true)
	{
		auto it_is = find_if_not(istream_iterator<char>(is), istream_iterator<char>(), xml::isspace);
		if (it_is == istream_iterator<char>())
			throw xml_invalid_syntax(is.tellg());
		if (*it_is != '<')
			throw invalid_xml_data(is.tellg());
		rModelTag = xml_tag(it);
		if (rModelTag.name() == "domain")
		{
			if (strDomain == rModelTag.attribute("name"))
			{
				std::ofstream os_tmp(temp_path(), std::ios_base::binary | std::ios_base::out);
				os_tmp << std::uint32_t(strDomain.size());
				os_tmp.write(strDomain.data(), strDomain.size());
				buf_ostream os_tmp_2;
				conv.model_domain_data(is, os_tmp);
				os << std::uint32_t(os_tmp.size());
			}
		}else if (rModelTag.name() == "polyobject")
		{
		}else if (rModelTag.name() == "sourceobject")
		{
		}else if (rModelTag.name() =="plainobject")
		{
		}else if (rModelTag.name() == "model" && rModelTag.is_closing_tag())
			break;
		else
			throw improper_xml_tag(is.tellg());


	};
}

class conversion_state
{
	std::string m_strDomain;
	std::size_t m_cObjects = std::size_t();
	std::ostream::pos_type m_cObjectsPos = std::ostream::pos_type(-1);
	std::ostream* m_pOs = nullptr;
	std::stack<std::istream*> m_prev;
public:
	template <class DomainName>
	conversion_state(DomainName&& domain, std::ostream& os):m_strDomain(std::forward<DomainName>(domain)), m_pOs(std::addressof(os)) {}
	conversion_state(const conversion_state&) = delete;
	conversion_state& operator=(const conversion_state&) = delete;

	void next_xml(std::istream& is)
	{
		using namespace std;
		auto it_is = find_if_not(istream_iterator<char>(is), istream_iterator<char>(), xml::isspace);
		if (it_is == istream_iterator<char>() || is.get() != '<')
			throw xml_invalid_syntax(is.tellg());
		auto tag = xml_tag(is);
		if (!tag.is_header())
			throw invalid_xml_data("XML header is not specified");
		if (tag.attribute("encoding") != "utf-8")
			throw invalid_xml_data("XML encoding must be utf-8");
		tag = xml_tag(is);
		if (tag.name() != "model")
			throw improper_xml_tag(tag.name());
		//convert_model(tag, m_strDomain, is, *m_pOs, 0);

	}
	void next_hgt(std::istream& is);
	inline bool is_model_specified() const
	{
		return m_cObjectsPos != std::ostream::pos_type(-1);
	}
	void finalize();
};

namespace Implementation
{
	std::unique_ptr<void> xml2bin_set(const std::string& domain, std::ostream& os)
	{
	}
	void xml2bin_next_xml(const std::unique_ptr<void>& state, std::istream& is)
	{
	}
	void xml2bin_next_hgt(const std::unique_ptr<void>& state, std::istream& is)
	{
	}
	void xml2bin_finalize(const std::unique_ptr<void>& state)
	{
	}
} //Implementation
