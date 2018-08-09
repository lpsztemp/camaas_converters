#include <domain_converter.h>
#include <impl_xml_parser.h>
#include <exceptions.h>

void skip_xml_domain_data(text_istream& is) //"is" is associated with the first character after the closing '>'
{
	int level = 1;
	wchar_t linebuf[100];
	do
	{
		is.getline(linebuf, sizeof(linebuf), L'<');
		if (is.eof())
			throw xml_invalid_syntax(is.get_resource_locator());
		is.putback(L'<');
		auto tag = xml::tag(is);
		if (tag.name() != L"domain")
			continue;
		if (!tag.is_closing_tag())
		{
			++level;
			continue;
		}
		--level;
	}while (level != 0);
}