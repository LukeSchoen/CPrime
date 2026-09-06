// EXPECT_COMPILE_FAIL: 1
struct Invalid { static_assert(sizeof(int)==0,"int has storage"); };
int main() { return 0; }
