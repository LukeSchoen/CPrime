// EXPECT_COMPILE_ARGS: -std=c++17
// A non-type pack element used as an explicit template argument in a call.
// Deduction from a parameter type and plain argument expansion already work;
// this shape is what std::get<I>(t)... needs.
#include <tuple>

template<int I> int at() { return I; }

template<int... Is> int sum_at() { return (at<Is>() + ...); }

template<int... Is> int sum_tuple(const std::tuple<int, int, int> &values) {
  int elements[] = {std::get<Is>(values)...};
  int total = 0;
  for (int i = 0; i < 3; ++i) total += elements[i];
  return total;
}

int main() {
  if (sum_at<1, 2, 3>() != 6) return 1;
  std::tuple<int, int, int> values{1, 2, 3};
  return sum_tuple<0, 1, 2>(values) - 6;
}
