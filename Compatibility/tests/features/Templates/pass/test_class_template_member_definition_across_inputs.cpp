// EXPECT_SOURCES: ["class_template_member_other.cpp"]

#include "class_template_member_link.h"

int linked_sum_elsewhere();

int main()
{
  LinkedMembers<int> value = {{1, 2, 3, 4}};
  if (value.Sum() != 3) return 1;
  if (linked_sum_elsewhere() != 3) return 2;
  return 0;
}
