// EXPECT_EXIT: 0

class Value
{
public:
  explicit Value(int seed);

public:
  int value;
};

Value::Value(int seed)
{
  value = seed + 4;
}

int main(void)
{
  Value v(7);
  return v.value == 11 ? 0 : 1;
}
