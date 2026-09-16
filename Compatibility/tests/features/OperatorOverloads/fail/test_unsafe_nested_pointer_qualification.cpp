// EXPECT_COMPILE_FAIL: 1
void consume(const char**);
int main() {
  char* pointer = 0;
  consume(&pointer);
}
