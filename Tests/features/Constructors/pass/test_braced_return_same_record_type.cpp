// EXPECT_COMPILE_ARGS: -Werror

struct Value
{
  int number;
};

Value MakeOne()
{
  return Value{1};
}

Value MakeZero()
{
  return Value{};
}

int main()
{
  if (MakeOne().number != 1) return 1;
  if (MakeZero().number != 0) return 2;
  return 0;
}
