static_assert(sizeof(u"ab") == 3 * sizeof(char16_t));
constexpr const char16_t *text = u"ab";
static_assert(text[0] == u'a');
#define UTF16_TEXT u"a" u"b"
static_assert(sizeof(UTF16_TEXT) == 3 * sizeof(char16_t));
constexpr char16_t supplementary[] = u"\U0001F600";
static_assert(sizeof(supplementary) == 3 * sizeof(char16_t));
static_assert(supplementary[0] == 0xD83D && supplementary[1] == 0xDE00);
template<char16_t C> constexpr char16_t character() { return C; }
static_assert(character<u'z'>() == u'z');
int main() { return text[1] != u'b' || supplementary[1] != 0xDE00; }
