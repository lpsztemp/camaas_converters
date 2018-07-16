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

private:
	static std::string form_what(std::istream::pos_type pos)
	{
		std::ostringstream os;
		os << "Invalid xml data at offset " << pos;
		return os.str();
	}
};

struct ambiguous_specification:std::invalid_argument
{
	ambiguous_specification():std::invalid_argument("Ambiguous model specification") {}
	template <class XmlEntityString>
	ambiguous_specification(const XmlEntityString& xml_entity):std::invalid_argument(form_what(xml_entity)) {}
private:
	template <class XmlEntityString>
	static std::string form_what(const XmlEntityString& xml_entity)
	{
		std::ostringstream ss;
		ss << "Ambiguous " << xml_entity;
		return ss.str();
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

template <class DomainConvert>
static void convert_poly(const xml_tag& rModelTag, std::istream& is, std::ostream& os, DomainConvert& conv)
{
}
template <class DomainConvert>
static void convert_source(const xml_tag& rModelTag, std::istream& is, std::ostream& os, DomainConvert& conv)
{
}
template <class DomainConvert>
static void convert_plain(const xml_tag& rModelTag, std::istream& is, std::ostream& os, DomainConvert& conv)
{
}

constexpr double unspecified_double()
{
	return std::numeric_limits<double>::infinity();
}

constexpr bool is_specified(double val)
{
	return val != unspecified_double();
}

struct point_t
{
	double x, y, z;
};

constexpr point_t unspecified_point()
{
	return {unspecified_double(), unspecified_double(), unspecified_double()};
}

constexpr bool is_specified(const point_t& val)
{
	return val.x != unspecified_double() && val.y != unspecified_double() && val.z != unspecified_double();
}

class conversion_state
{
	std::ostream* m_pOs = nullptr;
	std::istream* m_pHgt = nullptr;
	point_t m_size;
	std::string m_strModelName;

	typedef std::map<std::string, std::vector<std::uint8_t>> domain_data_map;
	domain_data_map m_mapDomainData;

	struct poly_data
	{
		struct face_data
		{
			std::list<point_t> lstVertices;
			domain_data_map mapDomainData;
		};
		std::string name;
		std::list<face_data> lstFaces;
		domain_data_map mapDomainData;
	};
	std::map<std::string, poly_data> m_polyNamedMap;
	std::list<poly_data> m_polyUnnamedList;

	struct source_data
	{
		point_t pos = unspecified_point();
		point_t dir = unspecified_point();
		point_t top = unspecified_point();
		domain_data_map mapDomainData;
		std::string name;
	};
	std::map<std::string, source_data> m_srcNamedMap;
	std::list<source_data> m_srcUnnamedList;

	struct plain_data
	{
		point_t pos = unspecified_point();
		point_t v1 = unspecified_point();
		point_t v2 = unspecified_point();
		domain_data_map mapDomainData;
		std::string name;
	};
	std::map<std::string, plain_data> m_plainNamedMap;
	std::list<plain_data> m_plainUnnamedList;		

	struct IConverter
	{
		virtual void model_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os) = 0;
		virtual void poly_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os) = 0;
		virtual void face_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os) = 0;
		virtual void source_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os) = 0;
		virtual void plain_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os) = 0;

		virtual ~IConverter() {}
	};
	template <class Converter>
	struct ConverterImpl:IConverter
	{
		virtual void model_domain_data(const std::string_view&, std::istream& is, std::ostream& os)
		{
			convert_model_domain_data(m_conv, is, os);
		}
		virtual void poly_domain_data(const std::string_view&, std::istream& is, std::ostream& os)
		{
			convert_poly_domain_data(m_conv, is, os);
		}
		virtual void face_domain_data(const std::string_view&, std::istream& is, std::ostream& os)
		{
			convert_face_domain_data(m_conv, is, os);
		}
		virtual void source_domain_data(const std::string_view&, std::istream& is, std::ostream& os)
		{
			convert_source_domain_data(m_conv, is, os);
		}
		virtual void plain_domain_data(const std::string_view&, std::istream& is, std::ostream& os)
		{
			convert_plain_domain_data(m_conv, is, os);
		}

		template <class Conv, class = std::enable_if_t<std::is_convertible_v<Conv&&, Converter>>>
		ConverterImpl(Conv&& conv):m_conv(std::forward<Conv>(conv)) {}
	private:
		Converter m_conv;
	};

	struct generalized_converter:IConverter
	{
		generalized_converter(std::initializer_list<std::pair<std::string, std::unique_ptr<IConverter>>> lst)
		{
			for (auto&& elem:lst)
				m_mpConv.emplace(std::move(elem));
		}
		virtual void model_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os)
		{
			auto itConv = m_mpConv.find(DomainName);
			if (itConv != m_mpConv.end())
				convert_model_domain_data(*itConv->second, is, os);
			else
				skip_domain_data(is);
		}
		virtual void poly_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os)
		{
			auto itConv = m_mpConv.find(DomainName);
			if (itConv != m_mpConv.end())
				convert_poly_domain_data(*itConv->second, is, os);
			else
				skip_domain_data(is);
		}
		virtual void face_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os)
		{
			auto itConv = m_mpConv.find(DomainName);
			if (itConv != m_mpConv.end())
				convert_face_domain_data(*itConv->second, is, os);
			else
				skip_domain_data(is);
		}
		virtual void source_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os)
		{
			auto itConv = m_mpConv.find(DomainName);
			if (itConv != m_mpConv.end())
				convert_source_domain_data(*itConv->second, is, os);
			else
				skip_domain_data(is);
		}
		virtual void plain_domain_data(const std::string_view& DomainName, std::istream& is, std::ostream& os)
		{
			auto itConv = m_mpConv.find(DomainName);
			if (itConv != m_mpConv.end())
				convert_plain_domain_data(*itConv->second, is, os);
			else
				skip_domain_data(is);
		}
	private:
		struct less_transparent
		{
			bool operator()(const std::string_view& lhs, const std::string_view& rhs) const
			{
				return lhs < rhs;
			}
			typedef void is_transparent;
		};
		std::map<std::string, std::unique_ptr<IConverter>, less_transparent> m_mpConv;
	};
	std::unique_ptr<IConverter> m_pConv;
	
