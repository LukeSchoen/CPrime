int main() {
  int values[3] = {3, 5, 7};
  auto closure = [values]() mutable { return ++values[1]; };
  values[1] = 11;
  return closure() != 6 || closure() != 7 || values[1] != 11;
}
