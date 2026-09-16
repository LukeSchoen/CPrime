// EXPECT_COMPILE_FAIL: 1
constexpr int bad() {
  int values[] = {3};
  int *pointer = nullptr;
  for (auto value : values) pointer = &value;
  return *pointer;
}
static_assert(bad() == 3);
