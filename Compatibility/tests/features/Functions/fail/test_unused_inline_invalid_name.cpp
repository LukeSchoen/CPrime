// EXPECT_COMPILE_FAIL: 1
inline int unused_function() { return nonexistent_value; }
int main() {}
