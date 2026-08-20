#include "test_out_of_class_template_member.h"

int out_of_class_template_member_a(void);

int main(void)
{
  OutOfClassBox<int> box2;
  box2.Set(7);
  int v = box2.Get();
  OutOfClassBox<char> box;
  box.Set('x');
  char c = box.Get();
  int a = out_of_class_template_member_a();
  return c == 'x' && a == 0 && v == 7 ? 0 : 1;
}
