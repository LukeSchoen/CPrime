static int difference(int a, int b) {
  int values[3] = {a, b, a + b};
  int *left = &values[0], *right = &values[2];
  if (right - left != 2) return 99;
  return (values[2] - values[0]) - (a - b);
}
int main(void) {
  return difference(11, 7) != 3 || difference(-11, 7) != 25;
}
