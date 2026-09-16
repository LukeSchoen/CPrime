// EXPECT_COMPILE_FAIL: 1
constexpr int bad() { int *pointer = nullptr; for (int i = 0; i < 1; ++i) pointer = &i; return *pointer; }
static_assert(bad() == 1);
