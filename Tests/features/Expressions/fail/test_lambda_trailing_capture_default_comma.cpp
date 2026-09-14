// EXPECT_COMPILE_FAIL: 1
int main() {
  auto invalid = [=,] { return 1; };
}
