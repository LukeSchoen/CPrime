// EXPECT_COMPILE_ARGS: -Werror
// EXPECT_SOURCES: ["inline_function_linkage_other.cpp"]
#include "inline_function_linkage.h"
int main() {
  if (other_reference_address() != &InlineApi::value) return 1;
  if (other_external_address() != &InlineApi::explicit_external) return 2;
  if (other_internal_address() == &InlineApi::internal) return 3;
  if (other_c_address() != &c_inline_value) return 4;
  if (InlineApi::value(2) != 5 || InlineApi::explicit_external(3) != 6) return 5;
  if (other_call() != 24 || c_inline_value(1) != 2) return 6;
  return 0;
}
