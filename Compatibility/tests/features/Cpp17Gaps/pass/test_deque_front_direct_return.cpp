// C++17 gap probe: std_deque_ops. A local deque must still hold its element when
// front()/back() is the returned expression; the direct-return form currently
// reads freed storage, while a named local copy reads the correct value.
#include <deque>

int first() {
  std::deque<int> values;
  values.push_back(42);
  return values.front();
}

int last() {
  std::deque<int> values;
  values.push_back(7);
  return values.back();
}

int main() {
  if (first() != 42) return 1;
  if (last() != 7) return 2;
  return 0;
}
