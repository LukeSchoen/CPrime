constexpr int values[2] = {4, 9};
constexpr bool first(const int *pointer) { return pointer == &values[0]; }
constexpr bool second(const int *pointer) { return pointer == &values[1]; }
constexpr int read(const int *pointer) { return *pointer; }
static_assert(first(&values[0]));
static_assert(!first(&values[1]));
static_assert(second(&values[1]));
static_assert(read(&values[0]) == 4);
static_assert(read(&values[1]) == 9);
int main() {}
