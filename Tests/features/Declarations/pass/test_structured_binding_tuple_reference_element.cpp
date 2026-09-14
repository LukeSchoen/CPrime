// EXPECT_COMPILE_ARGS: -std=c++17
#include <tuple>
template<class A, class B> constexpr bool same = false;
template<class A> constexpr bool same<A, A> = true;
int main() {
  int value = 3;
  std::tuple<int &> tuple(value);
  auto [alias] = tuple;
  static_assert(same<decltype(alias), int &>);
  static_assert(same<decltype((alias)), int &>);
  alias = 7;
  return value != 7 || &alias != &value;
}
