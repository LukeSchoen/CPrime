static_assert(sizeof(U"ab") == 3 * sizeof(char32_t));
constexpr const char32_t *text = U"ab";
static_assert(text[0] == U'a');
#define UTF32_TEXT U"a" U"b"
static_assert(sizeof(UTF32_TEXT) == 3 * sizeof(char32_t));
constexpr char32_t supplementary[] = U"\U0001F600";
static_assert(sizeof(supplementary) == 2 * sizeof(char32_t));
static_assert(supplementary[0] == 0x1F600);
template<char32_t C> constexpr char32_t character() { return C; }
static_assert(character<U'\U0001F600'>() == 0x1F600);
int main() { return text[1] != U'b' || supplementary[0] != 0x1F600; }
