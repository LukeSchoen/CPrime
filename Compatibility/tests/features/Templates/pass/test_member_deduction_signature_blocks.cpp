template<class Owner> struct Calculator {
  int offset;
  template<class T> Calculator(T value) : offset(int(value)) {}
  template<class T> auto add(T value, int extra = [] { return 3; }())
      -> decltype(value + extra) {
    int result = offset;
    { result += int(value); }
    const char *braces = "{)}";
    return result + extra + (braces[0] != '{');
  }
  template<class T> int operator()(T value) { return add(value); }
};
int main() {
  Calculator<int> c(4);
  return c.add(5) != 12 || c.add(6, 2) != 12 || c(7) != 14;
}
