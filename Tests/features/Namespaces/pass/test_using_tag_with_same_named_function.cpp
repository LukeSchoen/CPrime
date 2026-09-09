// EXPECT_EXIT: 0
namespace Types { union Value { int number; }; }
int Value(int number) { return number + 2; }
using Types::Value;
namespace Hidden {
  struct Item { int number; Item(int n) : number(n) {} };
  int Item(int n) { return n + 1; }
  typedef struct Item Alias;
}
int main() {
  union Value value;
  value.number = 5;
  Types::Value *same = &value;
  return Value(same->number) != 7 || Hidden::Alias(9).number != 9;
}
