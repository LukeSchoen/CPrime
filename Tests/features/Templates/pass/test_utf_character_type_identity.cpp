#include <type_traits>

static_assert(sizeof(char16_t) == 2 && sizeof(char32_t) == 4, "UTF storage widths");
static_assert(!std::is_same<char16_t, unsigned short>::value, "char16 is distinct");
static_assert(!std::is_same<char32_t, unsigned int>::value, "char32 is distinct");
static_assert(!std::is_same<wchar_t, char16_t>::value, "wide and UTF are distinct");
static_assert(std::is_integral<const volatile char16_t>::value, "qualified UTF integer");

int select(wchar_t) { return 1; }
int select(char16_t) { return 2; }
int select(unsigned short) { return 3; }
int select(char32_t) { return 4; }
int select(unsigned int) { return 5; }

template<class T> struct Character { static const int value = 0; };
template<> struct Character<wchar_t> { static const int value = 1; };
template<> struct Character<char16_t> { static const int value = 2; };
template<> struct Character<char32_t> { static const int value = 4; };
template<class T> int deduced(T value) { return Character<T>::value + select(value); }
template<class T> T identity(T value) { return value; }

int main()
{
  wchar_t wide = wchar_t(0x5678);
  char16_t shortText[] = {char16_t(0xfedc), char16_t(0x0123)};
  char32_t longText[] = {char32_t(0xfedcba98u), char32_t(0x12345)};
  if (select(wide) != 1 || select(shortText[0]) != 2 || select(longText[0]) != 4) return 1;
  if (select(static_cast<unsigned short>(shortText[0])) != 3) return 2;
  if (select(static_cast<unsigned int>(longText[0])) != 5) return 3;
  if (deduced(wide) != 2 || deduced(shortText[0]) != 4 || deduced(longText[0]) != 8) return 4;
  if (identity(shortText[0]) != 0xfedc || identity(longText[0]) != 0xfedcba98u) return 5;
  shortText[1] = char16_t(shortText[1] + 2);
  longText[1] = char32_t(longText[1] + 3);
  if (shortText[0] != 0xfedc || shortText[1] != 0x125) return 6;
  if (longText[0] != 0xfedcba98u || longText[1] != 0x12348) return 7;
  return 0;
}
