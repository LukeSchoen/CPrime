// EXPECT_EXIT: 0
struct First { virtual ~First() {} };
struct Second { virtual int value() { return 7; } };
struct Derived : First, Second {};

int main() {
  Derived *value = new Derived;
  int result = value->value() == 7 ? 0 : 1;
  delete value;
  return result;
}
