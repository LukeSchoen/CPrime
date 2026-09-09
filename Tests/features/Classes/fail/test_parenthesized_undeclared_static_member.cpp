// EXPECT_COMPILE_FAIL: 1
struct Owner {};
void (*Owner::missing)() = nullptr;
