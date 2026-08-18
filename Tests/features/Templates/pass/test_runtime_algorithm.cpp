#include <stdlib.h>
#include <algorithm>

int main()
{
  int values[] = { 4, 1, 3, 2, 2 };
  std::sort(values, values + 5);
  if (values[0] != 1 || values[4] != 4) return 1;
  if (std::lower_bound(values, values + 5, 2) - values != 1) return 2;
  if (std::upper_bound(values, values + 5, 2) - values != 3) return 3;
  std::replace(values, values + 5, 2, 7);
  return values[1] == 7 && values[2] == 7 ? 0 : 4;
}
