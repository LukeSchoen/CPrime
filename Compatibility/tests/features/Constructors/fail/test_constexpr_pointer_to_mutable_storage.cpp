// EXPECT_COMPILE_FAIL: 1
int values[] = {7};
struct Pointer { const int *value; };
constexpr Pointer pointer = {values};
static_assert(pointer.value[0] == 7, "mutable storage is not a constant expression");
