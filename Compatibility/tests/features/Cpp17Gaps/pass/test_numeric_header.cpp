// C++17 gap probe: lib_numeric and std_gcd. <numeric> is missing from the runtime.
#include <numeric>

int main() {
  int values[3] = {1, 2, 3};
  if (std::accumulate(values, values + 3, 0) != 6) return 1;
  int counted[3] = {0, 0, 0};
  std::iota(counted, counted + 3, 1);
  return std::accumulate(counted, counted + 3, 0) - 6;
}
