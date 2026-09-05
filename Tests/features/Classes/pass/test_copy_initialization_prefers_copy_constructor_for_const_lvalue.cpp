static int wrong_constructor;

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
  // Check the copy before the optional NRVO/implicit move at the return.
  if (result.selected != 1) ++wrong_constructor;
  return result;
}

int main()
{
  Value source;
  Value result = copy_const_lvalue(source);
  return wrong_constructor || (result.selected != 1 && result.selected != 2);
}

// EXPECT_EXIT: 0