public:
	template <class DomainName>
	conversion_state(DomainName&& domain, std::ostream& os):m_pOs(std::addressof(os))
	{
		if (domain == "radio_hf")
			m_pConv.reset(new ConverterImpl<radio_hf_convert>(radio_hf_convert()));
		else if (domain == "arch_ac")
			m_pConv.reset(new ConverterImpl<radio_hf_convert>(arch_ac_convert()));
		else
			throw std::invalid_argument("Unknown domain name");
	}
	explicit conversion_state(std::ostream& os):m_pOs(std::addressof(os))
	{
		m_pConv.reset(new generalized_converter({
			{std::string("radio_hf"),	std::unique_ptr<IConverter>(new ConverterImpl<radio_hf_convert>	(radio_hf_convert()))},
			{std::string("arch_ac"),	std::unique_ptr<IConverter>(new ConverterImpl<arch_ac_convert>	(arch_ac_convert()))}
		}));
	}
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
		auto it_is = find_if_not(istream_iterator<char>(is), istream_iterator<char>(), xml::isspace);
		if (it_is == istream_iterator<char>() || is.get() != '<')
			throw xml_invalid_syntax(is.tellg());
		tag = xml_tag(is);
		if (tag.name() != "model")
			throw improper_xml_tag(tag.name());
		this->convert_model(tag, is);
	}
	void next_hgt(std::istream& is)
	{
		m_pHgt = std::addressof(is); //do it in finalize directly to pOs
	}
	void finalize();


