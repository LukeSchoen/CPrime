// EXPECT_COMPILE_FAIL: 1
struct Pointer { const int *value; };
constexpr const int *bad() { int values[2] = {1,2}; Pointer p = {values}; return p.value; }
constexpr const int *escaped = bad();
