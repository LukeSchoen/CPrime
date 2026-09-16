// EXPECT_COMPILE_FAIL: 1
constexpr const char *text = "1\u03B1";
static_assert(text[0] == '1' && text[1] == '\\');
int main() { return 0; }