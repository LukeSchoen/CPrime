#include <initializer_list>
enum class Value { first = 3, second = 7, third = 11 };
int sum(std::initializer_list<Value> values) {
  int result = 0;
  for (const Value *p = values.begin(); p != values.end(); ++p) result += (int)*p;
  return result;
}
int main() {
  std::initializer_list<Value> values = { Value::first, Value::second, Value::third };
  if (values.size() != 3 || sum(values) != 21) return 1;
  if (sum(std::initializer_list<Value>{ Value::second, Value::first }) != 10) return 2;
  if (sum(values) != 21) return 3;
  std::initializer_list<Value> empty;
  return empty.size() != 0 || empty.begin() != empty.end();
}
