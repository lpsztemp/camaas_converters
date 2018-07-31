#include <algorithm>
#include <iterator>
#include <functional>
#include <list>
#include <filesystem>
#include <fstream>
#include <optional>
#include <basedefs.h>
#include <xml2bin.h>
#include <impl_radio_hf_domain_xml2bin.h>
#include <impl_arch_ac_domain_xml2bin.h>
#include <impl_xml_parser.h>
#include <binary_streams.h>
#include <domain_converter.h>
#include <hgt_optimizer.h>

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


constexpr double unspecified_double()
{
	return std::numeric_limits<double>::infinity();
}

constexpr bool is_specified(double val)
{
	return val != unspecified_double();
}

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
	binary_ostream* m_pOs = nullptr;
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
	std::unique_ptr<IDomainConverter> m_pConv;
public:
	conversion_state_impl(const std::string_view& domain, binary_ostream& os):m_pOs(std::addressof(os))
	{
		if (domain == radio_hf_convert::domain_name())
			m_pConv.reset(new ConverterImpl<radio_hf_convert>(radio_hf_convert()));
		else if (domain == arch_ac_convert::domain_name())
			m_pConv.reset(new ConverterImpl<arch_ac_convert>(arch_ac_convert()));
		else
			throw std::invalid_argument("Unknown domain name");
	}
	explicit conversion_state_impl(binary_ostream& os):m_pOs(std::addressof(os))
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
		auto model_size_pos = os.tellp();
		this->write(m_size);
		this->write(m_mapDomainData);
		auto object_count = std::uint32_t(this->poly_count() + this->source_count() + this->plain_count());
		auto object_count_pos = os.tellp();
		os << object_count;
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
		if (m_pHgt)
		{
			auto hgt_stats = convert_hgt(m_hgt_res, *m_pHgt, *m_pConv, os);
			auto old_pos = os.tellp();
			if (!is_specified(m_size))
			{
				m_size.x = m_hgt_res.cColumns * m_hgt_res.dx;
				m_size.y = m_hgt_res.cRows * m_hgt_res.dy;
				m_size.z = hgt_stats.max_height;
				os.seekp(model_size_pos);
				this->write(m_size);

			}
			os.seekp(object_count_pos);
			os << object_count + std::uint32_t(hgt_stats.poly_count);
		}
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
				buf_ostream os_buf;
				m_pConv->face_domain_data(std::move(strDomain), is, os_buf);
				face.mapDomainData.emplace(std::move(strDomain), std::move(os_buf.get_vector()));
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
				buf_ostream os_buf;
				m_pConv->poly_domain_data(std::move(strDomain), is, os_buf);
				if (!poly.mapDomainData.emplace(std::move(strDomain), std::move(os_buf.get_vector())).second)
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
				buf_ostream os_buf;
				m_pConv->source_domain_data(std::move(strDomain), is, os_buf);
				if (!source.mapDomainData.emplace(std::move(strDomain), std::move(os_buf.get_vector())).second)
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
				buf_ostream os_buf;
				m_pConv->plain_domain_data(std::move(strDomain), is, os_buf);
				if (!plain.mapDomainData.emplace(std::move(strDomain), std::move(os_buf.get_vector())).second)
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
				buf_ostream os_buf;
				m_pConv->model_domain_data(std::move(strDomain), is, os_buf);
				m_mapDomainData.emplace(std::move(strDomain), std::move(os_buf.get_vector()));
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
	auto write(binary_ostream& os, const T& val) const -> std::enable_if_t<std::is_pod_v<std::decay_t<T>>>
	{
		os << val;
	}
	void write(binary_ostream& os, const point_t& pt) const
	{
		os << std::uint32_t(3) // dimensions
			<< pt.x << pt.y << pt.z;
	}
	template <class Cont>
	auto write_sequence(binary_ostream& os, const Cont& cont) const -> std::enable_if_t<has_data_and_size<Cont>::value>
	{
		std::size_t cb = cont.size() * sizeof(typename std::decay<Cont>::type::value_type);
		os << std::uint32_t(cb);
		os.write(reinterpret_cast<const char*>(cont.data()), cb);
	}
	template <class Cont>
	auto write_sequence(binary_ostream& os, const Cont& cont) const -> std::enable_if_t<!has_data_and_size<Cont>::value && is_iteratable<Cont>::value>
	{
		using std::begin;
		using std::end;
		os << std::uint32_t(cont.size());
		for (const auto& elem:cont)
			this->write(os, elem);
	}
	void write(binary_ostream& os, const domain_data_map& mpDomainData) const
	{
		os << std::uint32_t(mpDomainData.size());
		for (const auto& prDomainData:mpDomainData)
		{
			this->write_sequence(os, prDomainData.first);
			this->write_sequence(os, prDomainData.second);
		}
	}
	void write_object_generic_part(binary_ostream& os, const std::string_view& strObjectName, const domain_data_map& mpDomainData, ObjectTypeId type_id) const
	{
		this->write_sequence(os, strObjectName);
		this->write(os, mpDomainData);
		os << type_id;
	}
	void write_object_generic_part(binary_ostream& os, const domain_data_map& mpDomainData, ObjectTypeId type_id) const
	{
		write_object_generic_part(os, std::string_view(), mpDomainData, type_id);
	}
	void write(binary_ostream& os, const poly_data::face_data& face) const
	{
		this->write_sequence(os, face.lstVertices);
		this->write(os, face.mapDomainData);
	}
	void write(binary_ostream& os, const poly_data& poly) const
	{
		this->write_object_generic_part(os, poly.name, poly.mapDomainData, ObjectPoly);
		this->write_sequence(os, poly.lstFaces);
	}
	void write(binary_ostream& os, const source_data& src) const
	{
		this->write_object_generic_part(os, src.name, src.mapDomainData, ObjectSource);
		this->write(os, src.pos);
		this->write(os, std::uint32_t(2));
		this->write(os, src.dir);
		this->write(os, src.top);
	}
	void write(binary_ostream& os, const plain_data& plain) const
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
};

namespace Implementation
{
	std::unique_ptr<conversion_state> xml2bin_set(const std::string& domain, binary_ostream& os)
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
