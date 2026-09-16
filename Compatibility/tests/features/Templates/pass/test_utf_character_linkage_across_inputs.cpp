// EXPECT_SOURCES: ["utf_character_linkage_other.cpp"]
#include "utf_character_linkage.h"
int main()
{
  char16_t unit = char16_t(0xf123);
  char32_t point = char32_t(0xfedcba98u);
  if (character_kind(wchar_t(1)) != 1 || character_kind(unit) != 2 || character_kind(point) != 3) return 1;
  if (character_kind(static_cast<unsigned short>(1)) != 4 || character_kind(1u) != 5) return 2;
  if (exchange_character(unit, char16_t(0x1234)) != 0xf123 || unit != 0x1234) return 3;
  if (exchange_character(point, char32_t(0x12345)) != 0xfedcba98u || point != 0x12345) return 4;
  CharacterPair<char16_t> units = {char16_t(0x1234), char16_t(0x4321)};
  CharacterPair<char32_t> points = {char32_t(0xf0000000u), char32_t(0x01234567)};
  if (read_character_pair(units) != 0x5555 || read_character_pair(points) != 0xf1234567u) return 5;
  return 0;
}
