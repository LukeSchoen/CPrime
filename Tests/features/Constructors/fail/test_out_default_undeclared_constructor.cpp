// EXPECT_COMPILE_FAIL: 1
struct Owner { int value; };
Owner::Owner() = default;
int main() { return 0; }
