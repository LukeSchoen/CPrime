// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_bitset. set/test/[] work; count() does not.
#include <bitset>

int main() {
  std::bitset<8> bits(0xB2);
  if (bits.count() != 4) return 1;
  bits.set(0);
  if (bits.count() != 5) return 2;
  bits.reset(0);
  return bits.count() == 4 ? 0 : 3;
}
