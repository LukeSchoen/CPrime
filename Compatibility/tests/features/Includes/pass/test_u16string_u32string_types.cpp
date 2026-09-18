// <string> publishes the four standard string types.  boost/log's
// code_conversion.hpp declares std::u16string and std::u32string parameters, so
// a translation unit that includes it has to know both types and be able to use
// them as strings.
#include <string>

std::u16string utf16;
std::u32string utf32;

int main()
{
  utf16.append(u"abc");
  utf32.append(U"def");
  if (utf16.size() != 3 || utf16[1] != u'b')
    return 1;
  if (utf32.size() != 3 || utf32[2] != U'f')
    return 2;
  std::wstring wide(L"g");
  std::string narrow("h");
  if (wide.size() != 1 || narrow.size() != 1)
    return 3;
  return 0;
}
