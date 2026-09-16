// EXPECT_COMPILE_FAIL: 1
struct Base { constexpr int get() const { return 7; } };
struct Left : Base {};
struct Right : Base {};
struct Derived : Left, Right {};
constexpr int bad() {
  Derived value{};
  auto pointer = &Base::get;
  return (value.*pointer)();
}
static_assert(bad() == 7);
