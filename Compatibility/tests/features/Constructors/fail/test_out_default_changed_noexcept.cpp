// EXPECT_COMPILE_FAIL: 1
struct Owner { Owner() noexcept; };
Owner::Owner() noexcept(false) = default;
int main() { return 0; }
