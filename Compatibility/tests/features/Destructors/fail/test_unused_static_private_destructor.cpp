// EXPECT_COMPILE_FAIL: 1
struct Value { private: ~Value() {} };
inline void unused() { static Value value; }
int main() {}
