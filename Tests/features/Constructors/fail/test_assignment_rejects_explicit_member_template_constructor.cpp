// EXPECT_COMPILE_FAIL: 1
struct Explicit {
  template<class T> explicit Explicit(T) {}
};
int main() { Explicit value(1); value = 2; }
