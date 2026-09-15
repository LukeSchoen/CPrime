template<char... C> constexpr int operator "" _spelling_pack() {
    constexpr char text[] = {C..., 0};
    return text[0] == '1' && text[1] == '\'' && text[2] == '2' && text[3] == 0;
}
template<char... C> constexpr int operator "" _exponent_pack() {
    constexpr char text[] = {C..., 0};
    return text[0] == '1' && text[1] == 'e' && text[2] == '+' && text[3] == '2' && text[4] == 0;
}
template<char... C> constexpr int operator "" _priority() { return 1; }
constexpr int operator "" _priority(unsigned long long) { return 2; }
static_assert(1'2_spelling_pack == 1);
static_assert(1e+2_exponent_pack == 1);
static_assert(123_priority == 2);
int main() { return 1'2_spelling_pack != 1 || 1e+2_exponent_pack != 1 || 123_priority != 2; }