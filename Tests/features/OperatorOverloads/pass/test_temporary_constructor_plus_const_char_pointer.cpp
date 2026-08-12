// EXPECT_EXIT: 0
// EXPECT_STDOUT:
#include <string.h>

class String
{
public:
  String();
  String(const char *text);
  String operator+(const char *rhs);
  int Length();

public:
  int len;
};

String::String()
{
  this->len = 0;
}

String::String(const char *text)
{
  this->len = (int)strlen(text);
}

String String::operator+(const char *rhs)
{
  String out;
  out.len = this->len + (int)strlen(rhs);
  return out;
}

int String::Length()
{
  return this->len;
}

int main(void)
{
  const char *base = "data/";
  String path = String(base) + "file.txt";

  return path.Length() == 13 ? 0 : 1;
}
