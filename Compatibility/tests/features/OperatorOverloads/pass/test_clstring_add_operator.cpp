// EXPECT_EXIT: 0
// EXPECT_STDOUT:
#include "clString.h"
#include "clString.c"

int main(void)
{
  clString a("abc");
  clString b("de");
  clString c = a + b;

  if (a.Length() != 3)
    return 1;
  if (a.at(0) != 'a')
    return 2;
  if (a.at(1) != 'b')
    return 3;
  if (a.at(2) != 'c')
    return 4;
  if (a.at(3) != 0)
    return 5;
  if (b.Length() != 2)
    return 6;
  if (c.Length() != 5)
    return 7;
  if (c.at(0) != 'a')
    return 8;
  if (c.at(1) != 'b')
    return 9;
  if (c.at(2) != 'c')
    return 10;
  if (c.at(3) != 'd')
    return 11;
  if (c.at(4) != 'e')
    return 12;
  return c.at(5) == 0 ? 0 : 13;
}
