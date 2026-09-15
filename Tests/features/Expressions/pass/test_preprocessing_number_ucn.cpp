#define \u03B1 9
#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)
constexpr const char *number = STRINGIFY(1\u03B1);
static_assert(number[0] == '1');
static_assert((unsigned char)number[1] == 0xCE);
static_assert((unsigned char)number[2] == 0xB1);
static_assert(number[3] == 0);
int main() { return number[0] != '1' || (unsigned char)number[1] != 0xCE || (unsigned char)number[2] != 0xB1 || number[3] != 0; }