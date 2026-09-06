// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
namespace flags {
enum class Options { first = 1, second = 2 };
Options operator|(Options left, Options right) {
  return static_cast<Options>(static_cast<int>(left) | static_cast<int>(right));
}
int operator+(Options left, int right) { return static_cast<int>(left) + right + 10; }
int consume(Options value) { return static_cast<int>(value); }
}
enum Value { one = 1, two = 2 };
int operator+(Value left, Value right) { return static_cast<int>(left) + static_cast<int>(right) + 20; }
int main() {
  if (flags::consume(flags::Options::first | flags::Options::second) != 3) return 1;
  if (flags::Options::first + 4 != 15) return 2;
  return one + two != 23;
}
