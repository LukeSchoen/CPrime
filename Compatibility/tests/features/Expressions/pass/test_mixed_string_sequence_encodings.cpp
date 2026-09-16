#define PREFIX u"a"
constexpr const char16_t a[] = "\x123" PREFIX R"(\n)";
constexpr const char32_t b[] = "\U0001F600" U"b";
constexpr const char c[] = "a" u8"b" "c";
constexpr const wchar_t d[] = "a" L"b";
constexpr const char16_t e[] = u"\x1" "2";
static_assert(a[0] == 0x123);
static_assert(a[1] == u'a');
static_assert(a[2] == u'\\');
static_assert(a[3] == u'n');
static_assert(sizeof(a) == 5 * sizeof(char16_t));
static_assert(b[0] == 0x1F600);
static_assert(sizeof(b) == 3 * sizeof(char32_t));
static_assert(c[2] == 'c');
static_assert(d[1] == L'b');
static_assert(e[0] == 1);
static_assert(e[1] == u'2');
int main() { return a[0] != 0x123 || b[0] != 0x1F600 || c[2] != 'c' || d[1] != L'b' || e[1] != u'2'; }