// EXPECT_COMPILE_FAIL: 1
struct Value { mutable int member; };
constexpr Value source{4};
constexpr int read(Value value) { return value.member; }
static_assert(read(source) == 4);
