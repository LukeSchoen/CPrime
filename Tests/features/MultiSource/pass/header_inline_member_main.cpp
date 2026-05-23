#include "test_header_inline_member.h"

int header_inline_member_a(void);

int main(void)
{
  class HeaderThing thing;
  thing.value = 3;
  return header_inline_member_a() == 2 && thing.get() == 3 ? 0 : 1;
}
