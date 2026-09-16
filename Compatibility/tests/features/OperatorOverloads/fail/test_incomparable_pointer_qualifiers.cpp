// EXPECT_COMPILE_FAIL: 1
int select(const int*);
int select(volatile int*);
int main() {
  int value = 0;
  return select(&value);
}
