// EXPECT_COMPILE_ARGS: -std=c++17
// `basic_string`'s iterator-pair constructor, which is what
// `return std::string(begin, end);` calls in
// boost/type_index/stl_type_index.hpp.  CPC picks the
// `(const char *, size_type)` constructor instead and reports
// "cannot convert 'const char *' to 'unsigned long long'".
// `std::vector` and `basic_string::assign` already accept the pair.
#include <string>

int main()
{
	const char text[] = "abc";
	std::string value(text, text + 3);
	return value.size() == 3 ? 0 : 1;
}
