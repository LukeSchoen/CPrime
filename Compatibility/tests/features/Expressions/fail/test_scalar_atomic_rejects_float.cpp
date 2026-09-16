// EXPECT_COMPILE_FAIL: 1
int main() { float value = 1; return __atomic_load_n(&value, __ATOMIC_RELAXED); }
