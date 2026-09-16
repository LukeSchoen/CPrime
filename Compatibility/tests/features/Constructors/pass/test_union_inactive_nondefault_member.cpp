int live;
struct Value {
  Value(int) { ++live; }
  ~Value() { --live; }
};
union Storage {
  char empty;
  Value value;
  Storage() : empty() {}
  ~Storage() {}
};
int main() {
  { Storage storage; if (live) return 1; }
  return live;
}
