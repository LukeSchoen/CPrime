// EXPECT_COMPILE_FAIL: 1
int select(const int*, const volatile int*);
int select(const volatile int*, volatile int*);
int main() {
  int value = 0;
  return select(&value, &value);
}
