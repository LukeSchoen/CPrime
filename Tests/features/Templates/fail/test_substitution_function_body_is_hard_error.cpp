// EXPECT_COMPILE_FAIL: 1
// Deducing an auto return instantiates the body; its errors are not SFINAE.
#include <utility>
template<class T> auto broken(T value) { return value.missing(); }
template<class T, class = decltype(broken(std::declval<T>()))> int select(T);
int select(...);
int main() { return select(1); }
