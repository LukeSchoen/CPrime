// EXPECT_COMPILE_FAIL: 1
// Binding an lvalue to a more cv-qualified reference and passing it by value
// are both exact conversion sequences; neither candidate is better, and no
// partial ordering applies to ordinary functions.
int pick(int) { return 1; }
int pick(int const &) { return 2; }
int main()
{
  int item = 0;
  return pick(item);
}
