#include <basedefs.h>
#include "impl_xml_parser.h"

#ifndef IMPL_RADIO_HF_DOMAIN_XML2BIN_H_
#define IMPL_RADIO_HF_DOMAIN_XML2BIN_H_

struct radio_hf_convert
{
	//REQUIRED METHODS
	static const std::string& domain_name();

	//NON MANDATORY METHODS
	void model_domain_data(std::istream& is, std::ostream& os);
	void poly_domain_data(std::istream& is, std::ostream& os);
	void source_domain_data(std::istream& is, std::ostream& os);

	//hgt
	void constant_domain_data(ConstantDomainDataId id, std::ostream& os);
};

#endif //IMPL_RADIO_HF_DOMAIN_XML2BIN_H_
