// EXPECT_COMPILE_FAIL: 1
struct Owner { typedef int Value; typedef double Value; };
int main() { return 0; }
