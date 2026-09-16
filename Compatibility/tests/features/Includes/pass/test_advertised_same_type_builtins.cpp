#include <type_traits>
#if __has_builtin(__is_same)
static_assert(__is_same(int, int));
static_assert(!__is_same(int, const int));
static_assert(!__is_same(int &, int &&));
static_assert(!__is_same(int[2], int[3]));
static_assert(std::is_same<decltype(__is_same(int, int)), bool>::value);
#endif
#if __has_builtin(__is_same_as)
using Alias = int;
static_assert(__is_same_as(Alias, int));
static_assert(!__is_same_as(void(), void() noexcept));
#endif
int main() { return 0; }
