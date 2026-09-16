// EXPECT_COMPILE_FAIL: 1
constexpr int bad() {
  int *pointer = nullptr;
  for (; int value = 7;) {
    pointer = &value;
    break;
  }
  return *pointer;
}
static_assert(bad() == 7);
