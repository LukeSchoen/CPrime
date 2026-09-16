// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: ctad_parenthesized. The retained CTAD case covers a braced
// initializer; deduction from a parenthesized one must work as well.
template<class T> struct Box {
  T value;
  Box(T v) : value(v) {}
};

int main() {
  Box box(3);
  if (box.value != 3) return 1;
  Box other(2.5);
  return other.value == 2.5 ? 0 : 2;
}
