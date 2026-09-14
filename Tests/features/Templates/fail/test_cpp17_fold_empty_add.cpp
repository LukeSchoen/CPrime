// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
// An empty unary + fold has no identity in C++17.
template<class... T> int sum(T... x) { return (... + x); }
int main() { return sum(); }
