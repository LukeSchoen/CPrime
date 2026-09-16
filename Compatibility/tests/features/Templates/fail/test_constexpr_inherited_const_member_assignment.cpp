// EXPECT_COMPILE_FAIL: 1
struct Inner { mutable int member; int fixed; };
struct Value { Inner inner; };
constexpr int bad() { const Value value{{3, 4}}; *(int *)&value.inner.fixed = 7; return 7; }
static_assert(bad() == 7);
