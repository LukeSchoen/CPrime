// EXPECT_COMPILE_ONLY: 1

#include <stddef.h>

void *operator new(size_t, int *);
void *operator new(size_t, void *);

int *pointer = 0;

void use()
{
  new (pointer) int *;
  new (&pointer) int *;
  new (pointer) int *;
}
