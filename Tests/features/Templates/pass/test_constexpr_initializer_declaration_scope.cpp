constexpr long long value = 7;
constexpr int values[1] = {9};
constexpr int scalar_scope() { int value = sizeof(value); return value; }
constexpr int array_scope() { int values[2] = {sizeof(values), ::values[0]}; return values[0] + values[1]; }
constexpr int global_scope() { int value = ::value; return value; }
static_assert(scalar_scope() == sizeof(int));
static_assert(array_scope() == 2 * sizeof(int) + 9);
static_assert(global_scope() == 7);
int main() { return scalar_scope() != sizeof(int) || array_scope() != 2 * sizeof(int) + 9 || global_scope() != 7; }