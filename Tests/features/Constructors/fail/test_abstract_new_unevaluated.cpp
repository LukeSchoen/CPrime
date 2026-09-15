// EXPECT_COMPILE_FAIL: 1
struct Abstract { virtual void run() = 0; };
int main() { return sizeof(new Abstract); }
