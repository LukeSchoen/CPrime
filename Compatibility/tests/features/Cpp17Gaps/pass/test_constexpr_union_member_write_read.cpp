// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: constexpr_union. A union member written and read back inside
// one constant evaluation.
union Value {
  int integer;
  unsigned bits;
};

constexpr int read_after_write() {
  Value value{};
  value.integer = 7;
  return value.integer;
}

static_assert(read_after_write() == 7, "union member write then read");

int main() { return 0; }
