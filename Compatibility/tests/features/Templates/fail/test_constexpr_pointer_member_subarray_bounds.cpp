// EXPECT_COMPILE_FAIL: 1
struct Pointer { const int *value; };
constexpr int bad() { int values[2][2] = {{1,2},{3,4}}; Pointer p = {values[0]}; return p.value[2]; }
static_assert(bad() == 3);
