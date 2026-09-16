// GCC's same-function jump builtins must round-trip through their five-pointer
// save area.  The buffer is exactly void *[5], so the lowering cannot wrap the
// larger CRT jmp_buf, and a resumed frame has to keep the caller's locals.

int main()
{
  void *buffer[5] = { nullptr };
  int jumps = 0;
  int value = __builtin_setjmp (buffer);

  if (value == 0)
  {
    ++jumps;
    __builtin_longjmp (buffer, 1);
    return 1;
  }

  if (value != 1)
    return 2;
  if (jumps != 1)
    return 3;
  return 0;
}
