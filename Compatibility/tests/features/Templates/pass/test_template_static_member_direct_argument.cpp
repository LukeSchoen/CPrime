template<class T> struct Value {
  typedef int type;
  static const unsigned flag = 7;
  unsigned stored;
  Value(unsigned value) : stored(value) {}
};
int declared(int value) { return value + 1; }
int main() {
  Value<char> value(Value<char>::flag);
  int declared(Value<char>::type);
  return value.stored != 7 || declared(3) != 4;
}
