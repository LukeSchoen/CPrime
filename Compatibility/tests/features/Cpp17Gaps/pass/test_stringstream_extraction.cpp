// C++17 gap probe: lib_sstream. Extraction must round-trip what insertion wrote.
#include <sstream>

int main() {
  std::stringstream stream;
  stream << 7;
  int value = 0;
  stream >> value;
  if (value != 7) return 1;

  std::istringstream input("9");
  int other = 0;
  input >> other;
  return other == 9 ? 0 : 2;
}
