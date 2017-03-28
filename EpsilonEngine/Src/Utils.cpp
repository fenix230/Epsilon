#include "Utils.h"
#include <vector>


namespace epsilon
{

	std::wstring epsilon::ToWstring(const std::string& str, const std::locale& loc /*= std::locale()*/)
	{
		std::vector<wchar_t> buf(str.size());
		std::use_facet<std::ctype<wchar_t>>(loc).widen(str.data(),//ctype<char_type>  
			str.data() + str.size(),
			buf.data());//把char转换为T  
		return std::wstring(buf.data(), buf.size());
	}

	std::string epsilon::ToString(const std::wstring& str, const std::locale& loc /*= std::locale()*/)
	{
		std::vector<char> buf(str.size());
		std::use_facet<std::ctype<wchar_t>>(loc).narrow(str.data(),
			str.data() + str.size(),
			'?', buf.data());//把T转换为char  
		return std::string(buf.data(), buf.size());
	}

}