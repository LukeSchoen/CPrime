// CL gap probe: lib_vector. push_back/emplace_back/range-for work; the
// initializer_list constructor does not.
#include <vector>

int main() {
  std::vector<int> values{1, 2, 3};
  if (values.size() != 3) return 1;
  if (values[0] != 1 || values[2] != 3) return 2;
  int sum = 0;
  for (int value : values) sum += value;
  return sum - 6;
}
