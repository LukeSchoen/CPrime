template<char C> struct character { static constexpr int value = C; };
static_assert(character<'1'>::value == 49);
static_assert(character<'!'>::value == 33);
static_assert(character<'?'>::value == 63);
static_assert(character<'\n'>::value == 10);
template<char C> int runtime_character() { return C; }
int main() { return runtime_character<'!'>() != 33 || runtime_character<'?'>() != 63 || runtime_character<'\n'>() != 10; }