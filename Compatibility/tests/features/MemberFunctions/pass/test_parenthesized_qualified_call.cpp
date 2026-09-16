// EXPECT_EXIT: 0
struct Base { virtual int read() { return 1; } };
struct Value : Base { int read() { return 2; } };
int main() {
  Value value;
  Base *pointer = &value;
  return (pointer->Base::read)() != 1 || ((pointer->read))() != 2;
}
