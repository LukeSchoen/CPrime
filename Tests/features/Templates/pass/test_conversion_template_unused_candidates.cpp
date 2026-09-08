struct Discard {
  template<class T> operator T() { T incomplete[]; }
};
template<class T> struct Value {
  template<class U> struct Inner {};
  Value() {}
  Value(const Value&) {}
  Value(Inner<T>) {}
  template<class U> operator Inner<U>();
};
Value<int> make() { return Value<int>(); }
void consume(Value<int>) {}
int main() {
  Discard discard;
  (void)discard;
  consume(make());
  return 0;
}
