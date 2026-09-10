typedef __builtin_va_list va_list;

int sum(int count, ...)
{
  va_list args;
  __builtin_va_start(args, ((const char *)(&count)));
  int total = 0;
  for (int i = 0; i < count; ++i)
    total += __builtin_va_arg(args, int);
  __builtin_va_end(args);
  return total;
}

int main()
{
  return sum(3, 4, 5, 6) == 15 ? 0 : 1;
}
