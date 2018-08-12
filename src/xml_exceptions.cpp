#include <xml_exceptions.h>
#include <codecvt>
#include <locale>

std::string make_exception_message(const std::wstring& what)
{
	typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt;
	return std::wstring_convert<codecvt>(new std::codecvt_byname<wchar_t, char, std::mbstate_t>("")).to_bytes(what);
}
