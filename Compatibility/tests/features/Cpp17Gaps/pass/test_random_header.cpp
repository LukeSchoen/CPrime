// C++17 gap probe: lib_random. <random> is missing from the runtime.
#include <random>

int main() {
  std::mt19937 engine(1);
  unsigned first = engine();
  unsigned second = engine();
  return first != second ? 0 : 1;
}
