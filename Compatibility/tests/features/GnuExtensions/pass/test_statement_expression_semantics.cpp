// Statement-expression semantics.

static int check_array_decay()
{
  int a[128];
  return sizeof(({ a; })) == sizeof(int *) ? 0 : 1;
}

struct Value
{
  int i;

  Value(int j) : i(j) {}
  Value(const Value &j) : i(j.i) {}
  Value &operator=(const Value &j)
  {
    i = j.i;
    return *this;
  }
};

static Value make(bool take_first)
{
  return ({ take_first ? Value(1) : Value(0); });
}

static int check_value_return()
{
  return make(true).i - 1;
}

static int check_copy_out_of_block()
{
  Value b = ({ Value a(1); a; });
  return b.i - 1;
}

static int calls;

static int record()
{
  return ++calls;
}

static int check_boolean_short_circuit()
{
  calls = 0;
  int value = 0;
  bool flag = value && (record(), ({ static int local = record(); local; })) != 0;
  if (flag || calls != 0) { return 1; }

  value = 1;
  flag = value && (record(), ({ static int local = record(); local; })) != 0;
  return flag && calls == 2 ? 0 : 1;
}

int main()
{
  if (int code = check_array_decay()) { return code; }
  if (int code = check_value_return()) { return code; }
  if (int code = check_copy_out_of_block()) { return code; }
  return check_boolean_short_circuit();
}
