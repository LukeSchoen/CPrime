// EXPECT_SOURCES: ["deduced_specialization_link_other.cpp"]
#include "deduced_specialization_link.h"
int main() {
  special_link::Value value; value.bias = 5;
  if (value.get(3) != 8 || value.get('x') != 6) return 1;
  if (special_link::calculate(4) != 8) return 2;
  if (special_link::calculate('x') != 2) return 3;
  return 0;
}
