// <random> provides a non-deterministic random_device, and <algorithm>'s
// shuffle accepts the uniform random bit generator overload.

#include <algorithm>
#include <random>
#include <vector>

int main()
{
  std::random_device device;
  std::mt19937 generator(device());

  std::vector<int> values{1, 2, 3, 4, 5, 6, 7, 8};
  std::shuffle(values.begin(), values.end(), generator);

  int sum = 0;
  bool seen[9] = {false, false, false, false, false, false, false, false, false};
  for (int i = 0; i < 8; ++i)
  {
    sum += values[i];
    if (values[i] < 1 || values[i] > 8 || seen[values[i]]) return 1;
    seen[values[i]] = true;
  }
  if (sum != 36) return 2;
  if (device.min() != 0 || device.max() != 0xffffffffu) return 3;
  return 0;
}
