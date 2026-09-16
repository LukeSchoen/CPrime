// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_execution. <execution> is missing from the runtime.
#include <execution>

int main() {
  (void)std::execution::seq;
  (void)std::execution::par;
  return 0;
}
