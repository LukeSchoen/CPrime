#include "test_header_template_inline_member.h"

int header_template_inline_member_a(void)
{
  HeaderTemplateThing<int> thing;
  thing.value = 5;
  return thing.get();
}
