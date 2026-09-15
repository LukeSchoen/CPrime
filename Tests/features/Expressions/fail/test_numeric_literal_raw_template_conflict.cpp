// EXPECT_COMPILE_FAIL: 1
template<char... C> constexpr int operator "" _conflict() { return 1; }
constexpr int operator "" _conflict(const char *) { return 2; }
int value = 123_conflict;
