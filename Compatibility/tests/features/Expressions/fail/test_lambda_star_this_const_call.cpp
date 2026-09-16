// EXPECT_COMPILE_FAIL: 1
struct Value {
  int number;
  void test() { auto invalid = [*this] { ++number; }; invalid(); }
};
int main() { Value value{1}; value.test(); }
