// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: optional_relational. Comparisons between two engaged optionals
// are wrong: `std::optional<int>{1} < std::optional<int>{2}` answers false.

#include <optional>

int main()
{
  std::optional<int> smaller{1};
  std::optional<int> larger{2};
  if (!(smaller < larger)) return 1;
  if (!(larger > smaller)) return 2;
  if (!(smaller <= larger)) return 3;
  if (!(larger >= smaller)) return 4;
  return 0;
}
