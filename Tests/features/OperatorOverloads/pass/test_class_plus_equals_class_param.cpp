// EXPECT_EXIT: 0
// EXPECT_STDOUT:
class cpcString
{
public:
  cpcString();
  cpcString(int len);
  cpcString operator+=(cpcString rhs);

public:
  int len;
};

cpcString::cpcString()
{
  this->len = 0;
}

cpcString::cpcString(int len)
{
  this->len = len;
}

cpcString cpcString::operator+=(cpcString rhs)
{
  this->len = this->len + rhs.len;
  return *this;
}

int main(void)
{
  cpcString a(3);
  cpcString b(4);

  a += b;

  if (a.len != 7)
    return 1;
  return 0;
}
