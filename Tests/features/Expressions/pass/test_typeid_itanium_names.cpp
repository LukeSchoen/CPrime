// EXPECT_EXIT: 0
#include <string.h>
#include <typeinfo>

struct S {};
typedef S volatile T[4];
static T t[3];

int local_names()
{
  struct A {};
  struct B {};
  if (strcmp(typeid(A).name(), "Z11local_namesvE1A")) return 1;
  if (strcmp(typeid(B).name(), "Z11local_namesvE1B")) return 2;
  return 0;
}

int main()
{
  if (strcmp(typeid(S).name(), "1S")) return 3;
  if (strcmp(typeid(t).name(), "A3_A4_1S")) return 4;
  return local_names();
}
