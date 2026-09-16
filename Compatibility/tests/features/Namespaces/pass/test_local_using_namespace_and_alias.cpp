// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
namespace first { int value() { return 3; } }
namespace second { int value() { return 5; } }
int main() {
  int result;
  {
    using namespace first;
    using Integer = int;
    Integer selected = value();
    result = selected;
  }
  {
    using namespace second;
    result += value();
  }
  return result != 8;
}
