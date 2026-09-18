// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: std_aligned_storage. std::aligned_storage is missing.
#include <type_traits>

int main() {
  std::aligned_storage<16, 8>::type storage;
  (void)storage;
  if (sizeof(storage) != 16) return 1;
  return alignof(decltype(storage)) == 8 ? 0 : 2;
}
