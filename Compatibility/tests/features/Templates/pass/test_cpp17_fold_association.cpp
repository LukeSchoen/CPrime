// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: GCC gcc-14.2.0 g++.dg/cpp1z/fold1.C, fold2.C.
// Independent runtime checks: parentheses and direction matter for subtraction.
template<class... T> int left(T... x) { return (... - x); }
template<class... T> int right(T... x) { return (x - ...); }
template<class... T> int left_seed(T... x) { return (20 - ... - x); }
template<class... T> int right_seed(T... x) { return (x - ... - 20); }
template<class... T> int squares(T... x) { return (0 + ... + (x * x)); }
struct Member {
  template<class... T> int sum(T... x) { return (0 + ... + x); }
};
int main() {
  if (left(10, 3, 2) != 5 || right(10, 3, 2) != 9) return 1;
  if (left(7) != 7 || right(7) != 7) return 2;
  if (left_seed() != 20 || right_seed() != 20) return 3;
  if (left_seed(10, 3, 2) != 5 || right_seed(10, 3, 2) != -11) return 4;
  if (squares(2, 3, 4) != 29) return 5;
  Member m;
  return m.sum(1, 2, 3) != 6 || m.sum() != 0;
}
