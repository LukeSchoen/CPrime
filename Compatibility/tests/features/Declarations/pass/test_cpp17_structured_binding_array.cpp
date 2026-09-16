// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: Clang llvmorg-18.1.8 SemaCXX/cxx1z-decomposition.cpp num_elems.
int main() {
  int values[2] = {4, 9};
  auto [copy_a, copy_b] = values;
  copy_a = 20;
  if (values[0] != 4 || copy_b != 9) return 1;
  auto &[a, b] = values;
  a = 7;
  b = 11;
  return values[0] != 7 || values[1] != 11 || &a != &values[0];
}
