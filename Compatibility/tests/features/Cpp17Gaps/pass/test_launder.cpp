// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_launder. The __builtin_launder spelling works; std::launder
// does not resolve.
#include <new>

int main() {
  int value = 1;
  int *pointer = std::launder(&value);
  return *pointer - 1;
}
