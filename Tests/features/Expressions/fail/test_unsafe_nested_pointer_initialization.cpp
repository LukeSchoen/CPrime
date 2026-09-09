// EXPECT_COMPILE_FAIL: 1
int main() {
  char* pointer = 0;
  char** outer = &pointer;
  const char* const** unsafe = &outer;
}
