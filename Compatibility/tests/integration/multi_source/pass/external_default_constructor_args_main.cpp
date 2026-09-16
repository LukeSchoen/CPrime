#include "external_default_constructor_args.h"

int main()
{
  DefaultCameraLike value;
  return value.value == 42 ? 0 : 1;
}
