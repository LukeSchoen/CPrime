#include "external_lifecycle.h"

extern int external_lifecycle_destroyed;

int main(void)
{
  {
    ExternalLifecycle value;
    if (value.value != 9)
      return 1;
  }
  return external_lifecycle_destroyed == 9 ? 0 : 2;
}
