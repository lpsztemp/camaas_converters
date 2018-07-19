#include <cstdint>
#include <iostream>
#include <string>
#include <stdexcept>
#include <iterator>
#include <type_traits>
#include <memory>
#include <fstream>
#include <stack>

#ifndef XML2BIN_H_
#define XML2BIN_H_

struct HGT_RESOLUTION_DATA
{
	double dx; //distance between points in rows
	double dy; //distance between points in columns
	std::size_t cColumns;
	std::size_t cRows;
};

static const HGT_RESOLUTION_DATA HGT_1 = {30, 30, 3601, 3601};
static const HGT_RESOLUTION_DATA HGT_3 = {90, 90, 1201, 1201};

namespace Implementation
{
	struct conversion_state {virtual inline ~conversion_state() {}};
	std::unique_ptr<conversion_state> xml2bin_set(const std::string& domain, std::ostream& os);
	void xml2bin_next_xml(const std::unique_ptr<conversion_state>& state, std::istream& is);
	void xml2bin_next_hgt(const std::unique_ptr<conversion_state>& state, const HGT_RESOLUTION_DATA& resolution, std::istream& is);
	void xml2bin_finalize(const std::unique_ptr<conversion_state>& state);

	template <class T>
	T& get_input_stream(const std::reference_wrapper<T>& ref)
	{
		return ref.get();
	}
	template <class T>
	auto get_input_stream(const T& ref) -> typename std::pointer_traits<T>::element_type&
	{
		return std::pointer_traits<T>::pointer_to(ref);
	}
	template <class T>
	struct is_input_stream:std::is_base_of<std::istream, decltype(get_input_stream(std::declval<T>()))> {};
}

template <class DomainString, class InputIteratorXmlBegin, class InputIteratorXmlEnd>
auto xml2bin(const DomainString& strDomain, InputIteratorXmlBegin xml_is_begin, InputIteratorXmlEnd xml_is_end, std::ostream& os)
	-> std::enable_if_t<
		Implementation::is_input_stream<typename std::iterator_traits<InputIteratorXmlBegin>::value_type>::value &&
		Implementation::is_input_stream<typename std::iterator_traits<InputIteratorXmlEnd>::value_type>::value
	>
{
	using namespace Implementation;
	auto state = xml2bin_set(strDomain, os);
	for (std::common_type_t<InputIteratorXmlBegin, InputIteratorXmlEnd> it = xml_is_begin; it = xml_is_end; ++it)
		xml2bin_next_xml(state, get_input_stream(*it));
	xml2bin_finalize(state);
}

template <class DomainString, class InputIteratorXmlBegin, class InputIteratorXmlEnd>
auto hgtxml2bin(const DomainString& strDomain, const HGT_RESOLUTION_DATA& resolution, std::istream& isHgt, InputIteratorXmlBegin xml_is_begin, InputIteratorXmlEnd xml_is_end, std::ostream& os)
	-> std::enable_if_t<
		Implementation::is_input_stream<typename std::iterator_traits<InputIteratorXmlBegin>::value_type>::value &&
		Implementation::is_input_stream<typename std::iterator_traits<InputIteratorXmlEnd>::value_type>::value
	>
{
	using namespace Implementation;
	auto state = xml2bin_set(strDomain, os);
	for (std::common_type_t<InputIteratorXmlBegin, InputIteratorXmlEnd> it = xml_is_begin; it = xml_is_end; ++it)
		xml2bin_next_xml(state, get_input_stream(*it));
	xml2bin_next_hgt(state, resolution, isHgt);
	xml2bin_finalize(state);
}

#endif //BIN2TEXT_H_
