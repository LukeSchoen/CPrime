// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
struct Value
{
  int value;
  Value(int input) : value(input) {}
  Value(const Value &) = delete;
  Value(Value &&) = delete;
};

Value make_value()
{
  return Value(7);
}

int live, copies;
struct Counted
{
  Counted() { ++live; }
  Counted(const Counted &) { ++copies; ++live; }
  ~Counted() { --live; }
};

Counted make_counted() { return Counted(); }

struct Aggregate { int number; };
Aggregate make_one() { return Aggregate{1}; }
Aggregate make_zero() { return Aggregate{}; }

int main()
{
  if (make_value().value != 7) return 1;
  {
    Counted paren(make_counted());
    if (copies || live != 1) return 2;
  }
  if (live || copies) return 4;
  return make_one().number != 1 || make_zero().number != 0;
}
