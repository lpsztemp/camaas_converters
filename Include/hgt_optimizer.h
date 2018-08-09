#include <binary_streams.h>
#include <xml2bin.h>
#include <domain_converter.h>

#ifndef XML2BIN_HGTOPTIMIZER_H_
#define XML2BIN_HGTOPTIMIZER_H_

//returns a number of faces obtained
unsigned convert_hgt_to_index_based_face(binary_ostream& os, const short* pInput, unsigned short cColumns, unsigned short cRows, double eColumnResolution, double eRowResolution);

#if CPP17_FILESYSTEM_SUPPORT
unsigned convert_hgt_to_index_based_face(std::filesystem::path input, std::filesystem::path output);
#endif //CPP17_FILESYSTEM_SUPPORT

struct HGT_CONVERSION_STATS
{
	short min_height;
	short max_height;
	unsigned poly_count;
};

//returns min and max heights
HGT_CONVERSION_STATS convert_hgt(const HGT_RESOLUTION_DATA& resolution, std::istream& is_data, const IDomainConverter& converter, binary_ostream& os);

#endif //XML2BIN_HGTOPTIMIZER_H_
