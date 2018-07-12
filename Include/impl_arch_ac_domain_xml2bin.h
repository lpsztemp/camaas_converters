#include "impl_xml_parser.h"

#ifndef IMPL_ARCH_AC_DOMAIN_XML2BIN_H_
#define IMPL_ARCH_AC_DOMAIN_XML2BIN_H_

struct arch_ac_convert
{
	void model_domain_data(std::istream& is, std::ostream& os);
	void face_domain_data(std::istream& is, std::ostream& os);
	void source_domain_data(std::istream& is, std::ostream& os);
};

#endif //IMPL_ARCH_AC_DOMAIN_XML2BIN_H_
