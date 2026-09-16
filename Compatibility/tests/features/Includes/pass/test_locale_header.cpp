#include <locale>

int main()
{
	std::locale loc;
	return std::use_facet<std::ctype<char> >(loc).is(std::ctype_base::space, ' ') ? 0 : 1;
}
