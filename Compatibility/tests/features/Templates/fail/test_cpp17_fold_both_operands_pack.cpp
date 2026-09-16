// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
template<class... T> int bad(T... x) { return (x + ... + x); }
int main() { return bad(1, 2); }
