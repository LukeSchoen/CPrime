// CL gap probe: lib_list. <list> is missing from the runtime.
#include <list>

int main() {
  std::list<int> values{1, 2};
  values.push_front(0);
  int sum = 0;
  for (int value : values) sum += value;
  return sum - 3;
}
