constexpr const char *text = R"tag(a\nb)tag";
static_assert(sizeof(R"tag(a\nb)tag") == 5);
static_assert(text[1] == '\\' && text[2] == 'n');
#define RAW_TEXT R"(a"b)"
static_assert(sizeof(RAW_TEXT) == 4);
constexpr const char *multiline = R"(a
b)";
static_assert(multiline[1] == '\n');
constexpr const char *spliced = R"(a\
b)";
static_assert(spliced[1] == '\\' && spliced[2] == '\n');
constexpr const char16_t *utf16 = uR"(\u0041)";
constexpr const char32_t *utf32 = UR"(\n)";
constexpr const char *utf8 = u8R"(\n)";
constexpr const wchar_t *wide = LR"(\n)";
static_assert(utf16[0] == u'\\' && utf16[1] == u'u');
static_assert(utf32[1] == U'n' && utf8[1] == 'n' && wide[1] == L'n');
int main() { return text[3] != 'b' || spliced[2] != '\n' || utf16[1] != u'u'; }
