#pragma warning (disable: 4996)
#include <basedefs.h>
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
#include <optional>

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

struct input_not_ready:std::invalid_argument
{
	input_not_ready():std::invalid_argument("Input model is not fully specified") {}
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
	static const std::string& domain_name();

	void model_domain_data(is, os);
	void poly_domain_data(is, os);
	void face_domain_data(is, os);
	void source_domain_data(is, os);
	void plain_domain_data(is, os);
	void constant_domain_data(SurfaceTypeId id, os);
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
template <class T, class = void> struct has_constant_domain_data:std::false_type {};
template <class T> struct has_constant_domain_data<T, std::void_t<decltype(std::declval<T>().constant_domain_data(std::declval<ConstantDomainDataId>(), std::declval<std::ostream&>()))>>:std::true_type {};

template <class Convert>
auto convert_model_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_model_domain_data<Convert&>::value>
{
	return conv.model_domain_data(is, os);
}
template <class Convert>
auto convert_model_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_model_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}
template <class Convert>
auto convert_poly_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_poly_domain_data<Convert&>::value>
{
	return conv.poly_domain_data(is, os);
}
template <class Convert>
auto convert_poly_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_poly_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}

template <class Convert>
auto convert_face_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_face_domain_data<Convert&>::value>
{
	return conv.face_domain_data(is, os);
}
template <class Convert>
auto convert_face_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_face_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}

template <class Convert>
auto convert_source_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_source_domain_data<Convert&>::value>
{
	return conv.source_domain_data(is, os);
}
template <class Convert>
auto convert_source_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_source_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}

template <class Convert>
auto convert_plain_domain_data(Convert& conv, std::istream& is, std::ostream& os) -> std::enable_if_t<has_plain_domain_data<Convert&>::value>
{
	return conv.plain_domain_data(is, os);
}
template <class Convert>
auto convert_plain_domain_data(Convert&, std::istream& is, std::ostream&) -> std::enable_if_t<!has_plain_domain_data<Convert&>::value>
{
	skip_domain_data(is);
}

