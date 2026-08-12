// EXPECT_EXIT: 0

class Test
{
public:
  Test();
  Test(int value);
  Test& operator=(const Test &rhs);

public:
  int value;
  int assigned;
};

Test::Test()
{
  this->value = 0;
  this->assigned = 0;
}

Test::Test(int value)
{
  this->value = value;
  this->assigned = 0;
}

Test& Test::operator=(const Test &rhs)
{
  this->value = rhs.value;
  this->assigned = this->assigned + 1;
  return *this;
}

int main(void)
{
  Test a(42);
  Test b;
  return 0;
}
