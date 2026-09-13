// EXPECT_SOURCES: ["template_variable_linkage_other.cpp"]
#include "template_variable_linkage.h"

template<int N>
static const int local_value = N;

int main()
{
  if (template_variable_other_value != &external_value<42>) return 1;
  if (external_value<42> != 42) return 2;
  if (template_variable_other_volatile_value != &volatile_value<42>) return 3;
  if (volatile_value<42> != 42) return 4;
  if (local_value<42> == local_value<43>) return 5;
  if (local_value<42> != 42 || local_value<43> != 43) return 6;
  return 0;
}
