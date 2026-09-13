// EXPECT_EXIT: 0
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

int main()
{
  return make_value().value != 7;
}
