struct Number {
  int value;
  int calls;
  Number() : value(3), calls(0) {}
  int operator+(int right) { ++calls; return value + right; }
  Number &operator<<(int right) { value += right; ++calls; return *this; }
};
int main() {
  Number n;
  if (n + 4 != 7 || n.calls != 1) return 1;
  n << 5 << 7;
  if (n.value != 15 || n.calls != 3) return 2;
  return 0;
}
