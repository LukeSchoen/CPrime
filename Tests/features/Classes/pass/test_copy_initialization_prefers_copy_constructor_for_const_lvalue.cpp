struct Value
{
  int selected;

  Value() : selected(0) {}
  Value(const Value &) : selected(1) {}
  Value(Value &&) : selected(2) {}
};

Value copy_const_lvalue(const Value &source)
{
  Value result = source;
  return result;
}

int main()
{
  Value source;
  Value result = copy_const_lvalue(source);
  return result.selected == 1 ? 0 : 1;
}

// EXPECT_EXIT: 0
