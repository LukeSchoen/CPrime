// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_as_const. <utility> declares std::as_const but never
// defines it, so the program links against an undefined symbol.
#include <utility>

int main() {
  int value = 1;
  const int &reference = std::as_const(value);
  value = 2;
  return reference == 2 ? 0 : 1;
}
