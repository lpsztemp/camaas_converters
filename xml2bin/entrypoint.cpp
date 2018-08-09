#include "xml2bin.h"
#include <list>
#include <string>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <sstream>

struct invalid_usage:std::runtime_error
{
	invalid_usage():std::runtime_error("Invalid program usage") {}
};

struct failed_to_open_a_file:std::runtime_error 
{
	failed_to_open_a_file(const std::string& file):std::runtime_error(form_what(file)) {}
private:
	static std::string form_what(const std::string& file)
	{
		std::ostringstream os;
		os << "Failed to open the file \"" << file << "\".";
		return os.str();
	}
};

struct unexpected_hgt_size:std::runtime_error
{
	unexpected_hgt_size():std::runtime_error("Unexpected HGT size. Only SRTM 30m and SRTM 90m are supported.") {}
};

class Program
{
public:
	Program(int argc, char** argv)
	{
		int i;
		for (i = 0; i < argc; ++i)
		{
			if (std::string_view(argv[i]) == "--help")
			{
				if (argc > 2)
					throw invalid_usage();
				std::cout << m_help_str;
				return;
			}else if (std::string_view(argv[i]) == "--hgt")
			{
				if (i == argc - 1 || !m_hgt.empty())
					throw invalid_usage();
				m_hgt = argv[i + 1];
			}else if (std::string_view(argv[i]) == "--domain")
			{
				if (i == argc - 1 || !m_domain.empty())
					throw invalid_usage();
				m_domain = argv[i + 1];
			}else if (std::string_view(argv[i]) == "--discard_output")
			{
				if (m_fDiscardOutput)
					throw invalid_usage();
				m_fDiscardOutput = true;
			}else if (argv[i][0] == '-' && argv[i][1] == '-')
				throw invalid_usage();
			else
				break;
		}
		if (i == argc)
			throw invalid_usage();
		do
		{
			this->add_file(argv[i]);
		}while (++i < argc);
		if (!this->is_ready())
			throw invalid_usage();
	}
	bool is_ready() const
	{
		return !m_lstXml.empty() && !m_output.empty() && !m_domain.empty();
	}
	Program& run()
	{
		std::list<text_file_istream> m_lst_xml_is;

		if (!is_ready())
			throw invalid_usage();
		for (const auto& strXml:m_lstXml)
		{
			if (m_lst_xml_is.emplace_back(text_file_istream(strXml)).fail())
				throw failed_to_open_a_file(strXml);
		}
		auto os = binary_ofstream(std::string_view(m_output), m_fDiscardOutput);
		if (m_hgt.empty())
		{
			xml2bin(m_domain, std::begin(m_lst_xml_is), std::end(m_lst_xml_is), os);
			return *this;
		}
		auto is_hgt = std::ifstream(m_hgt, std::ios_base::in | std::ios_base::binary);
		if (is_hgt.fail())
			throw failed_to_open_a_file(m_hgt);
		is_hgt.seekg(0, std::ios_base::end);
		auto cb = std::size_t(std::streamoff(is_hgt.tellg()));
		is_hgt.seekg(0, std::ios_base::beg);
		HGT_RESOLUTION_DATA hgt_res;
		switch (cb)
		{
		case HGT_3.cColumns * HGT_3.cRows * sizeof(std::int16_t):
			hgt_res = HGT_3;
			break;
		case HGT_1.cColumns * HGT_1.cRows * sizeof(std::int16_t):
			hgt_res = HGT_1;
			break;
		default:
			throw unexpected_hgt_size();
		}
		hgtxml2bin(m_domain, hgt_res, is_hgt, std::begin(m_lst_xml_is), std::end(m_lst_xml_is), os);
		return *this;
	}
private:
	std::list<std::string> m_lstXml;
	std::string m_hgt;
	bool m_fDiscardOutput = false;
	std::string m_output;
	std::string m_domain;
	static std::string m_help_str;

	Program& add_file(const char* file)
	{
		if (!m_output.empty())
			m_lstXml.emplace_back(std::move(m_output));
		m_output = file;
		return *this;
	}
};

std::string Program::m_help_str =
"xml2bin <[--domain <domain_name>] [--hgt <path_to_hgt>] [--discard_output] <input_xml_file_1> [... input_xml_file_n] <output_binary_file>>|<--help>\n"\
" --domain specifies a domain system id for which the program should perform the conversion. If an XML file contains definitions of\n"\
"       domain data with domain id different from the id specified by the --domain parameter, that domain data will be discarded."\
" --hgt specifies a path to a HGT file to be converted to a set of polygonal surfaces to specify, together with the XML files,\n"\
"       the output binary model. Only SRTM 30m and SRTM 90m are supported. The parameter is optional."\
" <input_xml_file_1> [... input_xml_file_n] is a set of one or more input XML files. Each file must specify unique objects (sources"\
"       and/or control panes and/or polygonal reflectors) to be unified to the resulting binary definition of the model, perhaps"\
"       with the specified HGT. All such objects, if they are named, must be specified in the set of input files not more than once.\n"\
"       The generic model parameters (size, domain data, etc.) must be specified exactly once. Should an XML file omit the generic\n"
"       model parameters, the set of objects specified by the file, must be bounded by <model></model> XML tags without unnecessary\n"
"       attributes or nested definitions.\n"
" --discard_output is a switch which allows to truncate the output file before the processing, should the file exist. Otherwise, if\n"
"       the output file exists and not empty, the program will fail.\n"\
" output_binary_file specifies a path to the output binary file.\n"\
" --help displays this message.\n";

int main(int argc, char** argv)
{
	try
	{
		Program pr(argc, argv);
		if (pr.is_ready())
			pr.run();
	}catch (std::exception& ex)
	{
		std::cerr << ex.what() << "\n";
		return -1;
	}
	return 0;
}