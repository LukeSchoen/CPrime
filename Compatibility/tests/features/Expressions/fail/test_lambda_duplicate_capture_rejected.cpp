// EXPECT_COMPILE_FAIL: 1
int main() {
  int value = 1;
  auto invalid = [value, value]() { return value; };
  return invalid();
}