template <class Convert>
auto write_constant_domain_data(Convert& conv, ConstantDomainDataId data_id, std::ostream& os) -> std::enable_if_t<has_constant_domain_data<Convert&>::value, bool>
{
	conv.constant_domain_data(data_id, os);
	return true;
}
template <class Convert>
auto write_constant_domain_data(Convert&, ConstantDomainDataId, std::ostream&) -> std::enable_if_t<!has_constant_domain_data<Convert&>::value, bool>
{
	return false;
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

template <class T, class = void>
struct has_data_and_size:std::false_type {};
template <class T>
struct has_data_and_size<T, std::void_t<decltype(std::declval<T>().data()), decltype(std::declval<T>().size())>>:std::true_type {};

template <class T, class = void>
struct is_iteratable:std::false_type {};
template <class T>
struct is_iteratable<T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>:std::true_type {};

class conversion_state_impl:public Implementation::conversion_state
{
	std::ostream* m_pOs = nullptr;
	std::istream* m_pHgt = nullptr;
	HGT_RESOLUTION_DATA m_hgt_res = {double(), double(), std::size_t(), std::size_t()};
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
		virtual std::optional<domain_data_map::value_type> constant_domain_data(const std::string_view&, ConstantDomainDataId id) = 0;
		virtual domain_data_map constant_domain_data(ConstantDomainDataId id) = 0;

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
		virtual std::optional<domain_data_map::value_type> constant_domain_data(const std::string_view&, ConstantDomainDataId id)
		{
			auto buf = memstreambuf{};
			std::ostream os{std::addressof(buf)};
			if (!write_constant_domain_data(m_conv, id, os))
				return std::optional<domain_data_map::value_type>();
			return std::optional<domain_data_map::value_type>(std::in_place_t(), Converter::name(), buf.get_vector());
		}
		virtual domain_data_map constant_domain_data(ConstantDomainDataId id)
		{
			return domain_data_map{this->constant_domain_data(Converter::name(), id)};
		}

		template <class ... ConverterParams, class = std::enable_if_t<std::is_constructible_v<Converter, ConverterParams&& ... >>>
		ConverterImpl(ConverterParams&& ... conv):m_conv(std::forward<ConverterParams>(conv) ... ) {}
	private:
		Converter m_conv;
	};

	struct generalized_converter:IConverter
	{
		template <class NameConverterTuple>
		generalized_converter(NameConverterTuple&& name_conv_tpl):m_mpConv(create_map(std::forward<NameConverterTuple>(name_conv_tpl)))
		{}
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
		virtual std::optional<domain_data_map::value_type> constant_domain_data(const std::string_view& DomainName, ConstantDomainDataId id)
		{
			auto itConv = m_mpConv.find(DomainName);
			if (itConv != m_mpConv.end())
			{
				auto buf = memstreambuf();
				std::ostream os{&buf};
				if (!write_constant_domain_data(itConv->second, id, os))
					return std::optional<domain_data_map::value_type>();
				return std::optional<domain_data_map::value_type>{std::in_place_t(), DomainName, buf.get_vector()};
			}
		}
		virtual domain_data_map constant_domain_data(ConstantDomainDataId id)
		{
			domain_data_map mpRet;
			for (auto& conv:m_mpConv)
			{
				auto buf = memstreambuf();
				std::ostream os{&buf};
				if (write_constant_domain_data(conv.second, id, os))
					mpRet.emplace(conv.first, buf.get_vector());
			}
			return mpRet;
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
		typedef std::map<std::string, std::unique_ptr<IConverter>, less_transparent> map_type;
		map_type m_mpConv;
		template <std::size_t I = 0, class ... Init>
		static auto create_map(const std::tuple<Init...>& tpl) -> std::enable_if_t<(I + 1 < sizeof ... (Init)), map_type>
		{
			map_type mp = create_map<I + 2>(tpl);
			mp.emplace(std::get<I>(tpl), std::unique_ptr<IConverter>(new ConverterImpl<std::tuple_element_t<I + 1, std::tuple<Init...>>>(std::get<I + 1>(tpl))));
			return mp;
		}
		template <std::size_t I = 0, class ... Init>
		static auto create_map(std::tuple<Init...>&& tpl) -> std::enable_if_t<(I + 1 < sizeof ... (Init)), map_type>
		{
			map_type mp = create_map<I + 2>(std::move(tpl));
			mp.emplace(std::get<I>(tpl), std::unique_ptr<IConverter>(new ConverterImpl<std::tuple_element_t<I + 1, std::tuple<Init...>>>(std::get<I + 1>(std::move(tpl)))));
			return mp;
		}
		template <std::size_t I = 0>
		static auto create_map(...)
		{
			return map_type();
		}
	};
	std::unique_ptr<IConverter> m_pConv;

	struct hgt_converter
	{
	};
	
public:
	conversion_state_impl(const std::string_view& domain, std::ostream& os):m_pOs(std::addressof(os))
	{
		if (domain == radio_hf_convert::domain_name())
			m_pConv.reset(new ConverterImpl<radio_hf_convert>(radio_hf_convert()));
		else if (domain == arch_ac_convert::domain_name())
			m_pConv.reset(new ConverterImpl<arch_ac_convert>(arch_ac_convert()));
		else
			throw std::invalid_argument("Unknown domain name");
	}
	explicit conversion_state_impl(std::ostream& os):m_pOs(std::addressof(os))
	{
		m_pConv.reset(new generalized_converter(std::make_tuple(
			radio_hf_convert::domain_name(),	radio_hf_convert(),
			arch_ac_convert::domain_name(),		arch_ac_convert()
		)));
	}
	conversion_state_impl(const conversion_state_impl&) = delete;
	conversion_state_impl& operator=(const conversion_state_impl&) = delete;

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
		it_is = find_if_not(istream_iterator<char>(is), istream_iterator<char>(), xml::isspace);
		if (it_is == istream_iterator<char>() || is.get() != '<')
			throw xml_invalid_syntax(is.tellg());
		tag = xml_tag(is);
		if (tag.name() != "model")
			throw improper_xml_tag(tag.name());
		this->convert_model(tag, is);
	}
	void next_hgt(const HGT_RESOLUTION_DATA& resolution, std::istream& is)
	{
		m_pHgt = std::addressof(is); //do it in finalize directly to pOs
		m_hgt_res = resolution;
	}
	void finalize()
	{
		if (!this->is_model_ready())
			throw input_not_ready();
		auto& os = *m_pOs;
		this->write_sequence(m_strModelName);
		this->write(CHU_METERS);
		this->write(m_size);
		this->write(m_mapDomainData);
		if (m_pHgt) {}

		os << std::uint32_t(this->poly_count() + this->source_count() + this->plain_count());
		for (const auto& poly:m_polyNamedMap)
			this->write(poly.second);
		for (const auto& poly:m_polyUnnamedList)
			this->write(poly);
		for (const auto& src:m_srcNamedMap)
			this->write(src.second);
		for (const auto& src:m_srcUnnamedList)
			this->write(src);
		for (const auto& plain:m_plainNamedMap)
			this->write(plain.second);
		for (const auto& plain:m_plainUnnamedList)
			this->write(plain);
	}


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
					throw invalid_xml_data("Missing face vertex ordinate");
				v.y = std::stod(y);
				const auto& z = tag.attribute("z");
				if (z.empty())
					throw invalid_xml_data("Missing face vertex applicate");
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
		poly.name = rTag.attribute("name");
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
				if (!poly.mapDomainData.emplace(std::move(strDomain), std::move(buf.get_vector())).second)
					throw ambiguous_specification(std::string("Domain \"") + strDomain + std::string("\" of polyobject")
						+ (poly.name.empty()?std::string():(std::string(" \"") + poly.name + std::string("\""))));
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
	source_data convert_source(const xml_tag& rTag, std::istream& is)
	{
		source_data source;
		source.name = rTag.attribute("name");
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
				m_pConv->source_domain_data(std::move(strDomain), is, os_buf);
				if (!source.mapDomainData.emplace(std::move(strDomain), std::move(buf.get_vector())).second)
					throw ambiguous_specification(std::string("Domain \"") + strDomain + std::string("\" of sourceobject")
						+ (source.name.empty()?std::string():(std::string(" \"") + source.name + std::string("\""))));
			}else if (tag.name() == "position")
			{
				if (!tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto coord = tag.attribute("x");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.pos.x = std::stod(coord);
				coord = tag.attribute("y");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.pos.y = std::stod(coord);
				coord = tag.attribute("z");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.pos.z = std::stod(coord);
			}else if (tag.name() == "direction")
			{
				if (!tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto coord = tag.attribute("x");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.dir.x = std::stod(coord);
				coord = tag.attribute("y");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.dir.y = std::stod(coord);
				coord = tag.attribute("z");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.dir.z = std::stod(coord);
			}else if (tag.name() == "top")
			{
				if (!tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto coord = tag.attribute("x");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.top.x = std::stod(coord);
				coord = tag.attribute("y");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.top.y = std::stod(coord);
				coord = tag.attribute("z");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				source.top.z = std::stod(coord);
			}else if (tag.name() == "sourceobject" && tag.is_closing_tag())
				break;
			else
				throw improper_xml_tag(is.tellg());
		};
		return source;
	}
	plain_data convert_plain(const xml_tag& rTag, std::istream& is)
	{
		plain_data plain;
		plain.name = rTag.attribute("name");
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
				m_pConv->plain_domain_data(std::move(strDomain), is, os_buf);
				if (!plain.mapDomainData.emplace(std::move(strDomain), std::move(buf.get_vector())).second)
					throw ambiguous_specification(std::string("Domain \"") + strDomain + std::string("\" of sourceobject")
						+ (plain.name.empty()?std::string():(std::string(" \"") + plain.name + std::string("\""))));
			}else if (tag.name() == "position")
			{
				if (!tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto coord = tag.attribute("x");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.pos.x = std::stod(coord);
				coord = tag.attribute("y");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.pos.y = std::stod(coord);
				coord = tag.attribute("z");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.pos.z = std::stod(coord);
			}else if (tag.name() == "v1")
			{
				if (!tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto coord = tag.attribute("x");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.v1.x = std::stod(coord);
				coord = tag.attribute("y");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.v1.y = std::stod(coord);
				coord = tag.attribute("z");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.v1.z = std::stod(coord);
			}else if (tag.name() == "v2")
			{
				if (!tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto coord = tag.attribute("x");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.v2.x = std::stod(coord);
				coord = tag.attribute("y");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.v2.y = std::stod(coord);
				coord = tag.attribute("z");
				if (coord.empty())
					throw invalid_xml_data(is.tellg());
				plain.v2.z = std::stod(coord);
			}else if (tag.name() == "plainobject" && tag.is_closing_tag())
				break;
			else
				throw improper_xml_tag(is.tellg());
		};
		return plain;
	}
	void convert_model(const xml_tag& rModelTag, std::istream& is)
	{
		auto strAttr = rModelTag.attribute("cx");
		if (!strAttr.empty())
		{
			if (is_specified(m_size.x))
				throw ambiguous_specification("Model attribute \"cx\"");
			m_size.x = std::stod(strAttr);
		}
		strAttr = rModelTag.attribute("cy");
		if (!strAttr.empty())
		{
			if (is_specified(m_size.y))
				throw ambiguous_specification("Model attribute \"cy\"");
			m_size.y = std::stod(strAttr);
		}
		strAttr = rModelTag.attribute("cz");
		if (!strAttr.empty())
		{
			if (is_specified(m_size.z))
				throw ambiguous_specification("Model attribute \"cz\"");
			m_size.z = std::stod(strAttr);
		}
		strAttr = rModelTag.attribute("name");
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
				if (poly.name.empty())
					m_polyUnnamedList.emplace_back(std::move(poly));
				else
				{
					auto name = poly.name;
					if (!m_polyNamedMap.emplace(std::move(name), std::move(poly)).second)
						throw ambiguous_specification(std::string("polyobject ") + name);
				}
			}else if (tag.name() == "sourceobject")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto source = convert_source(tag, is);
				if (source.name.empty())
					m_srcUnnamedList.emplace_back(std::move(source));
				else
				{
					auto name = source.name;
					if (!m_srcNamedMap.emplace(std::move(name), std::move(source)).second)
						throw ambiguous_specification(std::string("sourceobject ") + name);
				}
			}else if (tag.name() =="plainobject")
			{
				if (tag.is_closing_tag() || tag.is_unary_tag())
					throw invalid_xml_data(is.tellg());
				auto plain = convert_plain(tag, is);
				if (plain.name.empty())
					m_plainUnnamedList.emplace_back(std::move(plain));
				else
				{
					auto name = plain.name;
					if (!m_plainNamedMap.emplace(std::move(name), std::move(plain)).second)
						throw ambiguous_specification(std::string("plainobject ") + name);
				}
			}else if (tag.name() == "model" && tag.is_closing_tag())
				break;
			else
				throw improper_xml_tag(is.tellg());
		};
	}
	


	
	template <class T>
	auto write(std::ostream& os, const T& val) const -> std::enable_if_t<std::is_pod_v<std::decay_t<T>>>
	{
		os << val;
	}
	void write(std::ostream& os, const point_t& pt) const
	{
		os << std::uint32_t(3) // dimensions
			<< pt.x << pt.y << pt.z;
	}
	template <class Cont>
	auto write_sequence(std::ostream& os, const Cont& cont) const -> std::enable_if_t<has_data_and_size<Cont>::value>
	{
		std::size_t cb = cont.size() * sizeof(typename std::decay<Cont>::type::value_type);
		os << std::uint32_t(cb);
		os.write(reinterpret_cast<const char*>(cont.data()), cb);
	}
	template <class Cont>
	auto write_sequence(std::ostream& os, const Cont& cont) const -> std::enable_if_t<!has_data_and_size<Cont>::value && is_iteratable<Cont>::value>
	{
		using std::begin;
		using std::end;
		os << std::uint32_t(cont.size());
		for (const auto& elem:cont)
			this->write(os, elem);
	}
	void write(std::ostream& os, const domain_data_map& mpDomainData) const
	{
		os << std::uint32_t(mpDomainData.size());
		for (const auto& prDomainData:mpDomainData)
		{
			this->write_sequence(os, prDomainData.first);
			this->write_sequence(os, prDomainData.second);
		}
	}
	void write_object_generic_part(std::ostream& os, const std::string_view& strObjectName, const domain_data_map& mpDomainData, ObjectTypeId type_id) const
	{
		this->write_sequence(os, strObjectName);
		this->write(os, mpDomainData);
		os << type_id;
	}
	void write_object_generic_part(std::ostream& os, const domain_data_map& mpDomainData, ObjectTypeId type_id) const
	{
		write_object_generic_part(os, std::string_view(), mpDomainData, type_id);
	}
	void write(std::ostream& os, const poly_data::face_data& face) const
	{
		this->write_sequence(os, face.lstVertices);
		this->write(os, face.mapDomainData);
	}
	void write(std::ostream& os, const poly_data& poly) const
	{
		this->write_object_generic_part(os, poly.name, poly.mapDomainData, ObjectPoly);
		this->write_sequence(os, poly.lstFaces);
	}
	void write(std::ostream& os, const source_data& src) const
	{
		this->write_object_generic_part(os, src.name, src.mapDomainData, ObjectSource);
		this->write(os, src.pos);
		this->write(os, std::uint32_t(2));
		this->write(os, src.dir);
		this->write(os, src.top);
	}
	void write(std::ostream& os, const plain_data& plain) const
	{
		this->write_object_generic_part(os, plain.name, plain.mapDomainData, ObjectSource);
		this->write(os, plain.pos);
		this->write(os, std::uint32_t(2));
		this->write(os, plain.v1);
		this->write(os, plain.v2);
	}
	template <class T>
	void write(const T& elem)
	{
		this->write(*m_pOs, elem);
	}
	template <class T>
	void write_sequence(const T& container)
	{
		this->write_sequence(*m_pOs, container);
	}
	bool is_model_ready() const
	{
		return is_specified(m_size);
	}
	std::size_t poly_count() const
	{
		auto cPoly = m_polyNamedMap.size() + m_polyUnnamedList.size();
		if (m_pHgt != nullptr)
			++cPoly;
		return cPoly;
	}
	std::size_t source_count() const
	{
		return m_srcNamedMap.size() + m_srcUnnamedList.size();
	};
	std::size_t plain_count() const
	{
		return m_plainNamedMap.size() + m_plainUnnamedList.size();
	};

	template <std::size_t vertices_per_thread>
	static constexpr auto hgt_face_count(std::size_t cols, std::size_t rows) -> std::enable_if_t<vertices_per_thread == 3, std::size_t>
	{
		return (cols - 1) * (rows - 1) * 2;
	}
	template <std::size_t vertices_per_thread>
	static constexpr auto hgt_face_count(std::size_t cols, std::size_t rows) -> std::enable_if_t<vertices_per_thread == 4, std::size_t>
	{
		return (cols - 1) * (rows - 1);
	}
	template <std::size_t vertices_per_thread>
	static constexpr auto hgt_output_bytes(std::size_t cols, std::size_t rows) -> std::size_t
	{
		constexpr std::size_t face_cb =
			sizeof(std::uint32_t) /*number of vertices*/
			+ vertices_per_thread *
				(
					sizeof(std::uint32_t) /*number of dimensions*/
					+ 3 /*dimensions*/ * sizeof(double)
				);
		return hgt_face_count<vertices_per_thread>(cols, rows) * face_cb;
	}

	static constexpr std::size_t HGT_COLUMNS_PER_THREAD = 100;
	static constexpr std::size_t HGT_ROWS_PER_THREAD = 100;

	struct byte_buf
	{
		byte_buf() = default;
		explicit byte_buf(std::size_t cb):m_ptr(std::unique_ptr<std::uint8_t[]>(new std::uint8_t[cb])), m_cb(cb)
		{
		}
		std::uint8_t* data() const
		{
			return m_ptr.get();
		}
		std::size_t size() const
		{
			return m_cb;
		}
	private:
		std::unique_ptr<std::uint8_t[]> m_ptr;
		std::size_t m_cb = 0;
	};

	//first: land, second: water
	static std::pair<byte_buf, byte_buf> triangulate(const HGT_RESOLUTION_DATA& resolution, const void* pRead, std::size_t cbRead);
	template <std::size_t vertices_per_thread = 3>
	void convert_hgt(const HGT_RESOLUTION_DATA& resolution, std::istream& is, std::ostream& os)
	{
		std::ostream::pos_type land_faces, water_faces;

		auto water_data = m_pConv->constant_domain_data(ConstantDomainDataId::SurfaceWater);
		auto land_data = m_pConv->constant_domain_data(ConstantDomainDataId::SurfaceLand);


		auto cbInput = resolution.cColumns * resolution.cRows * sizeof(std::uint16_t);
		auto cbOutput = hgt_output_bytes<vertices_per_thread>(resolution.cColumns, resolution.cRows);
		auto cbRead = resolution.cColumns * HGT_ROWS_PER_THREAD * sizeof(std::uint16_t);
		auto cbWrite = hgt_output_bytes<vertices_per_thread>(resolution.cColumns, HGT_ROWS_PER_THREAD);
		std::ostream::pos_type land_faces = os.tellp(), water_faces = os.tellp() + cbWrite;
		std::size_t cLandFacesWritten = 0, cWaterFacesWritten = 0;
	}
};

namespace Implementation
{
	std::unique_ptr<conversion_state> xml2bin_set(const std::string& domain, std::ostream& os)
	{
		return std::unique_ptr<conversion_state>(new conversion_state_impl(domain, os));
	}
	void xml2bin_next_xml(const std::unique_ptr<conversion_state>& state, std::istream& is)
	{
		static_cast<conversion_state_impl*>(state.get())->next_xml(is);
	}
	void xml2bin_next_hgt(const std::unique_ptr<conversion_state>& state, const HGT_RESOLUTION_DATA& resolution, std::istream& is)
	{
		static_cast<conversion_state_impl*>(state.get())->next_hgt(resolution, is);
	}
	void xml2bin_finalize(const std::unique_ptr<conversion_state>& state)
	{
		static_cast<conversion_state_impl*>(state.get())->finalize();
	}
} //Implementation
