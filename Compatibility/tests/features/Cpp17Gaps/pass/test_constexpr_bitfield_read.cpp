// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: constexpr_bitfield. Bit-field members written and read back
// inside one constant evaluation.
struct Flags {
  unsigned low : 3;
  unsigned high : 5;
};

constexpr int read_back() {
  Flags flags{};
  flags.low = 5;
  flags.high = 17;
  return (int)(flags.low + flags.high);
}

static_assert(read_back() == 22, "bit-field write and read");

int main() { return 0; }
