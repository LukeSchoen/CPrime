// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: algorithm_cpp17. <numeric> lacks reduce and the parallel scan
// family, <algorithm> lacks for_each_n, sample and the searcher overloads.

#include <algorithm>
#include <numeric>
#include <vector>

int main()
{
  std::vector<int> values{1, 2, 3, 4};
  if (std::reduce(values.begin(), values.end(), 0) != 10) return 1;

  int visited = 0;
  std::for_each_n(values.begin(), 2, [&visited](int) { ++visited; });

  std::vector<int> needle{2, 3};
  auto found = std::search(values.begin(), values.end(),
                           std::default_searcher<const int *>(needle.begin(), needle.end()));
  return visited == 2 && found == values.begin() + 1 ? 0 : 2;
}
