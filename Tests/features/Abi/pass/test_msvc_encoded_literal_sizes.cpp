// EXPECT_COMPILE_ARGS: -std=c++17
// Microsoft x64 encoded-literal widths: char16_t and wchar_t are two bytes,
// char32_t is four, and each string literal carries a terminator.
static_assert(sizeof(char16_t) == 2, "char16_t width");
static_assert(sizeof(char32_t) == 4, "char32_t width");
static_assert(sizeof(wchar_t) == 2, "wchar_t width");

static_assert(sizeof(u"ab") == 6, "u\"ab\"");
static_assert(sizeof(U"ab") == 12, "U\"ab\"");
static_assert(sizeof(L"ab") == 6, "L\"ab\"");
static_assert(sizeof(u'a') == 2, "u'a'");
static_assert(sizeof(L'a') == 2, "L'a'");
static_assert(sizeof(U'a') == 4, "U'a'");

int main() { return 0; }
