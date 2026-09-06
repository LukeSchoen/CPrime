// EXPECT_COMPILE_FAIL: 1
struct Explicit { explicit Explicit(int) {} };
int main() { Explicit value(1); value=2; }
