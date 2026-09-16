// EXPECT_EXIT: 0
struct Value {
  int number;
  template<class T> void operator=(T value) { number = 1; }
};
template<> void Value::operator=<int>(int value) { number = value + 2; }
template<class T> struct Outer { struct Inner { int operator()(); }; };
template<> int Outer<int>::Inner::operator()() { return 9; }
int main() { Value value; value = 5; Outer<int>::Inner inner;
  return value.number != 7 || inner() != 9; }
