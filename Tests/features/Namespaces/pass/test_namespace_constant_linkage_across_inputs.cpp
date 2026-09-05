// EXPECT_COMPILE_ARGS: -Werror
// EXPECT_SOURCES: ["namespace_constant_linkage_other.cpp"]
#include "namespace_constant_linkage.h"
int main() {
  if (&private_number == other_private_number()) return 1;
  if (private_numbers == other_private_numbers() || other_private_numbers()[1] != 18)
    return 7;
  if (&private_pointer == other_private_pointer()) return 8;
  if (&values::private_number == other_nested_private_number()) return 2;
  if (&values::shared_number != other_shared_number()) return 3;
  if (values::external_number != 14 || Holder::number != 15) return 4;
  return volatile_number != 16 || other_external_number() != 29;
}
