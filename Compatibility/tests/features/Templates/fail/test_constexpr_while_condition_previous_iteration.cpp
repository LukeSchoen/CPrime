// EXPECT_COMPILE_FAIL: 1
constexpr int bad() {
  int *previous = nullptr;
  int count = 2;
  while (int value = count--) {
    if (previous) return *previous;
    previous = &value;
  }
  return 0;
}
static_assert(bad() == 2);
