// EXPECT_COMPILE_FAIL: 1
#if 0
#else
#elifdef PRESENT
#endif
int main(void) { return 0; }
