// boost/token_functions.hpp includes <cwctype> unconditionally and uses the
// std classification names, so the runtime must publish them.
#include <cwctype>

int main()
{
  std::wint_t upper = L'A';
  if (!std::iswalpha(upper))
    return 1;
  if (!std::iswupper(upper))
    return 2;
  if (std::iswlower(upper))
    return 3;
  if (!std::iswdigit(L'7'))
    return 4;
  if (!std::iswspace(L'\t'))
    return 5;
  if (!std::iswpunct(L','))
    return 6;
  if (std::iswpunct(L'A'))
    return 7;
  if (std::towlower(upper) != L'a')
    return 8;
  if (std::towupper(L'a') != L'A')
    return 9;
  return 0;
}
