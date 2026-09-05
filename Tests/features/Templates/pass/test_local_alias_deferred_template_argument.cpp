template<class T> int read(T p) { return *p; }
int local_alias(int *value) {
  using Pointer = int *;
  Pointer p = value;
  return read(p);
}
int another_scope(double *value) {
  using Pointer = double *;
  Pointer p = value;
  return read(p);
}
int main() {
  int i = 42;
  double d = 17;
  if (local_alias(&i) != 42) return 1;
  if (another_scope(&d) != 17) return 2;
  return 0;
}
