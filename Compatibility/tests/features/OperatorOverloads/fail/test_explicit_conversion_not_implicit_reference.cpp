// EXPECT_COMPILE_FAIL: 1
struct Explicit {
  int value;
  explicit operator int&() { return value; }
};
void change(int &value) { ++value; }
int main() { Explicit object; change(object); }
