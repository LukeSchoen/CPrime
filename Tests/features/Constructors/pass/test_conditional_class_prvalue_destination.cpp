// EXPECT_EXIT: 0
int alive, destroyed;

struct Value {
  int n;
  const Value *self;
  Value(int input) : n(input), self(this) { ++alive; }
  Value(const Value &) = delete;
  Value(Value &&) = delete;
  ~Value() { --alive; ++destroyed; }
  operator bool() const { return n != 0; }
};

Value make(int n) { return Value(n); }

int exercise(bool flag) {
  {
    Value value = flag ? Value(1) : make(2);
    if (value.n != (flag ? 1 : 2) || value.self != &value || alive != 1) return 1;
  }
  if (alive) return 2;
  {
    const Value &value = flag ? make(3) : Value(4);
    if (value.n != (flag ? 3 : 4) || value.self != &value || alive != 1) return 3;
  }
  if (alive) return 4;
  {
    Value value = (flag ? (false ? Value(5) : make(6)) : Value(7));
    if (value.n != (flag ? 6 : 7) || value.self != &value || alive != 1) return 5;
  }
  {
    Value value = Value(flag ? 1 : 0) ? make(Value(8).n) : make(Value(9).n);
    if (value.n != (flag ? 8 : 9) || value.self != &value || alive != 1) return 6;
  }
  try {
    const Value &value = flag ? Value(10) : throw 11;
    if (value.n != 10 || alive != 1) return 7;
  } catch (int n) {
    if (flag || n != 11 || alive) return 8;
  }
  return alive;
}

int main() {
  if (exercise(false)) return 1;
  if (exercise(true)) return 2;
  return destroyed != 13;
}
