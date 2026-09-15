constexpr bool scalar_identity() { bool value = &value == &value; return value; }
constexpr bool array_identity() { bool values[1] = {values == &values[0]}; return values[0]; }
static_assert(scalar_identity());
static_assert(array_identity());
int main() { return !scalar_identity() || !array_identity(); }