#include "test_header_template_function.h"

int header_template_function_a(void)
{
  return header_template_identity(int)(11);
}
