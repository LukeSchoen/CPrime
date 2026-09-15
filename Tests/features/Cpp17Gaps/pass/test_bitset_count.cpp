// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_bitset. set/test/[] work; count() does not.
#include <bitset>

int main() {
  std::bitset<8> bits(0xB3);
  if (bits.count() != 5) return 1;
  bits.set(0);
  if (bits.count() != 6) return 2;
  bits.reset(0);
  return bits.count() == 5 ? 0 : 3;
}
