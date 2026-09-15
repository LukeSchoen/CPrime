// CL gap probe: std_functional_bind. std::function with a lambda works; the
// result of std::bind must be callable too.
#include <functional>

int add(int left, int right) { return left + right; }

int main() {
  auto sum = std::bind(add, 1, 2);
  return sum() - 3;
}
