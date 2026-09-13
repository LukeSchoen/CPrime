// EXPECT_EXIT: 0

extern int unavailable(int);

extern inline __attribute__((gnu_inline)) int constant_only(int value)
{
  if (!__builtin_constant_p(value))
    return unavailable(value);
  return 42;
}

int main()
{
  return constant_only(0) != 42;
}
