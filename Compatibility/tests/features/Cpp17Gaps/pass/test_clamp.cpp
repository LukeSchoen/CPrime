// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_clamp. <algorithm> declares std::clamp but never defines
// it, so the program links against an undefined symbol.
#include <algorithm>

int main() {
  if (std::clamp(5, 0, 3) != 3) return 1;
  if (std::clamp(-1, 0, 3) != 0) return 2;
  return std::clamp(2, 0, 3) - 2;
}
