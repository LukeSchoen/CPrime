// EXPECT_EXIT: 0

class Value
{
public:
  Value(int first, int second = 20, int third = 300);
  int sum;
};

Value::Value(int first, int second, int third)
{
  sum = first + second + third;
}

int main()
{
  Value value(4);
  return value.sum == 324 ? 0 : 1;
}
