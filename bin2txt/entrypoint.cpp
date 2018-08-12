#include "bin2text.h"
#include <list>
#include <string>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <sstream>

#include <locale>
#include <codecvt>

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

class Program
{
public:
	Program(int argc, char** argv)
	{
		int i;
		for (i = 1; i < argc; ++i)
		{
			if (std::string_view(argv[i]) == "--help")
			{
				if (argc > 2)
					throw invalid_usage();
				std::cout << m_help_str;
				return;
			}else if (std::string_view(argv[i]) == "--domain")
			{
				if (i == argc - 1 || !m_domain.empty())
					throw invalid_usage();
				m_domain = argv[++i];
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
		return !m_input.empty() && !m_output.empty() && !m_domain.empty();
	}
	Program& run()
	{
		std::ifstream is(m_input, std::ios_base::in | std::ios_base::binary);
		if (is.fail())
			throw failed_to_open_a_file(m_input);
		std::ofstream os(m_output, std::ios_base::out | (m_fDiscardOutput?std::ios_base::trunc:std::ios_base::app));
#ifdef _MSC_VER
		os.imbue(std::locale(std::locale(""), new std::codecvt_utf8<wchar_t>()));
#else
		os.imbue(std::locale(""));
#endif
		if (os.fail() || os.rdbuf()->pubseekoff(std::ofstream::off_type(), std::ios_base::end, std::ios_base::out) != std::ofstream::pos_type())
			throw failed_to_open_a_file(m_output);
		bin2text(m_domain, is, os);
		return *this;
	}
private:
	std::string m_domain;
	std::string m_input;
	std::string m_output;
	static std::string m_help_str;
	bool m_fDiscardOutput = false;

	void add_file(std::string str)
	{
		if (m_input.empty())
			m_input = std::move(str);
		else if (m_output.empty())
			m_output = std::move(str);
		else
			throw invalid_usage();
	}
};

std::string Program::m_help_str =
"bin2txt <--domain <domain_name> [--discard_output] <input_binary_file> <output_text_file>>|<--help>\n"\
" --domain specifies a domain system id for which the program should perform the conversion.\n"\
" --discard_output is a switch which allows to truncate the output file before the processing, should the file exist. Otherwise, if\n"\
"       the output file exists and not empty, the program will fail.\n"\
" input_binary_file specifies a path to raw results of CAMaaS simulation to convert to text.\n"\
" output_text_file specifies a path to the output text file.\n"\
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