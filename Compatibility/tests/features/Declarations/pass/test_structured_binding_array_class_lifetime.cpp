// EXPECT_COMPILE_ARGS: -std=c++17
int live, copies;
struct Value {
  int n;
  Value(int v) : n(v) { ++live; }
  Value(const Value &v) : n(v.n) { ++live; ++copies; }
  ~Value() { --live; }
};
int main() {
  {
    Value values[2] = {4, 9};
    {
      auto [a, b] = values;
      if (live != 4 || copies != 2 || a.n != 4 || b.n != 9) return 1;
      a.n = 8;
      if (values[0].n != 4) return 2;
    }
    if (live != 2) return 3;
    auto &[a, b] = values;
    a.n = 7;
    if (values[0].n != 7 || &b != &values[1]) return 4;
  }
  return live;
}
