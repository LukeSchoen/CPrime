template<class T> struct Box {
  T value;
  Box(T n) : value(n) {}
  T get() const { return value; }
};

int main() {
  struct Local {
    int value;
    Local() : value(3) {}
  } local;
  int sum = local.value;
  for (int y = 0; y < 2; ++y) sum += y;
  Box<int> box(5);
  sum += box.get();
  for (int y = 0; y < 2; ++y) sum += y;
  if (sum != 10) return 1;

  int active = 7;
  typedef long long Wide;
  {
    int active = 11;
    Box<Wide> inner(13);
    if (active != 11 || inner.get() != 13) return 2;
  }
  Box<short> after(17);
  return active != 7 || after.get() != 17 || local.value != 3;
}
