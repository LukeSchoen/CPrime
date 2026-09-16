// EXPECT_COMPILE_FAIL: 1
struct Pointer { int *value; };
constexpr Pointer bad(int value) { return Pointer{&value}; }
static_assert(*bad(7).value == 7);
