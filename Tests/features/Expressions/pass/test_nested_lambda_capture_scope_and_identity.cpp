template<class T> int nestedType(T value) {
  auto outer = [value]() {
    return [value]() { return value + 2; };
  };
  auto inner = outer();
  if (sizeof(inner()) != sizeof(T)) return 1;
  return inner() != value + 2;
}

int main() {
  int value = 7;
  auto copy = [value]() {
    auto inner = [value]() { return value; };
    return inner();
  };
  auto reference = [&value]() {
    auto inner = [&value]() { return ++value; };
    return inner();
  };
  if (copy() != 7 || reference() != 8 || value != 8) return 1;

  auto outer = [value]() { return [value]() { return value + 3; }; };
  auto returned = outer();
  value = 99;
  if (returned() != 11) return 2;

  auto captureless = []() { return []() { return 31; }; };
  auto result = captureless();
  if (result() != 31) return 3;
  return nestedType(11) || nestedType(17LL);
}
