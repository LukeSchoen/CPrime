// EXPECT_COMPILE_FAIL: 1
struct Base { constexpr int get() const { return 7; } };
struct Derived : private Base {};
int read(Derived &value) {
  auto pointer = &Base::get;
  return (value.*pointer)();
}
