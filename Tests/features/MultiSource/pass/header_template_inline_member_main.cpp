#include "test_header_template_inline_member.h"

int header_template_inline_member_a(void);

int main(void)
{
  HeaderTemplateThing<int> thing;
  thing.value = 7;
  return header_template_inline_member_a() == 5 && thing.get() == 7 ? 0 : 1;
}
