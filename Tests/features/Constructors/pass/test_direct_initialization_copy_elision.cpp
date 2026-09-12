// EXPECT_EXIT: 0
int live, copies;

struct Value
{
  Value() { ++live; }
  Value(const Value &) { ++copies; ++live; }
  ~Value() { --live; }
};

Value make()
{
  return Value();
}

int main()
{
  Value value(make());
  if (copies) return 1;
  return live != 1;
}
