// EXPECT_COMPILE_FAIL: 1
struct Value { const int member; };
constexpr int bad() { Value value{1}; *(int *)&value.member = 2; return value.member; }
static_assert(bad() == 2);
