// C++17 gap probe: std_set. <set> is missing from the runtime.
#include <set>

int main() {
  std::set<int> values;
  values.insert(2);
  values.insert(1);
  values.insert(1);
  if (values.size() != 2) return 1;
  return *values.begin() - 1;
}
