// EXPECT_EXIT: 0
struct Value { int number; };
namespace Operations {
  template<class T> int operator<<(Value value, T amount) { return value.number + amount; }
  int operator+(Value value, int amount) { return value.number + amount; }
}
using Operations::operator+;
template<class T> int read(T amount) {
  using Operations::operator<<;
  Value value = { 4 };
  return value << amount;
}
int main() { Value value = { 5 }; return read(3) != 7 || value + 2 != 7; }
