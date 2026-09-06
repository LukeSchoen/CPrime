#include <type_traits>

// The token must retain its integer type even when the packed value fits char.
static_assert(std::is_same<decltype('\0A'), int>::value, "multicharacter type");
int main() { return '\0A' != 65; }
