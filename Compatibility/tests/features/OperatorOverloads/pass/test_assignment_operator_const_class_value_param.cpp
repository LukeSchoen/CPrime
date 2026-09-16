// EXPECT_EXIT: 0
// EXPECT_STDOUT:
class cpcString
{
public:
  cpcString();
  cpcString(int value);
  cpcString operator=(const cpcString rhs);

public:
  int value;
  int assigned;
};

cpcString::cpcString()
{
  this->value = 0;
  this->assigned = 0;
}

cpcString::cpcString(int value)
{
  this->value = value;
  this->assigned = 0;
}

cpcString cpcString::operator=(const cpcString rhs)
{
  this->value = rhs.value + 1;
  this->assigned = this->assigned + 1;
  return *this;
}

int main(void)
{
  cpcString a(41);
  cpcString b;
  cpcString c;

  b = a;
  c = b;

  if (b.value != 42)
    return 1;
  if (b.assigned != 1)
    return 2;
  if (c.value != 43)
    return 3;
  if (c.assigned != 1)
    return 4;
  return 0;
}