private:
	poly_data::face_data convert_face(const xml_tag& rTag, std::istream& is)
	{
		poly_data::face_data face;
		while (true)
		{
			auto it_is = find_if_not(std::istream_iterator<char>(is), std::istream_iterator<char>(), xml::isspace);
			if (it_is == std::istream_iterator<char>())
				throw xml_invalid_syntax(is.tellg());
			if (*it_is != '<')
				throw invalid_xml_data(is.tellg());
			auto tag = xml_tag(is);
			if (tag.name() == "domain")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto strDomain = tag.attribute("name");
				if (strDomain.empty())
					throw xml_attribute_not_found("name");
				auto buf = memstreambuf();
				std::ostream os_buf(&buf);
				m_pConv->face_domain_data(std::move(strDomain), is, os_buf);
				face.mapDomainData.emplace(std::move(strDomain), std::move(buf.get_vector()));
			}else if (tag.name() == "vertex")
			{
				if (!tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				point_t v;
				const auto& x = tag.attribute("x");
				if (x.empty())
					throw invalid_xml_data("Missing face vertex abscissa");
				v.x = std::stod(x);
				const auto& y = tag.attribute("y");
				if (y.empty())
					throw invalid_xml_data("Missing face vertex abscissa");
				v.y = std::stod(y);
				const auto& z = tag.attribute("z");
				if (z.empty())
					throw invalid_xml_data("Missing face vertex abscissa");
				v.z = std::stod(z);
				face.lstVertices.emplace_back(std::move(v));
			}else if (tag.name() == "face" && tag.is_closing_tag())
				break;
			else
				throw improper_xml_tag(is.tellg());
		};
		return face;
	}
	poly_data convert_poly(const xml_tag& rTag, std::istream& is)
	{
		poly_data poly;
		auto name = rTag.attribute("name");
		if (!rTag.empty())
		{
			if (!m_strModelName.empty())
				throw ambiguous_specification("Model attribute \"name\"");
			m_strModelName = std::move(strAttr);
		}
		while (true)
		{
			auto it_is = find_if_not(std::istream_iterator<char>(is), std::istream_iterator<char>(), xml::isspace);
			if (it_is == std::istream_iterator<char>())
				throw xml_invalid_syntax(is.tellg());
			if (*it_is != '<')
				throw invalid_xml_data(is.tellg());
			auto tag = xml_tag(is);
			if (tag.name() == "domain")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto strDomain = tag.attribute("name");
				if (strDomain.empty())
					throw xml_attribute_not_found("name");
				auto buf = memstreambuf();
				std::ostream os_buf(&buf);
				m_pConv->poly_domain_data(std::move(strDomain), is, os_buf);
				poly.mapDomainData.emplace(std::move(strDomain), std::move(buf.get_vector()));
			}else if (tag.name() == "face")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				poly.lstFaces.emplace_back(convert_face(tag, is));
			}else if (tag.name() == "poly" && tag.is_closing_tag())
				break;
			else
				throw improper_xml_tag(is.tellg());
		};
		return poly;
	}
	poly_data convert_source(const xml_tag& rTag, std::istream& is);
	poly_data convert_plain(const xml_tag& rTag, std::istream& is);
	void convert_model(const xml_tag& rModelTag, std::istream& is)
	{
		auto strAttr = rModelTag.attribute("cx");
		if (!strAttr.empty())
		{
			if (is_specified(m_size.x))
				throw ambiguous_specification("Model attribute \"cx\"");
			m_size.x = std::stod(strAttr);
		}
		auto strAttr = rModelTag.attribute("cy");
		if (!strAttr.empty())
		{
			if (is_specified(m_size.y))
				throw ambiguous_specification("Model attribute \"cy\"");
			m_size.y = std::stod(strAttr);
		}
		auto strAttr = rModelTag.attribute("cz");
		if (!strAttr.empty())
		{
			if (is_specified(m_size.z))
				throw ambiguous_specification("Model attribute \"cz\"");
			m_size.z = std::stod(strAttr);
		}
		auto strAttr = rModelTag.attribute("name");
		if (!strAttr.empty())
		{
			if (!m_strModelName.empty())
				throw ambiguous_specification("Model attribute \"name\"");
			m_strModelName = std::move(strAttr);
		}
		while (true)
		{
			auto it_is = find_if_not(std::istream_iterator<char>(is), std::istream_iterator<char>(), xml::isspace);
			if (it_is == std::istream_iterator<char>())
				throw xml_invalid_syntax(is.tellg());
			if (*it_is != '<')
				throw invalid_xml_data(is.tellg());
			auto tag = xml_tag(is);
			if (tag.name() == "domain")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto strDomain = tag.attribute("name");
				if (strDomain.empty())
					throw xml_attribute_not_found("name");
				auto buf = memstreambuf();
				std::ostream os_buf(&buf);
				m_pConv->model_domain_data(std::move(strDomain), is, os_buf);
				m_mapDomainData.emplace(std::move(strDomain), std::move(buf.get_vector()));
			}else if (tag.name() == "polyobject")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto poly = convert_poly(tag, is);
				auto name = poly.name;
				if (name.empty())
					m_polyUnnamedList.emplace_back(std::move(poly));
				else
					m_polyNamedMap.emplace(std::move(name), std::move(poly));
			}else if (tag.name() == "sourceobject")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto source = convert_source(tag, is);
				auto name = source.name;
				if (name.empty())
					m_srcUnnamedList.emplace_back(std::move(source));
				else
					m_srcNamedMap.emplace(std::move(name), std::move(source));
			}else if (tag.name() =="plainobject")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto plain = convert_plain(tag, is);
				auto name = plain.name;
				if (name.empty())
					m_plainUnnamedList.emplace_back(std::move(plain));
				else
					m_plainNamedMap.emplace(std::move(name), std::move(plain));
			}else if (tag.name() == "model" && tag.is_closing_tag())
				break;
			else
				throw improper_xml_tag(is.tellg());
		};
	}
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
