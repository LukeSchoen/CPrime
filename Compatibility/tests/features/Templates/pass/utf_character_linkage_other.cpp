#include "utf_character_linkage.h"
int character_kind(wchar_t) { return 1; }
int character_kind(char16_t) { return 2; }
int character_kind(char32_t) { return 3; }
int character_kind(unsigned short) { return 4; }
int character_kind(unsigned int) { return 5; }
char16_t exchange_character(char16_t &location, char16_t replacement)
{
  char16_t previous = location;
  location = replacement;
  return previous;
}
char32_t exchange_character(char32_t &location, char32_t replacement)
{
  char32_t previous = location;
  location = replacement;
  return previous;
}
unsigned int read_character_pair(const CharacterPair<char16_t> &value)
{
  return (unsigned int)value.first + value.second;
}
unsigned int read_character_pair(const CharacterPair<char32_t> &value)
{
  return (unsigned int)value.first + value.second;
}
