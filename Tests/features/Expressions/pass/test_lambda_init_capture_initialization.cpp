int calls;
int next_value() { return ++calls; }
int main() {
  int value = 7;
  auto copied = [value = next_value()]() mutable { return ++value; };
  if (calls != 1 || value != 7 || copied() != 2 || copied() != 3) return 1;
  auto reference = [&alias = value]() { alias += 5; };
  reference();
  auto generic = [initial = value](auto amount) { return initial + amount; };
  value = 19;
  return value != 19 || generic(3) != 15 || calls != 1;
}
