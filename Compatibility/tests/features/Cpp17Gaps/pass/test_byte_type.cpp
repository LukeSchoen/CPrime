// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: std_byte. <cstddef> declares std::byte in C++17.
#include <cstddef>

int main() {
  std::byte value{7};
  std::byte same{7};
  if (!(value == same)) return 1;
  return std::to_integer<int>(value) - 7;
}
