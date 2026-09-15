// EXPECT_COMPILE_FAIL: 1
struct Value { int member; };
constexpr const int *escape(Value value) { return &value.member; }
constexpr const int *pointer = escape(Value{3});
