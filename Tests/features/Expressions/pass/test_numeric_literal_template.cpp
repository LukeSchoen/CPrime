template<char... Digits>
constexpr unsigned operator "" _digits() { return sizeof...(Digits); }
static_assert(123_digits == 3);
static_assert(0xAB_digits == 4);
int main() { return 123_digits != 3; }