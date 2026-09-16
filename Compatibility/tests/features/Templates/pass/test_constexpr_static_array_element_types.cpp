constexpr double fractions[] = {2.5, 4.5};
constexpr long long integers[] = {-9, 4294967297LL};
int global;
constexpr int *pointers[] = {&global, nullptr};
constexpr signed char small[] = {-3, 7};
static_assert(fractions[1] == 4.5);
static_assert(integers[0] == -9);
static_assert(integers[1] == 4294967297LL);
static_assert(pointers[0] == &global);
static_assert(pointers[1] == nullptr);
static_assert(small[0] == -3);
static_assert("abc"[1] == 'b');
static_assert(sizeof(integers[100]) == sizeof(long long));
int main() {}
