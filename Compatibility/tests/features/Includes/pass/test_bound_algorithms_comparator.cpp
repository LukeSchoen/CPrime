// lower_bound and upper_bound accept the C++ comparator overload, which a
// binary search needs whenever the container is ordered by something other
// than operator<.

#include <algorithm>
#include <vector>

static bool descending(int left, int right) { return left > right; }

int main()
{
  std::vector<int> falling{9, 7, 5, 3, 1};
  if (*std::lower_bound(falling.begin(), falling.end(), 5, descending) != 5) return 1;
  if (*std::upper_bound(falling.begin(), falling.end(), 5, descending) != 3) return 2;
  if (std::lower_bound(falling.begin(), falling.end(), 8, descending) != falling.begin() + 1) return 3;
  if (std::upper_bound(falling.begin(), falling.end(), 8, descending) != falling.begin() + 1) return 4;

  std::vector<int> rising{1, 3, 5, 7, 9};
  auto ascending = [](int left, int right) { return left < right; };
  if (*std::lower_bound(rising.begin(), rising.end(), 5, ascending) != 5) return 5;
  if (*std::upper_bound(rising.begin(), rising.end(), 5, ascending) != 7) return 6;
  if (*std::lower_bound(rising.begin(), rising.end(), 6, ascending) != 7) return 7;
  if (*std::upper_bound(rising.begin(), rising.end(), 6, ascending) != 7) return 8;
  return 0;
}
