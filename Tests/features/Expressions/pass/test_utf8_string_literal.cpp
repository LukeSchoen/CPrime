constexpr const char *text = u8"ab";
static_assert(sizeof(u8"ab") == 3);
static_assert(text[0] == 'a');
#define UTF8_TEXT u8"a" u8"b"
static_assert(sizeof(UTF8_TEXT) == 3);
constexpr const char *supplementary = u8"\U0001F600";
static_assert(sizeof(u8"\U0001F600") == 5);
static_assert(static_cast<unsigned char>(supplementary[0]) == 0xF0);
static_assert(static_cast<unsigned char>(supplementary[3]) == 0x80);
template<char C> constexpr char character() { return C; }
static_assert(character<u8'z'>() == 'z');
int main() { return text[1] != 'b' || static_cast<unsigned char>(supplementary[1]) != 0x9F; }
