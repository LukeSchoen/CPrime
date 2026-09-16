// EXPECT_COMPILE_FAIL: 1
struct Range {
  int values[1];
  constexpr int *begin() { return values; }
  constexpr int *end() { return values + 1; }
};
constexpr int bad() {
  int *pointer = nullptr;
  for (int &value : Range{{7}}) pointer = &value;
  return *pointer;
}
static_assert(bad() == 7);
