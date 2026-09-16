// EXPECT_SOURCES: ["move_only_other.cpp"]

#include "move_only_link.h"

int main()
{
  MoveOnlyValue value = make_move_only_elsewhere(5);
  return value.value == 5 ? 0 : 1;
}
