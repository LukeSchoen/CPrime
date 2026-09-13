// EXPECT_COMPILE_FAIL: 1

extern int unavailable(int);

extern inline __attribute__((gnu_inline)) int constant_only(int value)
{
  if (!__builtin_constant_p(value))
    return unavailable(value);
  return 42;
}

int main()
{
  int value = 0;
  return constant_only(value);
}
