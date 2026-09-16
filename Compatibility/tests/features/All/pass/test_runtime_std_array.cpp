#include <array>
#include <stddef.h>

int main()
{
  std::array<int, 4> values;
  values.fill(3);
  values[1] = 5;
  values.at(2) = 7;

  const std::array<int, 4> copy(values);
  if (copy.size() != 4 || copy.empty())
    return 1;
  if (copy.front() != 3 || copy.back() != 3)
    return 2;
  if (copy.data()[1] != 5 || copy[2] != 7)
    return 3;

  int total = 0;
  for (const int* it = copy.begin(); it != copy.end(); ++it)
    total += *it;
  return total == 18 ? 0 : 4;
}
