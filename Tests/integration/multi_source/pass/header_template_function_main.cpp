#include "test_header_template_function.h"

int header_template_function_a(void);

int main(void)
{
  return header_template_function_a() == 11
         && header_template_identity(int)(13) == 13 ? 0 : 1;
}
