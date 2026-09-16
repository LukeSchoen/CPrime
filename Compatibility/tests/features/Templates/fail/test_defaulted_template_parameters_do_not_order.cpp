// EXPECT_COMPILE_FAIL: 1
struct Value {
  template<class T> static int read(T, int = 0) { return 1; }
  template<class T> static int read(T, short = 0) { return 2; }
};
int main() { return Value::read(3); }
