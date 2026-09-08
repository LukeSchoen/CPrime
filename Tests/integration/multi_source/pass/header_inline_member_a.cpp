#include "test_header_inline_member.h"

int header_inline_member_a(void)
{
  class HeaderThing thing;
  thing.value = 2;
  return thing.get();
}
