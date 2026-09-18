// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: initializer_list_pair. Naming
// std::initializer_list<std::pair<int, int>> reports "initializer_list element
// has incomplete type", which also stops std::vector<std::pair<...>> braced
// construction.

#include <initializer_list>
#include <utility>

int main()
{
  std::initializer_list<std::pair<int, int>> pairs{{1, 2}};
  return pairs.begin()->second == 2 ? 0 : 1;
}
