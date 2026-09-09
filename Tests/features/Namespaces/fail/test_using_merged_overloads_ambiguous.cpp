// EXPECT_COMPILE_FAIL: 1
namespace Imported { int read(int); }
namespace Combined {
  int read(int);
  using Imported::read;
}
int main() { return Combined::read(1); }
