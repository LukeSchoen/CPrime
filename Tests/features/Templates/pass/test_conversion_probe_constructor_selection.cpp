struct Value {
  int selected;
  Value(int) : selected(1) {}
  template<class T> Value(T) : selected(2) {}
};
int consume(Value value) { return value.selected; }
int main() { return consume(1) != 1 || consume(1.0) != 2; }
