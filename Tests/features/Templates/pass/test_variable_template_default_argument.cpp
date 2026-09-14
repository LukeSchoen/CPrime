template<class T, class U = T> constexpr bool equal_size = sizeof(T) == sizeof(U);
template<class T, class U = T> constexpr bool forwarded = equal_size<T, U>;
int main() {
  static_assert(equal_size<int>);
  static_assert(equal_size<char, char>);
  static_assert(!equal_size<char, long long>);
  static_assert(forwarded<int>);
  static_assert(!forwarded<char, long long>);
}
