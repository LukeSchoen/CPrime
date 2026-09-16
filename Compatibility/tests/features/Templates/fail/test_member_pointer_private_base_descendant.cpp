// EXPECT_COMPILE_FAIL: 1
struct Base { constexpr int get() const { return 7; } };
struct Private : private Base {};
struct Child : Private {
  int read() const { auto pointer = &Base::get; return (this->*pointer)(); }
};
