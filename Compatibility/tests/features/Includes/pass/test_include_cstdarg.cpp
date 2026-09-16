#include <cstdarg>

int sum(int count, ...)
{
  va_list args;
  int result = 0;
  va_start(args, count);
  for (int i = 0; i < count; ++i)
    result += va_arg(args, int);
  va_end(args);
  return result;
}

int main()
{
  return sum(4, 1, 2, 3, 4) == 10 ? 0 : 1;
}
