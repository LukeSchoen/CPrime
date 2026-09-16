template<class T> int apply(T value) {
  auto transform = [=](int extra) { return value + extra; };
  auto composed = [&]() { return transform(3) + transform(5); };
  return composed();
}
int main() {
  if (apply(7) != 22 || apply(11LL) != 30) return 1;
  int value = 3;
  auto change = [&value]() { value += 4; };
  change();
  if (value != 7) return 2;
  auto first = []() { return 1; };
  auto second = []() { return 2; };
  return first() != 1 || second() != 2;
}
