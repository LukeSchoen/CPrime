#include <stddef.h>

static int pointer_is_valid(void *pointer)
{
  return pointer != nullptr;
}

int main()
{
  return pointer_is_valid(alloca(8192)) ? 0 : 1;
}
