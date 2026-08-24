#include <vector>

int main()
{
  std::vector<std::vector<int>> values(
    3, std::vector<int>(2, 15));
  if (values[2][1] != 15)
    return 4;
  values[1][0] = 40;
  if (values.size() != 3 || values[1].size() != 2)
    return 1;
  if (values[1][0] != 40)
    return 2;
  return values[2][1] == 15 ? 0 : 3;
}
