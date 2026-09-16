// EXPECT_COMPILE_FAIL: 1
struct Function { private: void operator()(int); };
int main() { Function value; value(1); }
