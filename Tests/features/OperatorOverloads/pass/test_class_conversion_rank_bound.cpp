struct Number {
  int value;
  template<class T> Number(T v) : value(int(v)) {}
  operator int() const { return value; }
};
int choose(Number) { return 1; }
int choose(int) { return 2; }
int reverse(int) { return 3; }
int reverse(Number) { return 4; }
int converted(Number n) { return n.value; }
int pair(Number, int) { return 5; }
int pair(int, Number) { return 6; }
int main() {
  Number n(7);
  if (choose(7) != 2 || reverse(n) != 4) return 1;
  if (converted(9) != 9 || pair(1, n) != 6) return 2;
  return 0;
}
