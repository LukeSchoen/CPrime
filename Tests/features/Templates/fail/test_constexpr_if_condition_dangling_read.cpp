// EXPECT_COMPILE_FAIL: 1
constexpr int bad() {
  int *pointer = nullptr;
  if (int value = 7) pointer = &value;
  return *pointer;
}
static_assert(bad() == 7);
