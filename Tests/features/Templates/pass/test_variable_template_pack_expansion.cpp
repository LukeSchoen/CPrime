template<class... T> struct Count { static constexpr int value = sizeof...(T); };
template<class... T> inline constexpr int count = Count<T...>::value;
template<class First, class... Rest>
inline constexpr int tail = Count<Rest...>::value;
template<int... Values> inline constexpr int sum = (0 + ... + Values);
static_assert(count<> == 0, "empty type pack");
static_assert(count<int, double, char> == 3, "multiple type arguments");
static_assert(tail<int> == 0, "empty trailing pack");
static_assert(tail<int, double, char> == 2, "fixed argument before pack");
static_assert(sum<> == 0, "empty value pack");
static_assert(sum<1, 2, 3> == 6, "value pack fold");
int main() { return 0; }
