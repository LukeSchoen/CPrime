// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
// Initialization follows declaration order, so a's initializer runs before b
// exists and reading b there is not a constant expression, whatever order the
// mem-initializer list uses.
struct Order {
  int a;
  int b;
  constexpr Order() : b(2), a(b + 1) {}
};

constexpr Order value{};

int main() { return value.a; }
