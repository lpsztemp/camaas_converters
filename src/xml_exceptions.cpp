#include <xml_exceptions.h>
#include <codecvt>
#include <locale>

std::string make_exception_message(const std::wstring& what)
{
	typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt;
	return std::wstring_convert<codecvt>(&std::use_facet<codecvt>(std::locale(""))).to_bytes(what);
}