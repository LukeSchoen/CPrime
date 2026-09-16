constexpr char16_t small[] = u"\xFFFF\xD800\0";
constexpr char32_t large[] = U"\xFFFFFFFF\x00110000\0";
static_assert(small[0] == 0xFFFF && small[1] == 0xD800 && small[2] == 0);
static_assert(large[0] == 0xFFFFFFFFu && large[1] == 0x110000 && large[2] == 0);
static_assert(u'\xFFFF' == 0xFFFF);
static_assert(U'\xFFFFFFFF' == 0xFFFFFFFFu);
int main() { return small[1] != 0xD800 || large[0] != 0xFFFFFFFFu; }
