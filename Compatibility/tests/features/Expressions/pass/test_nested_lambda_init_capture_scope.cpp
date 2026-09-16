int main() {
  int value = 7;
  auto outer = [value = value + 1]() {
    auto inner = [value = value + 2]() { return value; };
    return inner();
  };
  auto nested = [inner = [value = 3]() { return value; }]() { return inner(); };
  return outer() != 10 || nested() != 3 || value != 7;
}
