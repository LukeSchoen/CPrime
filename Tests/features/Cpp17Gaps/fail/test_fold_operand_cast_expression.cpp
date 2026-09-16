// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
// A fold operand must be a cast-expression, so a compound pattern such as
// "values + 1" has no valid parse in (values + 1 + ...).
template<class... T> int sum(T... values) { return (values + 1 + ...); }

int main() { return sum(1, 2); }
