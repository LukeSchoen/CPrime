#include "variadic_static_member.h"

VariadicResult make_variadic_elsewhere(int value)
{
  return VariadicFactory::Make(value);
}
