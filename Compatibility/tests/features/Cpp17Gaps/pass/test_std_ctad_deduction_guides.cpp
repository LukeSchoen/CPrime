// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: std_ctad. The standard deduction guides for std::array,
// std::pair and std::tuple are missing, so the class template argument
// deduction they exist for does not reach the standard containers.

#include <array>
#include <tuple>
#include <utility>

int main()
{
  std::array values{1, 2, 3};
  std::pair pair{1, 2};
  std::tuple triple{1, 2, 3};
  return values[2] == 3 && pair.second == 2 && std::get<2>(triple) == 3 ? 0 : 1;
}
