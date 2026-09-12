#include "class_template_member_link.h"

int linked_sum_elsewhere()
{
  LinkedMembers<int> value = {{1, 2, 3, 4}};
  return value.Sum();
}
