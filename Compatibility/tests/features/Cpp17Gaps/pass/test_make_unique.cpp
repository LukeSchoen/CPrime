// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_make_unique. std::make_shared works; std::make_unique does
// not exist.
#include <memory>

int main() {
  std::unique_ptr<int> scalar = std::make_unique<int>(1);
  if (*scalar != 1) return 1;
  std::unique_ptr<int[]> array = std::make_unique<int[]>(3);
  array[0] = 2;
  array[2] = 4;
  return array[0] + array[2] - 6;
}
