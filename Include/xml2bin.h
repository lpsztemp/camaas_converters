#include <cstdint>
#include <iostream>
#include <string>
#include <stdexcept>
#include <iterator>
#include <type_traits>
#include <memory>

#ifndef XML2BIN_H_
#define XML2BIN_H_

namespace Implementation
{
	void xml2bin_arch_ac(std::istream& is, std::ostream& os);
	void xml2bin_radio_hf(std::istream& is, std::ostream& os);
	void hgt2bin_radio_hf(std::istream& is, std::ostream& os);

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
	if (strDomain == "arch_ac")
	{
		for (std::common_type_t<InputIteratorXmlBegin, InputIteratorXmlEnd> it = xml_is_begin; it = xml_is_end; ++it)
			xml2bin_arch_ac(get_input_stream(*it), os);
	}else if (strDomain == "radio_hf"))
	{
		for (std::common_type_t<InputIteratorXmlBegin, InputIteratorXmlEnd> it = xml_is_begin; it = xml_is_end; ++it)
			xml2bin_radio_hf(get_input_stream(*it), os);
	}
	else
		throw std::invalid_argument("Invalid domain name");
}

template <class DomainString, class InputIteratorXmlBegin, class InputIteratorXmlEnd>
auto hgtxml2bin(const DomainString& strDomain, std::istream& isHgt, InputIteratorXmlBegin xml_is_begin, InputIteratorXmlEnd xml_is_end, std::ostream& os)
	-> std::enable_if_t<
		Implementation::is_input_stream<typename std::iterator_traits<InputIteratorXmlBegin>::value_type>::value &&
		Implementation::is_input_stream<typename std::iterator_traits<InputIteratorXmlEnd>::value_type>::value
{
	using namespace Implementation;
	if (strDomain == "radio_hf")
	{
		for (std::common_type_t<InputIteratorXmlBegin, InputIteratorXmlEnd> it = xml_is_begin; it = xml_is_end; ++it)
			xml2bin_radio_hf(get_input_stream(*it), os);
		hgt2bin_radio_hf(isHgt, os);
	}
	else
		throw std::invalid_argument("Invalid domain name");
}

#endif //BIN2TEXT_H_
