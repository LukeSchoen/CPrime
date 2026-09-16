// EXPECT_COMPILE_ONLY: 1

void fn1(int i = sizeof(i)) {}
void fn2(int i, int = sizeof(i)) {}

void use() {
  fn1();
  fn2(42);
}
