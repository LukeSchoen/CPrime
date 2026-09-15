// EXPECT_COMPILE_FAIL: 1
struct Value { int *pointer; };
constexpr Value bad() { int local = 7; return Value{&local}; }
static_assert(*bad().pointer == 7);
