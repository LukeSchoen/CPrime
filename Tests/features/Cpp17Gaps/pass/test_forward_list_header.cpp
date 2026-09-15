// CL gap probe: lib_forward_list. <forward_list> is missing from the runtime.
#include <forward_list>

int main() {
  std::forward_list<int> values{1, 2};
  values.push_front(0);
  return values.front() == 0 ? 0 : 1;
}
