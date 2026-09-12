// EXPECT_SOURCES: ["variadic_static_member_other.cpp"]

#include "variadic_static_member.h"

VariadicResult make_variadic_elsewhere(int value);

int main()
{
  VariadicResult local = VariadicFactory::Make(4);
  VariadicResult other = make_variadic_elsewhere(4);
  if (local.value != 4 || other.value != 4) return 1;
  return 0;
}
