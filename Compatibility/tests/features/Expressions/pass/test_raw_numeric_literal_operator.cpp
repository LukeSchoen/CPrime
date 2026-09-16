constexpr int operator "" _spelling(const char *p) { return p[0] == '0' && p[1] == 'x' && p[2] == 'A' && p[3] == 0; }
constexpr int operator "" _choice(const char *) { return 1; }
constexpr int operator "" _choice(unsigned long long) { return 2; }
static_assert(0xA_spelling == 1);
static_assert(12_choice == 2);
constexpr int operator "" _first(const char *p) { return p[0]; }
static_assert(999999999999999999999999999999999999999999999999999_first == '9');
static_assert(1.25e+3_first == '1');
int main() { return 0xA_spelling != 1 || 12_choice != 2; }