#include <stddef.h>

typedef unsigned short uint16_t;

int choose(const wchar_t *p);
int choose(const uint16_t *p);

int choose(const wchar_t *p)
{
  (void)p;
  return 1;
}

int choose(const uint16_t *p)
{
  (void)p;
  return 2;
}

int main()
{
  wchar_t w = 0;
  uint16_t u = 0;
  return choose(&w) == 1 && choose(&u) == 2 ? 0 : 1;
}
