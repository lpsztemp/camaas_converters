#include "impl_xml_parser.h"

#ifndef IMPL_RADIO_HF_DOMAIN_XML2BIN_H_
#define IMPL_RADIO_HF_DOMAIN_XML2BIN_H_

struct radio_hf_convert
{
	void model_domain_data(std::istream& is, std::ostream& os);
	void poly_domain_data(std::istream& is, std::ostream& os);
	void source_domain_data(std::istream& is, std::ostream& os);
};

#endif //IMPL_RADIO_HF_DOMAIN_XML2BIN_H_
