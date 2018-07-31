#include "impl_xml_parser.h"

#ifndef IMPL_ARCH_AC_DOMAIN_XML2BIN_H_
#define IMPL_ARCH_AC_DOMAIN_XML2BIN_H_

struct arch_ac_convert
{
	//REQUIRED METHODS
	static const std::string& domain_name();

	//NON MANDATORY METHODS
	void model_domain_data(std::istream& is, binary_ostream& os);
	void face_domain_data(std::istream& is, binary_ostream& os);
	void source_domain_data(std::istream& is, binary_ostream& os);
};

#endif //IMPL_ARCH_AC_DOMAIN_XML2BIN_H_
