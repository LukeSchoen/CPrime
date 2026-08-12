// EXPECT_EXIT: 0
// EXPECT_STDOUT:
#include <string.h>

class cpcString
{
public:
  cpcString();
  cpcString(const char *text);
  cpcString Concat(const char *rhs);
  cpcString operator+(const char *rhs);
  int Length();

public:
  int len;
};

cpcString::cpcString()
{
  this->len = 0;
}

cpcString::cpcString(const char *text)
{
  this->len = (int)strlen(text);
}

cpcString cpcString::Concat(const char *rhs)
{
  cpcString out;
  out.len = this->len + (int)strlen(rhs);
  return out;
}

cpcString cpcString::operator+(const char *rhs)
{
  return this->Concat(rhs);
}

int cpcString::Length()
{
  return this->len;
}

int main(void)
{
  cpcString a("abc");
  cpcString b = a + "de";

  if (a.Length() != 3)
    return 1;
  if (b.Length() != 5)
    return 2;
  return 0;
}
