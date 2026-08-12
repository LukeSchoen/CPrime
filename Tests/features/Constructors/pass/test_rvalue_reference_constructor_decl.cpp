// EXPECT_EXIT: 0

class Test
{
public:
  Test();
  Test(Test &&rhs);

public:
  int value;
};

Test::Test()
{
  this->value = 3;
}

Test::Test(Test &&rhs)
{
  this->value = rhs.value + 4;
}

int main(void)
{
  Test a;
  Test b(a);
  return b.value == 7 ? 0 : 1;
}
