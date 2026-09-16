// EXPECT_COMPILE_FAIL: 1
int select(const int*);
int select(const volatile int*);
int main() { return select(nullptr); }
