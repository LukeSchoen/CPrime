struct Value { int n; };
struct Cache {
  mutable Value value;
  mutable int calls;
  Value& get() const { ++calls; return value; }
};
int main() {
  const Cache c = {{4},0};
  c.get().n = 9;
  c.value.n += 2;
  return c.get().n != 11 || c.calls != 2;
}
