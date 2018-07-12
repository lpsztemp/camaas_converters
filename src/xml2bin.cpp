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
				std::ofstream os_tmp(temp_path(), std::ios_base::binary | std::ios_base::out)
				
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

template <class DomainName, class DomainConvert>
static void convert_start(const DomainName& strDomain, std::istream& is, std::ostream& os, DomainConvert conv)
{
	using namespace std;
	auto it_is = find_if_not(istream_iterator<char>(is), istream_iterator<char>(), xml::isspace);
	if (it_is == istream_iterator<char>() || is.get() != '<')
		throw xml_invalid_syntax(is.tellg());
	auto tag = xml_tag(is);
	if (!tag.m_fIsHeader)
		throw invalid_xml_data("XML header is not specified");
	if (tag.attribute("encoding") != "utf-8")
		throw invalid_xml_data("XML encoding must be utf-8");
	tag = xml_tag(is);
	if (tag.name() != "model")
		throw improper_xml_tag(tag.name());
	

}

namespace Implementation
{
	void xml2bin_arch_ac(std::istream& is, std::ostream& os)
	{
	}
	void xml2bin_radio_hf(std::istream& is, std::ostream& os)
	{
	}
	void hgt2bin_radio_hf(std::istream& is, std::ostream& os)
	{
	}
} //Implementation
