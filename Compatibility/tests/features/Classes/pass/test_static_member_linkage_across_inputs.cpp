// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
// EXPECT_SOURCES: ["static_member_linkage_other.cpp"]
#include "static_member_linkage.h"
int main() {
  if (member_linkage::Service::value(4) != 13) return 1;
  if (member_linkage::Service::value(3.0) != 3.5) return 2;
  return member_linkage::Service::Nested_Type::value(7) != 12;
}
