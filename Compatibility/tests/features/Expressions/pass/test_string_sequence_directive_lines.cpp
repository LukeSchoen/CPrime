#line 100
constexpr const char *text = "a"
#define PART "b"
PART;
static_assert(text[0] == 'a');
static_assert(text[1] == 'b');
#line 200
constexpr const char *single = "x";
static_assert(__LINE__ == 201);
#line 300
constexpr int line = sizeof("a") + __LINE__;
static_assert(line == 302);
int main() { return text[1] != 'b' || line != 302; }