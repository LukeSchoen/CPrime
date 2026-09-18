// C++17 gap probe: lib_stack. <stack> is missing from the runtime.
#include <stack>

int main() {
  std::stack<int> values;
  values.push(1);
  values.push(2);
  if (values.top() != 2) return 1;
  if (values.size() != 2) return 2;
  values.pop();
  return values.top() - 1;
}
