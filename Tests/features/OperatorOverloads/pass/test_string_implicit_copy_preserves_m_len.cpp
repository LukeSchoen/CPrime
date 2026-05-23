// EXPECT_EXIT: 0
// EXPECT_STDOUT:
#include <stdlib.h>
#include <string.h>

class String
{
public:
  String();
  String(const char *text);
  int Length();

public:
  char *m_data;
  int m_len;
};

String::String()
{
  this->m_len = 0;
}

String::String(const char *text)
{
  this->m_len = (int)strlen(text);
  this->m_data = (char *)malloc((size_t)this->m_len + 1u);
  memcpy(this->m_data, text, (size_t)this->m_len + 1u);
}

int String::Length()
{
  return this->m_len;
}

static int check_value_copy(String value)
{
  if (value.m_data[0] != 'h')
    return 1;
  if (value.m_data[1] != 'i')
    return 2;
  if (value.m_data[2] != 0)
    return 3;
  if (value.m_len != 2)
    return 4;
  if (value.Length() != 2)
    return 5;
  return 0;
}

static String make_string(void)
{
  String value("hi");
  return value;
}

int main(void)
{
  String original("hi");
  String copy = original;
  String returned = make_string();
  int value_copy = check_value_copy(original);

  if (copy.m_data[0] != 'h')
    return 10;
  if (copy.m_data[1] != 'i')
    return 11;
  if (copy.m_data[2] != 0)
    return 12;
  if (copy.m_len != 2)
    return 13;
  if (copy.Length() != 2)
    return 14;
  if (returned.m_data[0] != 'h')
    return 15;
  if (returned.m_data[1] != 'i')
    return 16;
  if (returned.m_data[2] != 0)
    return 17;
  if (returned.m_len != 2)
    return 18;
  if (returned.Length() != 2)
    return 19;
  if (value_copy)
    return 20 + value_copy;
  return 0;
}
