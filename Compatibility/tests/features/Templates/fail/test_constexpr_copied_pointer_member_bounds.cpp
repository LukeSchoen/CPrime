// EXPECT_COMPILE_FAIL: 1
struct Pointer { const int *value; };
constexpr int bad() { int a[2][2] = {{1,2},{3,4}}; Pointer first{a[0]}; Pointer copy{first}; return copy.value[2]; }
static_assert(bad() == 3);
