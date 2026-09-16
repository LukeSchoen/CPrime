// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr Value first{4}, second{9};
constexpr int read(const Value *pointer) { return pointer->number; }
constexpr int invalid = read(&first + 1);
int main() {}
