namespace { int counter = 4; int increment() { return ++counter; } }
namespace { int twice() { return increment() * 2; } }
namespace Outer {
  int base = 7;
  namespace { struct Value { int n; }; int counter = 30; }
  namespace { int read() { Value v = { base + counter }; return v.n; } }
  namespace Inner { int inherited() { return read(); } }
}
int main() {
  if (twice() != 10 || counter != 5) return 1;
  if (Outer::read() != 37 || Outer::Inner::inherited() != 37) return 2;
  Outer::Value value = { 9 };
  return value.n != 9;
}
