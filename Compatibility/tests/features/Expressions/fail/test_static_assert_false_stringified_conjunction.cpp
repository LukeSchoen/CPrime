// EXPECT_COMPILE_FAIL: 1
#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)
constexpr const char *text = STRINGIFY(1\u03B1);
static_assert(text[0] == '1' && text[1] == '\\');
int main() { return 0; }