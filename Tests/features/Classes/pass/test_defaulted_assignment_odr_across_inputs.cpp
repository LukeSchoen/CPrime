// EXPECT_COMPILE_ARGS: -Werror
// EXPECT_SOURCES: ["defaulted_assignment_odr_other.cpp"]
#include "defaulted_assignment_odr.h"
int main() {
  Assigned source, copy, moved;
  source.value = 17;
  copy = source;
  moved = static_cast<Assigned&&>(copy);
  Assigned other = assigned_from_other(moved);
  return moved.value != 17 || other.value != 17;
}
