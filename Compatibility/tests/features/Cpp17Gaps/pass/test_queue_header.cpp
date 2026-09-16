// CL gap probe: std_queue. <queue> is present but does not declare std::queue.
#include <queue>

int main() {
  std::queue<int> values;
  values.push(1);
  values.push(2);
  if (values.front() != 1) return 1;
  if (values.back() != 2) return 2;
  values.pop();
  return values.front() - 2;
}
