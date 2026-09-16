// EXPECT_EXIT: 0
template<class T> struct Value { int number; };
Value<int> global_value;
template<class T> int read(T n) {
  struct Value { int add(int n) { return n + 2; } };
  Value value;
  ::Value<int> explicit_global;
  explicit_global.number = 7;
  if (explicit_global.number != 7) return 99;
  return value.add(n) + value.add(n);
}
int main() { return read(3) != 10 || read(5L) != 14; }
