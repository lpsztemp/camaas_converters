#include <domain_converter.h>
#include <impl_xml_parser.h>
//#include <exceptions.h>

void skip_xml_domain_data(std::istream& is) //"is" is associated with the first character after the closing '>'
{
	int level = 1;
	char linebuf[100];
	do
	{
		is.getline(linebuf, sizeof(linebuf), '<');
		if (is.eof())
			throw xml_invalid_syntax(is.tellg());
		auto tag = xml_tag(is);
		if (tag.name() != "domain")
			continue;
		if (!tag.is_closing_tag())
		{
			++level;
			continue;
		}
		--level;
	}while (level != 0);
}