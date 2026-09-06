#ifndef UTF_CHARACTER_LINKAGE_H
#define UTF_CHARACTER_LINKAGE_H
int character_kind(wchar_t);
int character_kind(char16_t);
int character_kind(char32_t);
int character_kind(unsigned short);
int character_kind(unsigned int);
char16_t exchange_character(char16_t &location, char16_t replacement);
char32_t exchange_character(char32_t &location, char32_t replacement);
template<class T> struct CharacterPair { T first, second; };
unsigned int read_character_pair(const CharacterPair<char16_t> &);
unsigned int read_character_pair(const CharacterPair<char32_t> &);
#endif
