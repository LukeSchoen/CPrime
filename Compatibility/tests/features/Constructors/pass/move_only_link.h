#ifndef CPRIME_TEST_MOVE_ONLY_LINK_H
#define CPRIME_TEST_MOVE_ONLY_LINK_H

struct MoveOnlyValue
{
  int value;

  explicit MoveOnlyValue(int input) : value(input) {}
  MoveOnlyValue(const MoveOnlyValue &) = delete;
  MoveOnlyValue(MoveOnlyValue &&other) : value(other.value)
  {
    other.value = 0;
  }
  MoveOnlyValue &operator=(const MoveOnlyValue &) = delete;
  MoveOnlyValue &operator=(MoveOnlyValue &&other)
  {
    value = other.value;
    other.value = 0;
    return *this;
  }
};

MoveOnlyValue make_move_only_elsewhere(int value);

#endif
