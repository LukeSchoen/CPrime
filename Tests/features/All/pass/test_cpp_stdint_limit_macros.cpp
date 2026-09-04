#include <stdint.h>

int main()
{
  uint32_t value = UINT32_MAX;
  return value == 0xffffffffU ? 0 : 1;
}
