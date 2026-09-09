template<class Key, class Value> struct Map {
  int add(const Key&, const Value&) { return 1; }
  int add(const Key&, Value&&) { return 2; }
  int add(Key&&, const Value&) { return 3; }
  int add(Key&&, Value&&) { return 4; }
};
enum Key { first = 1 };
int select(const int&) { return 1; }
int select(int&&) { return 2; }
int main() {
  struct Value { int field; };
  Map<int, Value> map;
  if (map.add(first, Value{2}) != 4) return 1;
  int key = 1;
  Value value{2};
  if (map.add(key, value) != 1) return 2;
  if (map.add(key, Value{2}) != 2) return 3;
  if (map.add(first, value) != 3) return 4;
  Key converted = first;
  if (map.add(converted, value) != 3) return 5;
  if (select(converted) != 2) return 6;
  return 0;
}
