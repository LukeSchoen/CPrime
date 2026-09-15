constexpr bool array_identity() { bool values[1] = {values == &values[0]}; return values[0]; }
static_assert(array_identity());
int main() { return !array_identity(); }