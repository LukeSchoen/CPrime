int select(float) { return 1; }
int select(double) { return 2; }
int select(long double) { return 3; }
template<class T> int deduced(T value) { return select(value); }
int main() {
  if (select(1.0f) != 1 || select(1.0) != 2 || select(1.0L) != 3) return 1;
  if (deduced(1.0f) != 1 || deduced(1.0) != 2 || deduced(1.0L) != 3) return 2;
  return 0;
}
