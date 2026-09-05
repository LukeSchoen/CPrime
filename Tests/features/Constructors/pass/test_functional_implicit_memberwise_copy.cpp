int copies = 0;
struct Value {
  int n;
  Value(int x) : n(x) {}
  Value(const Value& x) : n(x.n + 1) { ++copies; }
};
struct Aggregate { Value v; int tag; };
int main() {
  Aggregate a{Value(4), 7};
  copies = 0;
  const Aggregate& b = Aggregate(a);
  return copies != 1 || b.v.n != a.v.n + 1 || b.tag != 7;
}
