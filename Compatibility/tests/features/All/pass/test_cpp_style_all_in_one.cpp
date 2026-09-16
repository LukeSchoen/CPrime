// EXPECT_EXIT: 0

#include "list.h"
#include "list.c"

int main(void)
{
  List<int> numbers;
  for (int i = 0; i < 1000000; i = i + 1)
    numbers.push(i * 3);

  if (numbers.size() != 1000000)
    return 1;
  if (numbers.get(0) != 0)
    return 2;
  if (numbers.get(1) != 3)
    return 3;
  if (numbers.get(8) != 24)
    return 4;
  if (numbers.get(999999) != 2999997)
    return 5;
  return 0;
}
