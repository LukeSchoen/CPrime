// EXPECT_COMPILE_FAIL: 1
struct Owner { Owner& operator=(const Owner&) noexcept; };
Owner& Owner::operator=(const Owner&) noexcept(false) = default;
int main() { return 0; }
