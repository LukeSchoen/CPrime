// EXPECT_COMPILE_FAIL: 1
struct Owner { void operation(); };
void Owner::operation() = default;
