// EXPECT_COMPILE_ARGS: -std=c++17
// Microsoft x64 InterlockedIncrement/InterlockedDecrement return the value the
// operation left in memory.  The vendored headers used to infer it from the
// condition flags of an inline asm template, and the allocator could place the
// address operand and a flag byte in the same register, so the caller saw a
// constant.  The intrinsics now reach the runtime library helpers.
#include <windows.h>

int main()
{
  volatile long counter = 0;

  if (InterlockedIncrement(&counter) != 1)
    return 1;
  if (InterlockedIncrement(&counter) != 2)
    return 2;
  if (InterlockedIncrement(&counter) != 3)
    return 3;
  if (counter != 3)
    return 4;

  if (InterlockedDecrement(&counter) != 2)
    return 5;
  if (InterlockedDecrement(&counter) != 1)
    return 6;
  if (counter != 1)
    return 7;

  if (InterlockedIncrement(&counter) != 2)
    return 8;
  if (InterlockedDecrement(&counter) != 1)
    return 9;

  return 0;
}
