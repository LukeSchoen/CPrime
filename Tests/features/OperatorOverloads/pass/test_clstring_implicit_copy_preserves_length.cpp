// EXPECT_EXIT: 0
// EXPECT_STDOUT:
#include "clString.h"
#include "clString.c"

static int check_copy(clString value)
{
  if (value.m_data[0] != 'a')
    return 1;
  if (value.m_data[1] != 'b')
    return 2;
  if (value.m_data[2] != 'c')
    return 3;
  if (value.m_data[3] != 0)
    return 4;
  if (value.Length() != 3)
    return 5;
  return 0;
}

int main(void)
{
  clString original("abc");
  clString copy = original;
  int by_value = check_copy(original);

  if (original.Length() != 3)
    return 10;
  if (copy.m_data[0] != 'a')
    return 11;
  if (copy.m_data[1] != 'b')
    return 12;
  if (copy.m_data[2] != 'c')
    return 13;
  if (copy.m_data[3] != 0)
    return 14;
  if (copy.Length() != 3)
    return 15;
  if (by_value)
    return 20 + by_value;
  return 0;
}
