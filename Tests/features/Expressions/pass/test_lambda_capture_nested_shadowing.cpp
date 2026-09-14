int main() {
  int value = 2;
  auto closure = [value](int amount) {
    if (value != 2) return 0;
    { int value = 5; if (value != 5) return 0; }
    { if (value != 2) return 0; }
    return value + amount;
  };
  auto reference_shadow = [value] {
    int other = 7;
    { int &value = other; if (value != 7) return 0; }
    return value;
  };
  return closure(3) != 5 || reference_shadow() != 2;
}
