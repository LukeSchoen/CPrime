// EXPECT_COMPILE_ARGS: -std=c++17
template<class A, class B> struct same_type { static constexpr bool value = false; };
template<class A> struct same_type<A, A> { static constexpr bool value = true; };
template<class A, class B> constexpr bool same = same_type<A, B>::value;
int main() {
  int values[2] = {3, 5};
  auto &[a, b] = values;
  static_assert(same<decltype(a), int>);
  static_assert(same<decltype((a)), int &>);
  const auto [c, d] = values;
  static_assert(same<decltype(c), const int>);
  static_assert(same<decltype((c)), const int &>);
  auto &&[e, f] = values;
  static_assert(same<decltype(e), int>);
  e = 7;
  return a != 7 || b != 5 || c != 3 || d != 5 || &f != &values[1];
}
