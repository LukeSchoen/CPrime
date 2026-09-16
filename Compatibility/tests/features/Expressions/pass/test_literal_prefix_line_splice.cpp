constexpr const char *raw = u\
8\
R"(a\nb)";
constexpr const char16_t *encoded = u\
"ab";
constexpr char32_t character = U\
'z';
static_assert(raw[1] == '\\' && raw[2] == 'n');
static_assert(encoded[1] == u'b');
static_assert(character == U'z');
#define u8R invalid_prefix_macro
constexpr const char *macro_control = u8R"(ok)";
int main() { return macro_control[0] != 'o'; }