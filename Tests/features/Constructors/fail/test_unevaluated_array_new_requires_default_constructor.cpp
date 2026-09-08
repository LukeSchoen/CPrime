// EXPECT_COMPILE_FAIL: 1
struct Object { Object(int); };
int main() { return sizeof(new Object[3]); }
