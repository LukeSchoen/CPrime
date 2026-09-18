// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: shared_ptr_array. std::shared_ptr<T[]> does not accept the
// array form introduced with C++17.

#include <memory>

int main()
{
  std::shared_ptr<int[]> values(new int[3]{1, 2, 3});
  values[2] = 4;
  return values[0] == 1 && values[2] == 4 ? 0 : 1;
}
