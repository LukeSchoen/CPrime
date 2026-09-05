int main() {
  int value = 7;
  auto copy = [value](int add) { return value + add; };
  auto reference = [&](int add) { value += add; return value; };
  if (copy(3) != 10 || reference(5) != 12 || value != 12) return 1;
  if (copy(3) != 10) return 2;
  auto mutableCopy = [value]() mutable { return ++value; };
  if (mutableCopy() != 13 || mutableCopy() != 14 || value != 12) return 3;
  auto floating = [](double x) { double square = x * x; return square + 0.5; };
  if (floating(2.0) != 4.5) return 4;
  auto byte = [](int x) -> unsigned char { return x; };
  if (sizeof(byte(1)) != 1 || byte(259) != 3) return 5;
  int result = [&]() -> int { if (value > 10) return 31; return 2; }();
  return result != 31;
}
