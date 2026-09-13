#include <bitset>

int main() {
  std::bitset<sizeof(int) * 8> bits;
  bits[3] = 1;
  bits[3] = 0;
  bits.set(5);
  return bits[3] || !bits.test(5) || bits.size() != sizeof(int) * 8;
}

// EXPECT_EXIT: 0
