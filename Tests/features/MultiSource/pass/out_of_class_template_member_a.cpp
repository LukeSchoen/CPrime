#include "test_out_of_class_template_member.h"

int out_of_class_template_member_a(void)
{
  OutOfClassBox<int> box;
  box.Set(41);
  box.Clear();
  return box.Get();
}
